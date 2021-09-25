#include "Renderer3D.h"
#include <iostream>
#include "ptgl/Util/MathUtil.h"
#include "GraphicsView.h"
#include "Camera.h"
#include "DefaultShaderSource.h"
#include "PrimitiveShapeVertex.h"

namespace ptgl {

Renderer3D::Renderer3D(GraphicsView* view) :
    graphicsView_(view)
{

#if 0
    std::cout << "defaultVertexShaderSource = \n" << DefaultShaderSource::DefaultVertexShaderSource << std::endl;
    std::cout << "defaultFragmentShaderSource = \n" << DefaultShaderSource::DefaultFragmentShaderSource << std::endl;
#endif
}

Renderer3D::~Renderer3D()
{

}

void Renderer3D::initializeConfiguration()
{
    // load default shader
    if (!defaultShaderProgram_) {
        defaultShaderProgram_ = std::make_shared<ptgl::ShaderProgram>();

        ptgl::ShaderList shaders;
        shaders.push_back(ptgl::Shader::loadFromSource(ptgl::Shader::VertexShader, DefaultShaderSource::DefaultVertexShaderSource));
        shaders.push_back(ptgl::Shader::loadFromSource(ptgl::Shader::FragmentShader, DefaultShaderSource::DefaultFragmentShaderSource));

        defaultShaderProgram_->linkShaders(shaders);
    }

    shaderProgram_ = defaultShaderProgram_;

#if 0
    if (defaultShaderProgram_ && defaultShaderProgram_->valid()) {
        std::cout << "load defaultShaderProgram success" << std::endl;
    } else {
        std::cout << "load defaultShaderProgram failed" << std::endl;
    }
#endif

    // set Initial SheaderParameter
    shaderProgram_->bind();
    updateShaderParameterLocation();
    shaderProgram_->setParameter("lightEffectRate", 1.0);

    shaderProgram_->unbind();

    initializePrimitiveShapes();
}

// camera
int Renderer3D::windowWidth() const
{
    return graphicsView_->width();
}

int Renderer3D::windowHeight() const
{
    return graphicsView_->height();
}

CameraPtr Renderer3D::camera() const
{
    return graphicsView_->camera();
}

// shader
void Renderer3D::setDefaultShaderProgram(ShaderProgramPtr shaderProgram)
{
    if (shaderProgram && shaderProgram->valid()) {
        if (defaultShaderProgram_ && defaultShaderProgram_->binded()) {
            defaultShaderProgram_->unbind();
        }
        defaultShaderProgram_ = shaderProgram;

        setShaderProgram(defaultShaderProgram_);
    }
}

void Renderer3D::restoreDefaultShaderProgram()
{
    setShaderProgram(defaultShaderProgram_);
}

void Renderer3D::setShaderProgram(ShaderProgramPtr shaderProgram)
{
    if (shaderProgram && shaderProgram->valid()) {
        if (shaderProgram_ && shaderProgram_->binded()) {
            shaderProgram_->unbind();
        }
        if (forceUseShaderProgram_) {
            shaderProgram_ = forceUseShaderProgram_;
        } else {
            shaderProgram_ = shaderProgram;
        }

        // bind
        if (renderState() != RenderNoneState) {
            shaderProgram_->bind();

            // get attribute/uniform location
//            updateShaderParameterLocation();
        }
    }
}

void Renderer3D::setForceUseShaderProgram(ShaderProgramPtr shaderProgram)
{
    forceUseShaderProgram_ = shaderProgram;

    if (forceUseShaderProgram_) {
        setShaderProgram(forceUseShaderProgram_);
    } else {
        // if shaderProgram == null
        restoreDefaultShaderProgram();
    }
}

void Renderer3D::setEffectLight(double e)
{
    lightEffectRate_ = e;
    shaderProgram_->setParameter(unifLightEffectRateLocation_, e);
}

void Renderer3D::setDepthTest(bool on)
{
    if (on) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer3D::setPointSize(double size)
{
    pointSize_ = size;
//    glPointSize(pointSize_);
//    shaderProgram()->setParameter(unifPointSizeLocation_, pointSize_);
}

void Renderer3D::setLineWidth(double width)
{
    lineWidth_ = width;
    glLineWidth(lineWidth_);
}

void Renderer3D::setColor(const std::array<double, 3>& rgb)
{
    setColor(rgb[0], rgb[1], rgb[2]);
}

void Renderer3D::setColor(const std::array<double, 4>& rgba)
{
    setColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void Renderer3D::setColor(double r, double g, double b, double a)
{
    color_ = {{r, g, b, a}};
}

void Renderer3D::setTextColor(double r, double g, double b)
{
    textColor_ = {{r, g, b}};
}

void Renderer3D::getColor(double& r, double& g, double& b)
{
    r = color_[0], g = color_[1], b = color_[2];
}

void Renderer3D::getColor(double& r, double& g, double& b, double& a)
{
    r = color_[0], g = color_[1], b = color_[2], a = color_[3];
}

void Renderer3D::getTextColor(double& r, double& g, double& b)
{
    r = textColor_[0], g = textColor_[1], b = textColor_[2];
}

// draw functions
void Renderer3D::drawPoint(const double pos[3])
{
    drawPoints(pos, 1);
}

void Renderer3D::drawLine(const double pos1[3], const double pos2[3])
{
    drawLine(pos1[0], pos1[1], pos1[2], pos2[0], pos2[1], pos2[2]);
}

void Renderer3D::drawBox(const double pos[3], const double R[9], const double sides[3])
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(sides[0], sides[1], sides[2]);

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

       drawVertexBufferObject(primitiveBoxVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
}

void Renderer3D::drawSphere(const double pos[3], const double R[9], double r)
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(r);

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);
    drawVertexBufferObject(primitiveSphereVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
}

void Renderer3D::drawCylinder(const double pos[3], const double R[9], double length, double radius, bool cap)
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(radius, radius, length);

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

    if (cap) {
        drawVertexBufferObject(primitiveCylinderVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
    } else {
        drawVertexBufferObject(primitiveCylinderSideVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
    }
}

void Renderer3D::drawCapsule(const double pos[3], const double R[9], double length, double radius)
{
    // draw capsule side
    drawCylinder(pos, R, length, radius, false);

    // draw upper sphere
    auto drawHarfSphere = [&](VertexBufferObject& vbo, double dz) {
        Eigen::Affine3d t;
        t = Eigen::Translation3d(0.0, 0.0, dz) * Eigen::Scaling(radius);
        Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * t;

        updateModelMatrixParameter(affine);

        drawVertexBufferObject(vbo, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
    };

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);

    drawHarfSphere(primitiveUpperSphereVBO_, length/2);
    drawHarfSphere(primitiveLowerSphereVBO_, -length/2);
}

void Renderer3D::drawCone(const double pos[3], const double R[9], double length, double radius)
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(radius,radius,length);

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

       drawVertexBufferObject(primitiveConeVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
}

void Renderer3D::drawRing(const double pos[3], const double R[9], double length, double outer_radius, double inner_radius)
{
    (void)pos;
    (void)R;
    (void)length;
    (void)outer_radius;
    (void)inner_radius;
}

void Renderer3D::drawCircle(const double pos[3], const double R[9], double r)
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(r,r,1.0);

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

       drawVertexBufferObject(primitiveCircleVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
}

void Renderer3D::drawRingCircle(const double pos[3], const double R[9], double outer_radius, double inner_radius)
{
    ptgl::VertexSet vertexSet = ptgl::PrimitiveShapeVertex::generateRingCircle(outer_radius, inner_radius);
    drawVertex(pos, R, vertexSet.vertices, vertexSet.indices);
}

void Renderer3D::drawRect(const double pos[3], const double R[9], double xw, double yh)
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(xw,yh,1.0);

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

       drawVertexBufferObject(primitiveRectVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
}

void Renderer3D::drawArrow(const double pos1[3], const double pos2[3], double r)
{
    (void)pos1;
    (void)pos2;
    (void)r;
}

void Renderer3D::drawArrowCone(const double pos1[3], const double pos2[3], double r)
{
    (void)pos1;
    (void)pos2;
    (void)r;
}

void Renderer3D::drawAxis(const double pos[3], const double R[9], double length)
{
    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R) * Eigen::Scaling(length);

    shaderProgram()->setParameter(unifModelMatrixLocation_, affine.matrix());
    shaderProgram()->setParameter(unifLightEffectRateLocation_, 0.0);    // disable light

    // draw x
    shaderProgram()->setParameter(unifColorLocation_, 1,0,0,1);
    drawVertexBufferObject(primitiveAxisVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 0, 2);
    // draw y
    shaderProgram()->setParameter(unifColorLocation_, 0,1,0,1);
    drawVertexBufferObject(primitiveAxisVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 2, 2);
    // draw z
    shaderProgram()->setParameter(unifColorLocation_, 0,0,1,1);
    drawVertexBufferObject(primitiveAxisVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 4, 2);

    shaderProgram()->setParameter(unifLightEffectRateLocation_, 1.0);    // enable light

}

void Renderer3D::drawPoints(const double* p, int numpoints)
{
    generalVertexBuffer_.clear();
    for (int i = 0; i < numpoints; ++i) {
        generalVertexBuffer_.push_back({{(GLfloat)p[3*i], (GLfloat)p[3*i+1], (GLfloat)p[3*i+2]}});
    }
//    updateVertexBufferObject(generalVBO_.vertexVBO(), &generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());
    updateGeneralVBO(&generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    shaderProgram()->setParameter(unifModelMatrixLocation_, tf_.transformation().matrix());
    shaderProgram()->setParameter(unifLightEffectRateLocation_, 0.0);    // disable light
    shaderProgram()->setParameter(unifPointSizeLocation_, pointSize_);    // set point size

    drawVertexBufferObject(generalVBO_.vertexVBO(), attrVertexLocation_, GL_POINTS, 0, generalVertexBuffer_.size());

    shaderProgram()->setParameter(unifLightEffectRateLocation_, 1.0);    // enable light
    shaderProgram()->setParameter(unifPointSizeLocation_, 1.0);            // disable point size
}

void Renderer3D::drawLines(const double* lines, int numlines, GLenum mode)
{
    generalVertexBuffer_.clear();
    for (int i = 0; i < numlines; ++i) {
        generalVertexBuffer_.push_back({{(GLfloat)lines[3*i], (GLfloat)lines[3*i+1], (GLfloat)lines[3*i+2]}});
    }
//    updateVertexBufferObject(generalVBO_.vertexVBO(), &generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());
    updateGeneralVBO(&generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    shaderProgram()->setParameter(unifModelMatrixLocation_, tf_.transformation().matrix());
    shaderProgram()->setParameter(unifLightEffectRateLocation_, 0.0);    // disable light
    drawVertexBufferObject(generalVBO_.vertexVBO(), attrVertexLocation_, mode, 0, generalVertexBuffer_.size());
    shaderProgram()->setParameter(unifLightEffectRateLocation_, 1.0);    // enable light
}

void Renderer3D::drawLines(const double* lines, int numlines)
{
    drawLines(lines, numlines, GL_LINES);
}

void Renderer3D::drawLineStrip(const double* lines, int numlines)
{
    drawLines(lines, numlines, GL_LINE_STRIP);
}

void Renderer3D::drawLineLoop(const double* lines, int numlines)
{
    drawLines(lines, numlines, GL_LINE_LOOP);
}

void Renderer3D::drawPoint(double x, double y, double z)
{
    double p[3] = {x, y, z};
    drawPoints(p, 1);
}

void Renderer3D::drawLine(double x1, double y1, double z1, double x2, double y2, double z2)
{
    double p[6] = {x1, y1, z1, x2, y2, z2};
    drawLines(p, 2);
}

void Renderer3D::beginDrawPoints()
{
    generalVertexBuffer_.clear();
}

void Renderer3D::addDrawPoints(const double* p, int numpoints)
{
    for (int i = 0; i < numpoints; ++i) {
        generalVertexBuffer_.push_back({{(GLfloat)p[3*i], (GLfloat)p[3*i+1], (GLfloat)p[3*i+2]}});
    }
}

void Renderer3D::addDrawPoints(double x, double y, double z)
{
    generalVertexBuffer_.push_back({{(GLfloat)x, (GLfloat)y, (GLfloat)z}});
}

void Renderer3D::endDrawPoinhts()
{
    if (generalVertexBuffer_.empty()) {
        return;
    }

    updateGeneralVBO(&generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());

    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    shaderProgram()->setParameter(unifModelMatrixLocation_, tf_.transformation().matrix());
    shaderProgram()->setParameter(unifLightEffectRateLocation_, 0.0);    // disable light
    shaderProgram()->setParameter(unifPointSizeLocation_, pointSize_);    // set point size

    drawVertexBufferObject(generalVBO_.vertexVBO(), attrVertexLocation_, GL_POINTS, 0, generalVertexBuffer_.size());

    shaderProgram()->setParameter(unifLightEffectRateLocation_, 1.0);    // enable light
    shaderProgram()->setParameter(unifPointSizeLocation_, 1.0);            // disable point size
}

void Renderer3D::drawWorldGrid()
{
    shaderProgram()->setParameter(unifModelMatrixLocation_, Eigen::Matrix4d(Eigen::Matrix4d::Identity()));
    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);

    shaderProgram()->setParameter(unifLightEffectRateLocation_, 0.0);    // disable light

       drawVertexBufferObject(primitiveWorldGridVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 0, primitiveWorldGridVBO_.numVertices()/3);

    shaderProgram()->setParameter(unifLightEffectRateLocation_, 1.0);    // enable light
}

void Renderer3D::drawWorldAxis(double length)
{
    Eigen::Matrix4d s = Eigen::Matrix4d::Identity();
    s(0,0) = length; s(1,1) = length; s(2,2) = length;
    shaderProgram()->setParameter(unifModelMatrixLocation_, s);
    shaderProgram()->setParameter(unifLightEffectRateLocation_, 0.0);    // disable light

    // draw x
    shaderProgram()->setParameter(unifColorLocation_, 1,0,0,1);
    drawVertexBufferObject(primitiveAxisVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 0, 2);
    // draw y
    shaderProgram()->setParameter(unifColorLocation_, 0,1,0,1);
    drawVertexBufferObject(primitiveAxisVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 2, 2);
    // draw z
    shaderProgram()->setParameter(unifColorLocation_, 0,0,1,1);
    drawVertexBufferObject(primitiveAxisVBO_.vertexVBO(), attrVertexLocation_, GL_LINES, 4, 2);

    shaderProgram()->setParameter(unifLightEffectRateLocation_, 1.0);    // enable light
}

void Renderer3D::drawVertexBufferObject(GLuint vbo, GLint attrVertex, GLenum mode, int offset, int n)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(attrVertex);
    glVertexAttribPointer(attrVertex, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(mode, offset, n);
    glDisableVertexAttribArray(attrVertex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer3D::drawVertexBufferObject(const ptgl::VertexBufferObject& vbo, GLint attrVertex, GLenum mode)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo.vertexVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.indexVBO());
    glEnableVertexAttribArray(attrVertex);
    glVertexAttribPointer(attrVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ptgl::Vertex), (const GLvoid*)0 );
    glDrawElements( mode, vbo.numIndices(), GL_UNSIGNED_INT, 0 );
    glDisableVertexAttribArray(attrVertex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer3D::drawVertexBufferObject(const ptgl::VertexBufferObject& vbo, GLint attrVertex, GLint attrNormal, GLenum mode)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo.vertexVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.indexVBO());
    glEnableVertexAttribArray(attrVertex);
    glEnableVertexAttribArray(attrNormal);
    glVertexAttribPointer(attrVertex, 3, GL_FLOAT, GL_FALSE, sizeof(ptgl::Vertex), (const GLvoid*)0 );
    glVertexAttribPointer(attrNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
    glDrawElements( mode, vbo.numIndices(), GL_UNSIGNED_INT, 0 );
    glDisableVertexAttribArray(attrVertex);
    glDisableVertexAttribArray(attrNormal);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer3D::updateVertexBufferObject(GLuint vbo, const GLfloat* vertex, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * size, vertex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer3D::updateVertexBufferObject(GLuint vbo, const Vertex* vertex, size_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * size, vertex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer3D::updateIndexBufferObject(GLuint vbo, const GLuint* indices, size_t size)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint) * size, indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// draw vertices
void Renderer3D::registerVertices(const std::string& name, const VertexList& vertices, bool override)
{
    IndexList indices;
    for (size_t i = 0; i < vertices.size(); ++i) {
        indices.push_back(i);
    }
    registerVertices(name, vertices, indices, override);
}

void Renderer3D::registerVertices(const std::string& name, const VertexList& vertices, const IndexList& indices, bool override)
{
    auto fitr = registeredVerticesVBOInfoMap_.find(name);
    if (fitr != registeredVerticesVBOInfoMap_.end()) {
        // find
        if (override) {
            // update
            auto& vboInfo = fitr->second;
//            auto& vbo = vboInfo.vbo;
            vboInfo.numVertices = vertices.size();
//            updateVertexBufferObject(vbo.vertexVBO(), vertices.data(), vertices.size());
        }

    } else {
        // not find
        VBOInfo vboInfo;
        vboInfo.numVertices = vertices.size();
        vboInfo.numIndices = indices.size();
        vboInfo.vbo.loadVertices(vertices.data(), vertices.size(), GL_DYNAMIC_DRAW);
        vboInfo.vbo.loadIndices(indices.data(), indices.size(), GL_DYNAMIC_DRAW);
        registeredVerticesVBOInfoMap_.insert({name, vboInfo});
    }
}

bool Renderer3D::isRegisteredVertices(const std::string& name)
{
    auto fitr = registeredVerticesVBOInfoMap_.find(name);
    return fitr != registeredVerticesVBOInfoMap_.end();
}

void Renderer3D::drawRegisteredVertices(const std::string& name)
{
    auto fitr = registeredVerticesVBOInfoMap_.find(name);
    if (fitr != registeredVerticesVBOInfoMap_.end()) {
        // find
        auto& vboInfo = fitr->second;
        auto& vbo = vboInfo.vbo;
        Eigen::Affine3d affine = tf_.transformation();

        shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
        updateModelMatrixParameter(affine);

           drawVertexBufferObject(vbo, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
    }
}

void Renderer3D::drawVertex(const VertexList& vertices, const IndexList& indices)
{
    vertexVBO_.updateVertices(vertices.data(), vertices.size());
    vertexVBO_.updateIndices(indices.data(), indices.size());

    Eigen::Affine3d affine = tf_.transformation();
    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

    drawVertexBufferObject(vertexVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);
}

void Renderer3D::drawVertex(const double pos[3], const double R[9], const VertexList& vertices, const IndexList& indices)
{
    vertexVBO_.updateVertices(vertices.data(), vertices.size());
    vertexVBO_.updateIndices(indices.data(), indices.size());

    Eigen::Affine3d affine = tf_.transformation() * ptgl::transformation(pos, R);
    shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
    updateModelMatrixParameter(affine);

    drawVertexBufferObject(vertexVBO_, attrVertexLocation_, attrNormalLocation_, GL_TRIANGLES);

}

void Renderer3D::updateGeneralVBO(const GLfloat* vertex, size_t size)
{
    size_t bufferSize = sizeof(GLfloat) * size;
    if (generalVBOBufferSize_ < bufferSize) {
        // resize
        generalVBOBufferSize_ = 1.5*bufferSize;
        glBindBuffer(GL_ARRAY_BUFFER, generalVBO_.vertexVBO());
        glBufferData(GL_ARRAY_BUFFER, generalVBOBufferSize_, vertex, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } else {
        updateVertexBufferObject(generalVBO_.vertexVBO(), vertex, size);
    }
}

Eigen::Matrix3d Renderer3D::calcNormalMatrix(const Eigen::Affine3d& modelAffine)
{
       Eigen::Matrix3d modelViewIT = modelAffine.matrix().block<3,3>(0,0);
    modelViewIT.transposeInPlace();
    return Eigen::Matrix3d(modelViewIT.inverse());
}

void Renderer3D::initializePrimitiveShapes()
{
    // General VBO
    generalVBO_.loadVertices(nullptr, generalVBOBufferSize_, GL_DYNAMIC_DRAW);
    // don't use Indices

    // Vertex
    vertexVBO_.loadVertices(nullptr, 128, GL_DYNAMIC_DRAW);
    vertexVBO_.loadIndices(nullptr, 128, GL_DYNAMIC_DRAW);

    // Box
    primitiveBoxVBO_.loadVertices(ptgl::PrimitiveShapeVertex::BoxVertices.data(), ptgl::PrimitiveShapeVertex::BoxVertices.size());
    primitiveBoxVBO_.loadIndices(ptgl::PrimitiveShapeVertex::BoxIndices.data(), ptgl::PrimitiveShapeVertex::BoxIndices.size());

    // Sphere
    primitiveSphereVBO_.loadVertices(ptgl::PrimitiveShapeVertex::SphereVertices.data(), ptgl::PrimitiveShapeVertex::SphereVertices.size());
    primitiveSphereVBO_.loadIndices(ptgl::PrimitiveShapeVertex::SphereIndices.data(), ptgl::PrimitiveShapeVertex::SphereIndices.size());

    // Cylinder
    primitiveCylinderVBO_.loadVertices(ptgl::PrimitiveShapeVertex::CylinderVertices.data(), ptgl::PrimitiveShapeVertex::CylinderVertices.size());
    primitiveCylinderVBO_.loadIndices(ptgl::PrimitiveShapeVertex::CylinderIndices.data(), ptgl::PrimitiveShapeVertex::CylinderIndices.size());

    // CylinderSide
    primitiveCylinderSideVBO_.loadVertices(ptgl::PrimitiveShapeVertex::CylinderSideVertices.data(), ptgl::PrimitiveShapeVertex::CylinderSideVertices.size());
    primitiveCylinderSideVBO_.loadIndices(ptgl::PrimitiveShapeVertex::CylinderSideIndices.data(), ptgl::PrimitiveShapeVertex::CylinderSideIndices.size());

    // UpperSphere
    primitiveUpperSphereVBO_.loadVertices(ptgl::PrimitiveShapeVertex::UpperSphereVertices.data(), ptgl::PrimitiveShapeVertex::UpperSphereVertices.size());
    primitiveUpperSphereVBO_.loadIndices(ptgl::PrimitiveShapeVertex::UpperSphereIndices.data(), ptgl::PrimitiveShapeVertex::UpperSphereIndices.size());

    // LowerSphere
    primitiveLowerSphereVBO_.loadVertices(ptgl::PrimitiveShapeVertex::LowerSphereVertices.data(), ptgl::PrimitiveShapeVertex::LowerSphereVertices.size());
    primitiveLowerSphereVBO_.loadIndices(ptgl::PrimitiveShapeVertex::LowerSphereIndices.data(), ptgl::PrimitiveShapeVertex::LowerSphereIndices.size());

    // Cone
    primitiveConeVBO_.loadVertices(ptgl::PrimitiveShapeVertex::ConeVertices.data(), ptgl::PrimitiveShapeVertex::ConeVertices.size());
    primitiveConeVBO_.loadIndices(ptgl::PrimitiveShapeVertex::ConeIndices.data(), ptgl::PrimitiveShapeVertex::ConeIndices.size());

    // Ring

    // Circle
    primitiveCircleVBO_.loadVertices(ptgl::PrimitiveShapeVertex::CircleVertices.data(), ptgl::PrimitiveShapeVertex::CircleVertices.size());
    primitiveCircleVBO_.loadIndices(ptgl::PrimitiveShapeVertex::CircleIndices.data(), ptgl::PrimitiveShapeVertex::CircleIndices.size());

    // RingCircle

    // Rect
    primitiveRectVBO_.loadVertices(ptgl::PrimitiveShapeVertex::RectVertices.data(), ptgl::PrimitiveShapeVertex::RectVertices.size());
    primitiveRectVBO_.loadIndices(ptgl::PrimitiveShapeVertex::RectIndices.data(), ptgl::PrimitiveShapeVertex::RectIndices.size());

    // Arrow

    // ArrowCone

    // Axis
    {
        generalVertexBuffer_.clear();
        // x
        generalVertexBuffer_.push_back({{0.f,0.f,0.f}});
        generalVertexBuffer_.push_back({{1.f,0.f,0.f}});
        // y
        generalVertexBuffer_.push_back({{0.f,0.f,0.f}});
        generalVertexBuffer_.push_back({{0.f,1.f,0.f}});
        // z
        generalVertexBuffer_.push_back({{0.f,0.f,0.f}});
        generalVertexBuffer_.push_back({{0.f,0.f,1.f}});
        primitiveAxisVBO_.loadVertices(nullptr, 3*generalVertexBuffer_.size());
        updateVertexBufferObject(primitiveAxisVBO_.vertexVBO(), &generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());
    }

    // WolrdGrid
    {
        generalVertexBuffer_.clear();

        int n = 10;
        double dg = 1;
        // set  x
        for (int i = -n; i < n+1; ++i) {
            GLfloat x = dg * i;
            GLfloat y0 = -dg * n;
            GLfloat y1 = dg * n;
            generalVertexBuffer_.push_back({{x, y0, 0.f}});
            generalVertexBuffer_.push_back({{x, y1, 0.f}});
        }

        // set y
        for (int i = -n; i < n+1; ++i) {
            GLfloat y = dg * i;
            GLfloat x0 = -dg * n;
            GLfloat x1 = dg * n;
            generalVertexBuffer_.push_back({{x0, y, 0.f}});
            generalVertexBuffer_.push_back({{x1, y, 0.f}});
        }

        primitiveWorldGridVBO_.loadVertices(nullptr, 3*generalVertexBuffer_.size());
        updateVertexBufferObject(primitiveWorldGridVBO_.vertexVBO(), &generalVertexBuffer_[0][0], 3*generalVertexBuffer_.size());
    }
}

void Renderer3D::updateShaderParameterLocation()
{
    // get attribute/uniform location
    attrVertexLocation_ = shaderProgram()->attribute("position");
    attrNormalLocation_ = shaderProgram()->attribute("normal");
    unifColorLocation_ = shaderProgram()->uniform("color");
    unifModelMatrixLocation_ = shaderProgram()->uniform("modelMatrix");
    unifLightEffectRateLocation_ = shaderProgram()->uniform("lightEffectRate");
    unifNormalMatrixLocation_ = shaderProgram()->uniform("normalMatrix");
    unifPointSizeLocation_ = shaderProgram()->uniform("pointSize");
}

// for GraphicsView
void Renderer3D::beginRender(RenderState state)
{
    setRenderState(state);

    tf_.clear();
    restoreDefaultShaderProgram();    // set DefaultShader and bind

//    shaderProgram()->bind();

    // get attribute/uniform location
    updateShaderParameterLocation();

    // set initial parameter
#if 0
    shaderProgram()->setParameter("projectionMatrix", camera()->projection());
    shaderProgram()->setParameter("viewMatrix", camera()->modelview());
#else
    shaderProgram()->setParameter("projectionMatrix", Eigen::Matrix4d(camera()->projection()));
    shaderProgram()->setParameter("viewMatrix", Eigen::Matrix4d(camera()->modelview()));
#endif
    shaderProgram()->setParameter("modelMatrix", Eigen::Matrix4d(Eigen::Matrix4d::Identity()));
    shaderProgram()->setParameter("normalMatrix", Eigen::Matrix3d(Eigen::Matrix3d::Identity()));
    shaderProgram()->setParameter("cameraPosition", camera()->position());
    shaderProgram()->setParameter("color", Eigen::Vector4d(1,1,1,1));
    shaderProgram()->setParameter("lightDirection", Eigen::Vector3d(1,1,1));
    shaderProgram()->setParameter("lightEffectRate", 1.0);
    shaderProgram()->setParameter("pointSize", 1.0);

    // todo

}

void Renderer3D::endRender(RenderState state)
{
    (void)state;

    // unbind shader program
    if (shaderProgram() && shaderProgram()->binded()) {
        shaderProgram()->unbind();
    }
    // todo
    setRenderState(RenderNoneState);
}

void Renderer3D::setProjectionViewMatrix(const Eigen::Matrix4d& m)
{
    projectionViewMatrix_ = m;
    inverseProjectionViewMatrix_ = m.inverse();
}


} /* namespace ptgl */
