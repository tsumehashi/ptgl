/*
 tdogl::Shader

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


#include "Shader.h"
#include <iostream>
#include <sstream>

namespace ptgl {

Shader::Shader()
{

}

Shader::~Shader()
{
    if (object_ != 0) {
        glDeleteShader(object_);
    }
}

bool Shader::load(ShaderType shaderType, const std::string& shaderCode)
{
    shaderType_ = shaderType;

    GLenum glShaderType = GL_VERTEX_SHADER;
    if (shaderType == Shader::VertexShader) {
        glShaderType = GL_VERTEX_SHADER;
    } else {
        glShaderType = GL_FRAGMENT_SHADER;
    }

    object_ = glCreateShader(glShaderType);
    if (object_ == 0) {
        std::cerr << "error : Shader::load glCreateShader faild" << std::endl;
        return false;
    }

    const char* code = shaderCode.c_str();
    glShaderSource(object_, 1, (const GLchar**)&code, NULL);

    // compile
    glCompileShader(object_);

    GLint status;
    glGetShaderiv(object_, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        std::string msg("Compile failure in shader:\n");

        GLint infoLogLength;
        glGetShaderiv(object_, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* strInfoLog = new char[infoLogLength + 1];
        glGetShaderInfoLog(object_, infoLogLength, NULL, strInfoLog);
        msg += strInfoLog;
        delete[] strInfoLog;

        glDeleteShader(object_); object_ = 0;
//        throw std::runtime_error(msg);
        std::cerr << "error : Shader::load " << msg << std::endl;
        return false;
    }

    return true;
}

ShaderPtr Shader::loadFromSource(ShaderType shaderType, const std::string& shaderCode)
{
    ShaderPtr shader = std::make_shared<Shader>();
    shader->load(shaderType, shaderCode);
    if (!shader->loaded()) {
        return nullptr;
    }
    return shader;
}

ShaderPtr Shader::loadFromStream(ShaderType shaderType, std::istream& is)
{
    //read whole file into stringstream buffer
    std::stringstream buffer;
    buffer << is.rdbuf();

    return Shader::loadFromSource(shaderType, buffer.str());
}

ShaderPtr Shader::loadFromFile(ShaderType shaderType, const std::string& filePath)
{
    //open file
    std::ifstream ifs;
    ifs.open(filePath.c_str(), std::ios::in | std::ios::binary);
    if(!ifs.is_open()){
        std::cerr << "error : Shader::loadFromFile Failed to open file: " << filePath << std::endl;
        return nullptr;
    }
    return Shader::loadFromStream(shaderType, ifs);
}

} /* namespace ptgl */
