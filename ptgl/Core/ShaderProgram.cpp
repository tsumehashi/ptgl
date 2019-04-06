/*
 tdogl::Program

 Copyright 2012 Thomas Dalling - http://tomdalling.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */


#include "ShaderProgram.h"
#include <iostream>
#include "GLPath.h"

namespace ptgl {

ShaderProgram::ShaderProgram()
{


}

ShaderProgram::~ShaderProgram() {
    if (object_ != 0) {
        if (binded()) {
            unbind();
        }
        glDeleteProgram(object_);
    }
}

bool ShaderProgram::linkShaders(const ShaderList& shaders)
{
    if (shaders.empty()) {
        std::cerr << "error ShaderProgram::linkShaders No shaders were provided to create the program" << std::endl;
        return false;
    }

    object_ = glCreateProgram();
    if (object_ == 0) {
        std::cerr << "error ShaderProgram::linkShaders glCreateProgram failed" << std::endl;
        return false;
    }

    // attach all the shaders
    for (auto& shader : shaders) {
        if (shader) {
            glAttachShader(object_, shader->object());
        }
    }

    // link the shaders together
    glLinkProgram(object_);

    // detach all the shaders
    for (auto& shader : shaders) {
        glDetachShader(object_, shader->object());
    }

    //throw exception if linking failed
    GLint status;
    glGetProgramiv(object_, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        std::string msg("Program linking failure: ");

        GLint infoLogLength;
        glGetProgramiv(object_, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* strInfoLog = new char[infoLogLength + 1];
        glGetProgramInfoLog(object_, infoLogLength, NULL, strInfoLog);
        msg += strInfoLog;
        delete[] strInfoLog;

        glDeleteProgram(object_); object_ = 0;
        std::cerr << "error ShaderProgram::linkShaders " << msg << std::endl;
        return false;
    }

    return true;
}

void ShaderProgram::bind()
{
    // bind the program (the shaders)
    binded_ = true;
    glUseProgram(object());
}

void ShaderProgram::unbind()
{
    // unbind the program
    binded_ = false;
    glUseProgram(0);
}

GLint ShaderProgram::attribute(const std::string& name) const
{
    GLint id = glGetAttribLocation(object_, (const GLchar*)name.c_str());
    if (id == -1) {
//        throw std::runtime_error(std::string("Program attribute not found: ") + attribName);
        if (debugOut_) {
            std::cerr << "error : ShaderProgram::attribute Program attribute not found: " << name << std::endl;
        }
    }

    return id;
}

GLint ShaderProgram::uniform(const std::string& name) const
{
    GLint id = glGetUniformLocation(object_, (const GLchar*)name.c_str());
    if (id == -1) {
//        throw std::runtime_error(std::string("Program attribute not found: ") + attribName);
        if (debugOut_) {
            std::cerr << "error : ShaderProgram::uniform Program uniform not found: " << name << std::endl;
        }
    }

    return id;
}

bool ShaderProgram::setParameter(GLint location, double x)
{
    if (location == -1) {
        return false;
    }
    glUniform1f(location, (float)x);
    return true;
}

bool ShaderProgram::setParameter(GLint location, double x, double y)
{
    if (location == -1) {
        return false;
    }
    glUniform2f(location, (float)x, (float)y);
    return true;
}

bool ShaderProgram::setParameter(GLint location, double x, double y, double z)
{
    if (location == -1) {
        return false;
    }
    glUniform3f(location, (float)x, (float)y, (float)z);
    return true;
}

bool ShaderProgram::setParameter(GLint location, double x, double y, double z, double w)
{
    if (location == -1) {
        return false;
    }
    glUniform4f(location, (float)x, (float)y, (float)z, (float)w);
    return true;
}

bool ShaderProgram::setParameter(const std::string& name, double x)
{
    GLint id = uniform(name);
    return setParameter(id, x);
}

bool ShaderProgram::setParameter(const std::string& name, double x, double y)
{
    GLint id = uniform(name);
    return setParameter(id, x, y);
}

bool ShaderProgram::setParameter(const std::string& name, double x, double y, double z)
{
    GLint id = uniform(name);
    return setParameter(id, x, y, z);
}

bool ShaderProgram::setParameter(const std::string& name, double x, double y, double z, double w)
{
    GLint id = uniform(name);
    return setParameter(id, x, y, z, w);
}

} /* namespace ptgl */
