#include "VertexBufferObject.h"

namespace ptgl {

VertexBufferObject::VertexBufferObject() {

}

VertexBufferObject::~VertexBufferObject() {

}

VBO::VBO()
{

}

VBO::~VBO()
{

}

bool VBO::loadVertex(GLuint& vbo, const std::vector<GLfloat> vertex, GLenum usage)
{
    return loadVertex(vbo, vertex.data(), vertex.size(), usage);
}

bool VBO::loadVertex(GLuint& vbo, const GLfloat* vertex, size_t size, GLenum usage)
{
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*size, vertex, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo != 0;
}

bool VBO::loadIndex(GLuint& vbo, const std::vector<GLuint> indices, GLenum usage)
{
    return loadIndex(vbo, indices.data(), indices.size(), usage);
}

bool VBO::loadIndex(GLuint& vbo, const GLuint* indices, size_t size, GLenum usage)
{
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*size, indices, usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return vbo != 0;
}

bool VBO::loadVertex(const std::vector<GLfloat> vertex, GLenum usage)
{
    return loadVertex(vertexVBO_, vertex, usage);
}

bool VBO::loadVertex(const GLfloat* vertex, size_t size, GLenum usage)
{
    return loadVertex(vertexVBO_, vertex, size, usage);
}

bool VBO::loadIndex(const std::vector<GLuint> indices, GLenum usage)
{
    return loadIndex(indexVBO_, indices, usage);
}

bool VBO::loadIndex(const GLuint* indices, size_t size, GLenum usage)
{
    return loadIndex(indexVBO_, indices, size, usage);
}

bool VertexBufferObject::isValid() const
{
    return (vertexVBO_ != 0) && (indexVBO_ != 0);
}

bool VertexBufferObject::loadVertices(GLuint& vbo, const Vertex* vertices, size_t size, GLenum usage)
{
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*size, vertices, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo != 0;
}

bool VertexBufferObject::loadIndices(GLuint& vbo, const GLuint* indices, size_t size, GLenum usage)
{
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*size, indices, usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return vbo != 0;
}

bool VertexBufferObject::loadVertices(const Vertex* vertices, size_t size, GLenum usage)
{
    vertexUsage_ = usage;
    numVertices_ = size;
    vertexVBOBufferSize_ = sizeof(Vertex) * size;
    return loadVertices(vertexVBO_, vertices, size, usage);
}

bool VertexBufferObject::loadIndices(const GLuint* indices, size_t size, GLenum usage)
{
    indexUsage_ = usage;
    numIndices_ = size;
    indexVBOBufferSize_ = sizeof(GLuint) * size;
    return loadIndices(indexVBO_, indices, size, usage);
}

bool VertexBufferObject::updateVertices(GLuint vbo, const Vertex* vertices, size_t size)
{
    if (vbo == 0) return false;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * size, vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool VertexBufferObject::updateIndices(GLuint vbo, const GLuint* indices, size_t size)
{
    if (vbo == 0) return false;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint) * size, indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return true;
}

bool VertexBufferObject::updateVertices(const Vertex* vertices, size_t size)
{
    numVertices_ = size;

    // check size
    size_t bufferSize = sizeof(Vertex) * size;
    if ((vertexVBOBufferSize_ < bufferSize) || (vertexUsage_ != GL_DYNAMIC_DRAW)) {
        // resize
#if 0
        vertexVBOBufferSize_ = 1.5 * bufferSize;
        vertexUsage_ = GL_DYNAMIC_DRAW;
        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO_);
        glBufferData(GL_ARRAY_BUFFER, vertexVBOBufferSize_, vertices, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
        vertexVBOBufferSize_ = 1.5 * bufferSize;
        vertexUsage_ = GL_DYNAMIC_DRAW;
        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO_);
        glBufferData(GL_ARRAY_BUFFER, vertexVBOBufferSize_, nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * size, vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
        return true;
    } else {
        return updateVertices(vertexVBO_, vertices, size);
    }
}

bool VertexBufferObject::updateIndices(const GLuint* indices, size_t size)
{
    numIndices_ = size;

    // check size
    size_t bufferSize = sizeof(GLuint) * size;
    if ((indexVBOBufferSize_ < bufferSize) || (indexUsage_ != GL_DYNAMIC_DRAW)) {
        // resize
#if 0
        indexVBOBufferSize_ = 1.5 * bufferSize;
        indexUsage_ = GL_DYNAMIC_DRAW;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexVBOBufferSize_, indices, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#else
        indexVBOBufferSize_ = 1.5 * bufferSize;
        indexUsage_ = GL_DYNAMIC_DRAW;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexVBOBufferSize_, nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint) * size, indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
        return true;
    } else {
        return updateIndices(indexVBO_, indices, size);
    }
}

} /* namespace ptgl */
