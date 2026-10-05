#ifndef PTGL_CORE_PLASTICSHADERSOURCE_H_
#define PTGL_CORE_PLASTICSHADERSOURCE_H_

#include <string>

namespace ptgl {

struct PlasticShaderSource {
    static const std::string VertexShaderSource;
    // Shared by Plastic and CAD surfaces; caller defines PTGL_SHADOW_PRECISION.
    static const std::string ShadowFragmentShaderSource;
    static const std::string FragmentShaderSource;
};

} // namespace ptgl

#endif
