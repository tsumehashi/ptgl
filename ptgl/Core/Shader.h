#ifndef PTGL_CORE_SHADER_H_
#define PTGL_CORE_SHADER_H_

#include <memory>
#include <vector>
#include <fstream>
#include "GLPath.h"

namespace ptgl {

class Shader;
typedef std::shared_ptr<Shader> ShaderPtr;

typedef std::vector<ShaderPtr> ShaderList;

class Shader {
public:
    enum ShaderType {
        VertexShader,
        FragmentShader
    };

    Shader();
    virtual ~Shader();

    static ShaderPtr loadFromSource(ShaderType shaderType, const std::string& shaderCode);
    static ShaderPtr loadFromStream(ShaderType shaderType, std::istream& is);
    static ShaderPtr loadFromFile(ShaderType shaderType, const std::string& filePath);

    bool load(ShaderType shaderType, const std::string& shaderCode);

    bool loaded() const { return object_ != 0; }
    ShaderType shaderType() const { return shaderType_; }
    GLuint object() const { return object_;    }

private:
    Shader(const Shader& other) = delete;
    Shader& operator = (const Shader& other) = delete;

    ShaderType shaderType_ = VertexShader;
    GLuint object_ = 0;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_SHADER_H_ */
