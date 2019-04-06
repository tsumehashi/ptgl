#ifndef PTGL_CORE_VERTEXBUFFEROBJECT_H_
#define PTGL_CORE_VERTEXBUFFEROBJECT_H_

#include "GLPath.h"
#include "Vertex.h"

namespace ptgl {

class VBO {
public:
    VBO();
    virtual ~VBO();

    static bool loadVertex(GLuint& vbo, const std::vector<GLfloat> vertex, GLenum usage = GL_STATIC_DRAW);
    static bool loadVertex(GLuint& vbo, const GLfloat* vertex, size_t size, GLenum usage = GL_STATIC_DRAW);
    static bool loadIndex(GLuint& vbo, const std::vector<GLuint> indices, GLenum usage = GL_STATIC_DRAW);
    static bool loadIndex(GLuint& vbo, const GLuint* indices, size_t size, GLenum usage = GL_STATIC_DRAW);

    GLuint vertexVBO() const { return vertexVBO_; }
    GLuint indexVBO() const { return indexVBO_; }
    GLenum vertexUsage() const { return vertexUsage_; }
    GLenum indexUsage() const { return indexUsage_; }

//    bool valid() const { return (vertexVBO_ != 0) && (indexVBO_ != 0); }

    bool loadVertex(const std::vector<GLfloat> vertex, GLenum usage = GL_STATIC_DRAW);
    bool loadVertex(const GLfloat* vertex, size_t size, GLenum usage = GL_STATIC_DRAW);
    bool loadIndex(const std::vector<GLuint> indices, GLenum usage = GL_STATIC_DRAW);
    bool loadIndex(const GLuint* indices, size_t size, GLenum usage = GL_STATIC_DRAW);

private:
    GLuint vertexVBO_ = 0;
    GLuint indexVBO_ = 0;
    GLenum vertexUsage_ = GL_STATIC_DRAW;
    GLenum indexUsage_ = GL_STATIC_DRAW;
};

class VertexBufferObject {
public:
    VertexBufferObject();
    virtual ~VertexBufferObject();

    static bool loadVertices(GLuint& vbo, const Vertex* vertices, size_t size, GLenum usage = GL_STATIC_DRAW);
    static bool loadIndices(GLuint& vbo, const GLuint* indices, size_t size, GLenum usage = GL_STATIC_DRAW);

    bool loadVertices(const Vertex* vertices, size_t size, GLenum usage = GL_STATIC_DRAW);
    bool loadIndices(const GLuint* indices, size_t size, GLenum usage = GL_STATIC_DRAW);

    static bool updateVertices(GLuint vbo, const Vertex* vertices, size_t size);
    static bool updateIndices(GLuint vbo, const GLuint* indices, size_t size);

    bool updateVertices(const Vertex* vertices, size_t size);
    bool updateIndices(const GLuint* indices, size_t size);

    bool isValid() const;

    GLuint vertexVBO() const { return vertexVBO_; }
    GLuint indexVBO() const { return indexVBO_; }

    GLenum vertexUsage() const { return vertexUsage_; }
    GLenum indexUsage() const { return indexUsage_; }

    GLuint numVertices() const { return numVertices_; }
    GLuint numIndices() const { return numIndices_; }

private:
    GLuint vertexVBO_ = 0;
    GLuint indexVBO_ = 0;
    GLenum vertexUsage_ = GL_STATIC_DRAW;
    GLenum indexUsage_ = GL_STATIC_DRAW;
    GLuint numVertices_ = 0;
    GLuint numIndices_ = 0;
    size_t vertexVBOBufferSize_ = 0;
    size_t indexVBOBufferSize_ = 0;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_VERTEXBUFFEROBJECT_H_ */
