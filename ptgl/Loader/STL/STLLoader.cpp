#include "STLLoader.h"
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace ptgl
{
namespace loader
{
namespace
{
std::uint32_t uint32(const unsigned char *p)
{
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) |
           (std::uint32_t(p[3]) << 24);
}
float number(const unsigned char *p)
{
    const auto bits = uint32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    if (!std::isfinite(value))
        throw std::runtime_error("Nonfinite STL coordinate/normal");
    return value;
}
VertexListPtr readMesh(std::istream &input)
{
    if (!input)
        throw std::runtime_error("Cannot read STL stream");
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),
                                           std::istreambuf_iterator<char>()};
    if (input.bad())
        throw std::runtime_error("STL stream read failed");
    auto vertices = std::make_shared<VertexList>();
    // A binary header may start with 'solid'. Exact length is the discriminator;
    // validate it before trusting the triangle count or allocating vertex storage.
    if (bytes.size() >= 84 && std::uint64_t(uint32(bytes.data() + 80)) * 50 + 84 == bytes.size()) {
        const auto count = uint32(bytes.data() + 80);
        vertices->reserve(size_t(count) * 3);
        for (size_t i = 0; i < count; ++i) {
            const auto *facet = bytes.data() + 84 + i * 50;
            const float nx = number(facet), ny = number(facet + 4), nz = number(facet + 8);
            for (int j = 0; j < 3; ++j) {
                const auto *p = facet + 12 + j * 12;
                vertices->emplace_back(number(p), number(p + 4), number(p + 8), nx, ny, nz);
            }
        }
        return vertices;
    }
    std::istringstream text(std::string(bytes.begin(), bytes.end()));
    text.imbue(std::locale::classic());
    std::string word, rest;
    auto expect = [&](const char *expected) {
        if (!(text >> word) || word != expected)
            throw std::runtime_error("Malformed or truncated STL");
    };
    auto point = [&] {
        std::array<float, 3> p;
        for (auto &value : p)
            if (!(text >> value) || !std::isfinite(value))
                throw std::runtime_error("Invalid ASCII STL coordinate/normal");
        return p;
    };
    expect("solid");
    std::getline(text, rest);
    while (text >> word) {
        if (word == "endsolid") {
            std::getline(text, rest);
            text >> std::ws;
            if (text.eof())
                return vertices;
            expect("solid");
            std::getline(text, rest);
            continue;
        }
        if (word != "facet")
            throw std::runtime_error("Expected ASCII STL facet");
        expect("normal");
        const auto n = point();
        expect("outer");
        expect("loop");
        for (int j = 0; j < 3; ++j) {
            expect("vertex");
            const auto p = point();
            vertices->emplace_back(p[0], p[1], p[2], n[0], n[1], n[2]);
        }
        expect("endloop");
        expect("endfacet");
    }
    throw std::runtime_error("Missing ASCII STL endsolid");
}
} // namespace

STLLoader::STLLoader() = default;
STLLoader::~STLLoader() = default;

VertexListPtr STLLoader::loadVertex(const std::string &filepath)
{
    std::ifstream stream(std::filesystem::u8path(filepath), std::ios::binary);
    return loadVertex(stream);
}

VertexListPtr STLLoader::loadVertex(std::istream &stream)
{
    try {
        return readMesh(stream);
    } catch (const std::exception &error) {
        std::cerr << "STLLoader: " << error.what() << '\n';
        return nullptr;
    }
}
} // namespace loader
} // namespace ptgl
