#include "ptgl/Plot/Stream.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
static double percentile(std::vector<double> v, double p) {
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, std::size_t(p * (v.size() - 1)))];
}
int main() {
    constexpr std::size_t channels = 32, capacity = 60000;
    ptgl::plot::StreamBuffer stream(channels, capacity);
    std::array<float, channels> row{};
    std::uint64_t tick = 0;
    auto append = [&] {
        for (std::size_t c = 0; c < channels; ++c)
            row[c] = float(100 * std::sin(tick * .001 * (1 + c * .05)));
        stream.append(double(tick++) * .001, row.data(), row.size());
    };
    const auto start = Clock::now();
    for (std::size_t i = 0; i < capacity; ++i) append();
    std::cout << std::fixed << std::setprecision(3)
              << "CPU benchmark only; no GPU/presentation timing. Synthetic input.\n"
              << "channels=" << channels << " capacity=" << capacity
              << " core_storage_MiB=" << stream.storageBytes() / 1048576.0
              << " prefill_ms=" << ms(start, Clock::now()) << '\n';
    std::vector<ptgl::plot::Point> output;
    output.reserve(capacity);
    for (const auto width : {512u, 1920u}) {
        for (const auto seconds : {60.0, 10.0, .1}) {
            std::vector<double> ingest, query;
            std::size_t total_points = 0;
            for (int frame = 0; frame < 180; ++frame) {
                const auto a = Clock::now();
                for (int j = 0; j < 17; ++j) append();
                const auto b = Clock::now();
                const ptgl::plot::Range view{stream.newestTime() - seconds, stream.newestTime()};
                for (std::size_t c = 0; c < channels; ++c) {
                    stream.select(c, view, width, output);
                    total_points += output.size();
                }
                const auto e = Clock::now();
                if (frame >= 10) { ingest.push_back(ms(a, b)); query.push_back(ms(b, e)); }
            }
            std::cout << "width=" << width << " seconds=" << seconds
                      << " ingest17_p95_ms=" << percentile(ingest, .95)
                      << " select32_p50_ms=" << percentile(query, .50)
                      << " p95_ms=" << percentile(query, .95)
                      << " p99_ms=" << percentile(query, .99)
                      << " avg_points=" << total_points / 180 << '\n';
        }
    }
}
