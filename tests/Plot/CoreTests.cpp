#include "ptgl/Plot/Stream.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string(#x) + " at line " + std::to_string(__LINE__)); } while (false)

static std::vector<ptgl::plot::Point> reference(const std::vector<ptgl::plot::Point>& raw, ptgl::plot::Range view, std::size_t width) {
    std::vector<ptgl::plot::Point> result;
    if (raw.empty() || !view.valid() || !width || view.max < raw.front().time || view.min > raw.back().time) return result;
    width = std::min(width, raw.size());
    bool gap = true;
    auto bucket = [&](const std::vector<ptgl::plot::Point>& items) {
        std::size_t i = 0;
        while (i < items.size()) {
            if (!std::isfinite(items[i].value)) { gap = true; ++i; continue; }
            const auto first = i;
            auto low = i, high = i;
            while (i < items.size() && std::isfinite(items[i].value)) {
                if (items[i].value < items[low].value) low = i;
                if (items[i].value > items[high].value) high = i;
                ++i;
            }
            std::vector<std::size_t> indices{first, low, high, i - 1};
            std::sort(indices.begin(), indices.end());
            indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
            for (auto index : indices) { auto p = items[index]; p.break_before = gap; result.push_back(p); gap = false; }
        }
    };
    std::vector<std::vector<ptgl::plot::Point>> columns(width);
    auto first = std::lower_bound(raw.begin(), raw.end(), view.min, [](auto p, double t) { return p.time < t; });
    auto after = std::upper_bound(raw.begin(), raw.end(), view.max, [](double t, auto p) { return t < p.time; });
    if (std::size_t(after - first) <= width) {
        const auto a = first == raw.begin() ? first : first - 1;
        const auto b = after == raw.end() ? after : after + 1;
        for (auto it = a; it != b; ++it) {
            if (!std::isfinite(it->value)) { gap = true; continue; }
            auto p = *it; p.break_before = gap; result.push_back(p); gap = false;
        }
        return result;
    }
    if (first != raw.begin()) bucket({*(first - 1)});
    // Independent full scan; deliberately no block summaries or ring indexing.
    std::size_t column = 0;
    for (auto it = first; it != after; ++it) {
        while (column + 1 < width && it->time >= view.min + view.span() * (double(column + 1) / width)) ++column;
        columns[column].push_back(*it);
    }
    for (const auto& items : columns) bucket(items);
    if (after != raw.end()) bucket({*after});
    return result;
}

static void compare(const std::vector<ptgl::plot::Point>& a, const std::vector<ptgl::plot::Point>& b) {
    CHECK(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].time == b[i].time);
        CHECK(a[i].value == b[i].value);
        CHECK(a[i].break_before == b[i].break_before);
        if (i) CHECK(a[i].time > a[i - 1].time);
    }
}

static void validationAndPrecision() {
    bool threw = false;
    try { ptgl::plot::StreamBuffer invalid(0, 1); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    ptgl::plot::StreamBuffer s(2, 3);
    float row[]{1, 2};
    CHECK(!s.append(0, nullptr, 2));
    CHECK(!s.append(0, row, 1));
    CHECK(!s.append(std::numeric_limits<double>::infinity(), row, 2));
    CHECK(s.size() == 0);
    const double epoch = 1800000000.0;
    CHECK(s.append(epoch, row, 2));
    CHECK(!s.append(epoch, row, 2));
    CHECK(!s.append(epoch - 1, row, 2));
    row[0] = 3;
    CHECK(s.append(epoch + .001, row, 2));
    ptgl::plot::Point p;
    CHECK(s.nearest(0, epoch + .0009, p));
    CHECK(p.value == 3);
    CHECK(!s.nearest(0, epoch - 1, p));
    CHECK(s.sample(0, 1).time - s.sample(0, 0).time > .0009);
    auto memory = s.storageBytes();
    for (int i = 2; i < 100; ++i) CHECK(s.append(epoch + i * .001, row, 2));
    CHECK(s.size() == 3);
    CHECK(s.overwritten() == 97);
    CHECK(s.storageBytes() == memory);
    s.clear();
    CHECK(s.size() == 0 && s.overwritten() == 0);
    CHECK(s.append(0, row, 2));
    CHECK(s.sample(0, 0).value == 3);
}

static void extremaAndGaps() {
    ptgl::plot::StreamBuffer s(1, 2048);
    std::vector<ptgl::plot::Point> raw, out;
    for (int i = 0; i < 2048; ++i) {
        float v = 0;
        if (i == 751) v = 999;
        if (i == 750) v = -888;
        if (i >= 300 && i <= 304) v = std::numeric_limits<float>::quiet_NaN();
        if (i == 999) v = std::numeric_limits<float>::infinity();
        CHECK(s.append(i * .001, &v, 1));
        raw.push_back(s.sample(0, i));
    }
    s.select(0, {0, 2.047}, 20, out);
    compare(out, reference(raw, {0, 2.047}, 20));
    CHECK(std::any_of(out.begin(), out.end(), [](auto p) { return p.value == 999; }));
    CHECK(std::any_of(out.begin(), out.end(), [](auto p) { return p.value == -888; }));
    CHECK(std::count_if(out.begin(), out.end(), [](auto p) { return p.break_before; }) == 3);
    ptgl::plot::Point p;
    CHECK(!s.nearest(0, .3, p));
    s.select(0, {.7501, .7509}, 1200, out); // View contains only a crossing segment.
    CHECK(out.size() == 2 && out.front().time == .750 && out.back().time == .751);
    s.select(0, {-2, -1}, 20, out);
    CHECK(out.empty());
}

static void randomizedRingQueries() {
    std::mt19937 rng(71834);
    for (std::size_t cap : {1u, 7u, 16u, 17u, 63u, 128u, 513u}) {
        ptgl::plot::StreamBuffer s(3, cap);
        std::vector<ptgl::plot::Point> raw[3];
        double t = 1700000000.0;
        for (std::size_t n = 0; n < cap * 4 + 40; ++n) {
            t += .001 + (rng() % 50) * .0001;
            float row[3];
            for (auto& v : row) v = rng() % 17 == 0 ? std::numeric_limits<float>::quiet_NaN() : float(int(rng() % 101) - 50);
            CHECK(s.append(t, row, 3));
            for (std::size_t c = 0; c < 3; ++c) {
                raw[c].push_back({t, row[c], false});
                if (raw[c].size() > cap) raw[c].erase(raw[c].begin());
                if (n % 13) continue;
                for (int trial = 0; trial < 5; ++trial) {
                    const double span = std::max(.01, t - raw[c].front().time);
                    const double start = raw[c].front().time + span * (int(rng() % 140) - 20) / 100.0;
                    const ptgl::plot::Range view{start, start + span * (1 + rng() % 120) / 100.0};
                    const auto width = std::size_t(1 + rng() % 100);
                    std::vector<ptgl::plot::Point> out;
                    s.select(c, view, width, out);
                    compare(out, reference(raw[c], view, width));
                }
            }
        }
    }
}

static void rangeControls() {
    ptgl::plot::Range r{0, 60};
    r.zoom(15, .5, .005, 60);
    CHECK(r.min == 7.5 && r.max == 37.5);
    r.pan(-20);
    r.clampTo({0, 60});
    CHECK(r.min == 0 && r.max == 30);
    r.zoom(15, 100, .005, 60);
    r.clampTo({0, 60});
    CHECK(r.min == 0 && r.max == 60);
    r.zoom(15, -1, .005, 60);
    CHECK(r.span() == 60);
}

static void stableScrolling() {
    ptgl::plot::StreamBuffer data(1, 12000);
    const double epoch = 1700000000;
    for (int i = 0; i < 12000; ++i) {
        const float v = i == 5011 ? 999.f : i == 6021 ? -999.f :
            i >= 4500 && i < 4510 ? NAN : float(std::sin(i * .137) + std::sin(i * .013));
        CHECK(data.append(epoch + i * .001, &v, 1));
    }
    std::vector<ptgl::plot::Point> a, b, before;
    data.select(0, {epoch+2.005,epoch+8.005}, 100, a, ptgl::plot::BucketAlignment::time);
    data.select(0, {epoch+2.015,epoch+8.015}, 100, b, ptgl::plot::BucketAlignment::time);
    auto interior = [&](std::vector<ptgl::plot::Point> v) {
        v.erase(std::remove_if(v.begin(),v.end(),[&](auto p){return p.time < epoch+2.2 || p.time > epoch+7.8;}),v.end());
        if (!v.empty()) v[0].break_before = true;
        return v;
    };
    compare(interior(a), interior(b)); // Subpixel pans do not replace interior samples.
    CHECK(std::count_if(a.begin(),a.end(),[](auto p){return p.break_before;}) == 2);
    CHECK(std::any_of(a.begin(),a.end(),[](auto p){return p.value == 999;}));
    CHECK(std::any_of(a.begin(),a.end(),[](auto p){return p.value == -999;}));
    before = a;
    for (int i = 12000; i < 12100; ++i) { float v = 1; CHECK(data.append(epoch+i*.001,&v,1)); }
    data.select(0, {epoch+2.005,epoch+8.005}, 100, a, ptgl::plot::BucketAlignment::time);
    compare(before,a); // Eviction must not move the time origin.
    data.clear();
    for (int i = 0; i < 100; ++i) { float v = float(i); CHECK(data.append(-100+i*.01,&v,1)); }
    data.select(0,{-100,-99.01},10,a,ptgl::plot::BucketAlignment::time);
    CHECK(!a.empty() && a.front().time == -100 && a.back().value == 99);
}

int main() {
    try {
        validationAndPrecision();
        extremaAndGaps();
        randomizedRingQueries();
        rangeControls();
        stableScrolling();
        std::cout << "PASS: validation, timestamp precision, eviction, extrema, gaps, randomized queries, view controls\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
