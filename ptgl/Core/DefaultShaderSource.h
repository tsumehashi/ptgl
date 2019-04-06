#ifndef PTGL_CORE_DEFAULTSHADERSOURCE_H_
#define PTGL_CORE_DEFAULTSHADERSOURCE_H_

#include <string>

namespace ptgl {

class DefaultShaderSource {
public:
    DefaultShaderSource();
    virtual ~DefaultShaderSource();

    // default shader source
    static const std::string DefaultVertexShaderSource;
    static const std::string DefaultFragmentShaderSource;

    // default picking up shader source
    static const std::string PickingUpVertexShaderSource;
    static const std::string PickingUpFragmentShaderSource;

    // default depth render shader source
    static const std::string DepthRenderVertexShaderSource;
    static const std::string DepthRenderFragmentShaderSource;

    // default shader source
    static const std::string Default2DVertexShaderSource;
    static const std::string Default2DFragmentShaderSource;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_DEFAULTSHADERSOURCE_H_ */
