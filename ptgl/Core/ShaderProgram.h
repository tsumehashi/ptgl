#ifndef PTGL_CORE_SHADERPROGRAM_H_
#define PTGL_CORE_SHADERPROGRAM_H_

#include <memory>
#include <string>
#include <Eigen/Core>
#include "GLPath.h"
#include "Shader.h"

namespace ptgl {

class ShaderProgram;
typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;

class ShaderProgram {
public:

    // Three.js (Vertex Shader)
    // uniform mat4 modelMatrix
    // uniform mat4 viewMatrix
    // uniform mat4 modelViewMatrix
    // uniform mat4 projectionMatrix
    // uniform mat3 normalMatrix
    // uniform vec3 cameraPosition
    // attribute vec3 position
    // attribute vec3 normal
    // attribute vec2 uv

    // Three.js (Fragment Shader)
    // uniform mat4 viewMatrix
    // uniform vec3 cameraPosition

    //
    // uniform mat4 modelMatrix
    // uniform mat4 viewMatrix
    // uniform mat4 modelViewMatrix
    // uniform mat4 projectionMatrix
    // uniform mat3 normalMatrix
    // uniform vec3 cameraPosition
    // uniform vec4 color
    // uniform vec3 lightDirection
    // uniform float lightEffectRate
    // uniform float pointSize
    // attribute vec3 position
    // attribute vec3 normal
    // attribute vec2 uv

    template <typename T> using vec2 = Eigen::Matrix<T, 2, 1>;
    template <typename T> using vec3 = Eigen::Matrix<T, 3, 1>;
    template <typename T> using vec4 = Eigen::Matrix<T, 4, 1>;
    template <typename T> using mat2 = Eigen::Matrix<T, 2, 2>;
    template <typename T> using mat3 = Eigen::Matrix<T, 3, 3>;
    template <typename T> using mat4 = Eigen::Matrix<T, 4, 4>;

    ShaderProgram();
    virtual ~ShaderProgram();

    void setDebugOut(bool on) { debugOut_ = on; }

    void setName(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

    bool linkShaders(const ShaderList& shaders);

    bool valid() const { return loaded(); }
    bool loaded() const { return object_ != 0; }
    GLuint object() const { return object_; }

    bool binded() const { return binded_; }
    void bind();
    void unbind();

    GLint attribute(const std::string& name) const;
    GLint uniform(const std::string& name) const;

    // Scalar
    bool setParameter(GLint location, double x);
    bool setParameter(GLint location, double x, double y);
    bool setParameter(GLint location, double x, double y, double z);
    bool setParameter(GLint location, double x, double y, double z, double w);

    bool setParameter(const std::string& name, double x);
    bool setParameter(const std::string& name, double x, double y);
    bool setParameter(const std::string& name, double x, double y, double z);
    bool setParameter(const std::string& name, double x, double y, double z, double w);

    // Vector
    template <typename T>
    bool setParameter(GLint location, const vec2<T>& v)
    {
        if (location == -1) {
            return false;
        }
        vec2<float> vf = v.template cast<float>();
        glUniform2fv(location, 1, vf.data());
        return true;
    }

    template <typename T>
    bool setParameter(GLint location, const vec3<T>& v)
    {
        if (location == -1) {
            return false;
        }
        vec3<float> vf = v.template cast<float>();
        glUniform3fv(location, 1, vf.data());
        return true;
    }

    template <typename T>
    bool setParameter(GLint location, const vec4<T>& v)
    {
        if (location == -1) {
            return false;
        }
        vec4<float> vf = v.template cast<float>();
        glUniform4fv(location, 1, vf.data());
        return true;
    }

    // Matrix
    template <typename T>
    bool setParameter(GLint location, const mat2<T>& m)
    {
        if (location == -1) {
            return false;
        }
        mat2<float> mf = m.template cast<float>();
        glUniformMatrix2fv(location, 1, GL_FALSE, mf.data());    // column-major
        return true;
    }

    template <typename T>
    bool setParameter(GLint location, const mat3<T>& m)
    {
        if (location == -1) {
            return false;
        }
        mat3<float> mf = m.template cast<float>();
        glUniformMatrix3fv(location, 1, GL_FALSE, mf.data());    // column-major
        return true;
    }

    template <typename T>
    bool setParameter(GLint location, const mat4<T>& m)
    {
        if (location == -1) {
            return false;
        }
        mat4<float> mf = m.template cast<float>();
        glUniformMatrix4fv(location, 1, GL_FALSE, mf.data());    // column-major
        return true;
    }

    // Vector
    template <typename T>
    bool setParameter(const std::string& name, const vec2<T>& v)
    {
        GLint location = uniform(name);
        return setParameter(location, v);
    }

    template <typename T>
    bool setParameter(const std::string& name, const vec3<T>& v)
    {
        GLint location = uniform(name);
        return setParameter(location, v);
    }

    template <typename T>
    bool setParameter(const std::string& name, const vec4<T>& v)
    {
        GLint location = uniform(name);
        return setParameter(location, v);
    }

    // Matrix
    template <typename T>
    bool setParameter(const std::string& name, const mat2<T>& m)
    {
        GLint location = uniform(name);
        return setParameter(location, m);
    }

    template <typename T>
    bool setParameter(const std::string& name, const mat3<T>& m)
    {
        GLint location = uniform(name);
        return setParameter(location, m);
    }

    template <typename T>
    bool setParameter(const std::string& name, const mat4<T>& m)
    {
        GLint location = uniform(name);
        return setParameter(location, m);
    }

private:
    ShaderProgram(const ShaderProgram&) = delete;
    const ShaderProgram& operator = (const ShaderProgram&) = delete;

    std::string name_;

    bool debugOut_ = false;
    bool binded_ = false;
    GLuint object_ = 0;

};

} /* namespace ptgl */

#endif /* PTGL_CORE_SHADERPROGRAM_H_ */
