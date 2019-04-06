#include "STLLoader.h"
#include <iostream>
#include "thirdparty/Loader/STL/stl_parser/parse_stl.h"

namespace ptgl {
namespace loader {

STLLoader::STLLoader() {

}

STLLoader::~STLLoader() {

}

VertexListPtr STLLoader::loadVertex(const std::string& filepath)
{
    try {
        stl::stl_data data = stl::parse_stl(filepath);

        ptgl::VertexListPtr vertexList = std::make_shared<ptgl::VertexList>();
        for (size_t i = 0; i < data.triangles.size(); ++i) {
            auto& tri = data.triangles[i];
            vertexList->emplace_back(tri.v1.x, tri.v1.y, tri.v1.z, tri.normal.x, tri.normal.y, tri.normal.z);
            vertexList->emplace_back(tri.v2.x, tri.v2.y, tri.v2.z, tri.normal.x, tri.normal.y, tri.normal.z);
            vertexList->emplace_back(tri.v3.x, tri.v3.y, tri.v3.z, tri.normal.x, tri.normal.y, tri.normal.z);
        }

        return vertexList;

    } catch (std::exception& e) {
        std::cerr << "catch exception STLLoader::loadBinary filepath = " << filepath << std::endl;
        std::cerr << e.what() << std::endl;
        return nullptr;
    }

    return nullptr;
}

VertexListPtr STLLoader::loadVertex(std::istream& is)
{
    try {
        stl::stl_data data = stl::parse_stl(is);

        ptgl::VertexListPtr vertexList = std::make_shared<ptgl::VertexList>();
        for (size_t i = 0; i < data.triangles.size(); ++i) {
            auto& tri = data.triangles[i];
            vertexList->emplace_back(tri.v1.x, tri.v1.y, tri.v1.z, tri.normal.x, tri.normal.y, tri.normal.z);
            vertexList->emplace_back(tri.v2.x, tri.v2.y, tri.v2.z, tri.normal.x, tri.normal.y, tri.normal.z);
            vertexList->emplace_back(tri.v3.x, tri.v3.y, tri.v3.z, tri.normal.x, tri.normal.y, tri.normal.z);
        }

        return vertexList;

    } catch (std::exception& e) {
        std::cerr << "catch exception STLLoader::loadBinary" << std::endl;
        std::cerr << e.what() << std::endl;
        return nullptr;
    }

    return nullptr;
}

}
} /* namespace ptgl */
