#include "OBJLoader.h"
#include <iostream>
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
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str());    // triangulate = true

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

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                float vx = attrib.vertices[3*idx.vertex_index+0];
                float vy = attrib.vertices[3*idx.vertex_index+1];
                float vz = attrib.vertices[3*idx.vertex_index+2];
                float nx = 0;
                float ny = 0;
                float nz = 0;
                float tx = 0;
                float ty = 0;

                // set normals
                if (!attrib.normals.empty()) {
                    nx = attrib.normals[3*idx.normal_index+0];
                    ny = attrib.normals[3*idx.normal_index+1];
                    nz = attrib.normals[3*idx.normal_index+2];
                }

                // set texcorrds
                if (!attrib.texcoords.empty()) {
                    tx = attrib.texcoords[2*idx.texcoord_index+0];
                    ty = attrib.texcoords[2*idx.texcoord_index+1];
                }
                vertexList->emplace_back(vx, vy, vz, nx, ny, nz, tx, ty);
            }

            if (attrib.normals.empty()) {
                // generate normals
                auto& v0 = (*vertexList)[vertexList->size()-3];
                auto& v1 = (*vertexList)[vertexList->size()-2];
                auto& v2 = (*vertexList)[vertexList->size()-1];

                Eigen::Vector3d nv = calcPlaneNorm(Eigen::Vector3d(v0.x, v0.y, v0.z),
                                                   Eigen::Vector3d(v1.x, v1.y, v1.z),
                                                   Eigen::Vector3d(v2.x, v2.y, v2.z));
                v0.nx = nv(0); v0.ny = nv(1); v0.nz = nv(2);
                v1.nx = nv(0); v1.ny = nv(1); v1.nz = nv(2);
                v2.nx = nv(0); v2.ny = nv(1); v2.nz = nv(2);
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
