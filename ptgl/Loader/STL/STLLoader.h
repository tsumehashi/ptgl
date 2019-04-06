#ifndef PTGL_LOADER_STL_STLLOADER_H_
#define PTGL_LOADER_STL_STLLOADER_H_

#include <string>
#include <fstream>
#include "ptgl/Core/Vertex.h"

namespace ptgl {
namespace loader {

class STLLoader {
public:
    STLLoader();
    virtual ~STLLoader();

    VertexListPtr loadVertex(const std::string& filepath);
    VertexListPtr loadVertex(std::istream& is);
};

}
} /* namespace ptgl */

#endif /* PTGL_LOADER_STL_STLLOADER_H_ */
