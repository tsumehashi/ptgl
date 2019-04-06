#ifndef PTGL_LOADER_OBJ_OBJLOADER_H_
#define PTGL_LOADER_OBJ_OBJLOADER_H_

#include <string>
#include "ptgl/Core/Vertex.h"

namespace ptgl {
namespace loader {

class OBJLoader {
public:
    OBJLoader();
    virtual ~OBJLoader();

    VertexListPtr loadVertex(const std::string& filepath);
};

}
} /* namespace ptgl */

#endif /* PTGL_LOADER_OBJ_OBJLOADER_H_ */
