#include "OBJLoader.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "ptgl/Util/Intersection.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "thirdparty/Loader/OBJ/tinyobjloader/tiny_obj_loader.h"

namespace ptgl {
namespace loader {

OBJLoader::OBJLoader() {

}

OBJLoader::~OBJLoader() {

}

VertexListPtr OBJLoader::loadVertex(const std::string& filepath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    std::ifstream input(std::filesystem::u8path(filepath));
    if (!input) {
        std::cerr << "OBJLoader: cannot open " << filepath << std::endl;
        return nullptr;
    }
    // This API returns geometry/UVs only; material libraries are not imported.
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &input, nullptr, true);

    if (!err.empty()) { // `err` may contain warning message.
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cerr << "error OBJLoader::loadVertex" << std::endl;
        return nullptr;
    }

    VertexListPtr vertexList = std::make_shared<VertexList>();

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = shapes[s].mesh.num_face_vertices[f];
            if (fv != 3) {
                std::cerr << "error OBJLoader::loadVertex: expected a triangle" << std::endl;
                return nullptr;
            }
            const size_t faceStart = vertexList->size();
            bool missingNormals = false;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                if (idx.vertex_index < 0 ||
                    static_cast<size_t>(idx.vertex_index) >= attrib.vertices.size() / 3 ||
                    idx.normal_index < -1 ||
                    (idx.normal_index >= 0 && static_cast<size_t>(idx.normal_index) >= attrib.normals.size() / 3) ||
                    idx.texcoord_index < -1 ||
                    (idx.texcoord_index >= 0 && static_cast<size_t>(idx.texcoord_index) >= attrib.texcoords.size() / 2)) {
                    std::cerr << "error OBJLoader::loadVertex: invalid vertex attribute index" << std::endl;
                    return nullptr;
                }
                float vx = attrib.vertices[3*idx.vertex_index+0];
                float vy = attrib.vertices[3*idx.vertex_index+1];
                float vz = attrib.vertices[3*idx.vertex_index+2];
                float nx = 0;
                float ny = 0;
                float nz = 0;
                float tx = 0;
                float ty = 0;

                // set normals
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3*idx.normal_index+0];
                    ny = attrib.normals[3*idx.normal_index+1];
                    nz = attrib.normals[3*idx.normal_index+2];
                } else {
                    missingNormals = true;
                }

                // set texcorrds
                if (idx.texcoord_index >= 0) {
                    tx = attrib.texcoords[2*idx.texcoord_index+0];
                    ty = attrib.texcoords[2*idx.texcoord_index+1];
                }
                vertexList->emplace_back(vx, vy, vz, nx, ny, nz, tx, ty);
            }

            if (missingNormals) {
                // generate normals
                auto& v0 = (*vertexList)[faceStart];
                auto& v1 = (*vertexList)[faceStart+1];
                auto& v2 = (*vertexList)[faceStart+2];

                Eigen::Vector3d nv = calcPlaneNorm(Eigen::Vector3d(v0.x, v0.y, v0.z),
                                                   Eigen::Vector3d(v1.x, v1.y, v1.z),
                                                   Eigen::Vector3d(v2.x, v2.y, v2.z));
                for (size_t v = 0; v < fv; ++v) {
                    if (shapes[s].mesh.indices[index_offset + v].normal_index < 0) {
                        auto& vertex = (*vertexList)[faceStart + v];
                        vertex.nx = nv(0); vertex.ny = nv(1); vertex.nz = nv(2);
                    }
                }
            }

            index_offset += fv;

            // per-face material
//            shapes[s].mesh.material_ids[f];
        }
    }

    return vertexList;
}

}
} /* namespace ptgl */
