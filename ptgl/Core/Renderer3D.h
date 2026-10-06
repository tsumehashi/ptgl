#ifndef PTGL_CORE_RENDERER3D_H_
#define PTGL_CORE_RENDERER3D_H_

#include <array>
#include <list>
#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include "TransformStack.h"
#include "GLPath.h"
#include "ShaderProgram.h"
#include "VertexBufferObject.h"
#include "Material.h"

namespace ptgl {

class GraphicsView;
class Camera;
typedef std::shared_ptr<Camera> CameraPtr;

class Renderer3D {
    friend class GraphicsView;
public:

    enum RenderState {
        RenderNoneState,
        RenderSceneState,
        RenderOverlayState,
        RenderPickingState,
        RenderPickingOverlayState,
        RenderShadowState,
        RenderBoundsState,
        RenderTransparentState,
        RenderOcclusionState,
        RenderEdgeState, // Nearest visible surfaces, including translucent items.
    };

    enum CoordinatePlane {
        XYPlane,
        YZPlane,
        XZPlane,
    };

    enum DrawMode {
        Solid,
        Wire
    };

    Renderer3D(GraphicsView* view);
    virtual ~Renderer3D();

    virtual void initializeConfiguration();
    virtual void finalizeConfiguration(); // Requires the owning GL context.

    RenderState renderState() const { return renderState_; }
#if 0
    const Eigen::Matrix4d& projectionViewMatrix() const { return projectionViewMatrix_; }
    const Eigen::Matrix4d& inverseProjectionViewMatrix() const { return inverseProjectionViewMatrix_; }
#else
    const Eigen::Matrix<double, 4, 4, Eigen::DontAlign>& projectionViewMatrix() const { return projectionViewMatrix_; }
    const Eigen::Matrix<double, 4, 4, Eigen::DontAlign>& inverseProjectionViewMatrix() const { return inverseProjectionViewMatrix_; }
#endif

    // camera
    int windowWidth() const;
    int windowHeight() const;
    CameraPtr camera() const;

    // shader
    void setDefaultShaderProgram(ShaderProgramPtr shaderProgram);
    void restoreDefaultShaderProgram();
    void setShaderProgram(ShaderProgramPtr shaderProgram);
    ShaderProgramPtr shaderProgram() { return shaderProgram_; }

    void setForceUseShaderProgram(ShaderProgramPtr shaderProgram);
    bool hasForcedShaderProgram() const { return bool(forceUseShaderProgram_); }
    // Geometry-only bounds collection: callbacks must not advance animation.
    void beginBoundsCollection() { sceneBounds_.setEmpty(); collectingBounds_ = true; }
    Eigen::AlignedBox3d endBoundsCollection() { collectingBounds_ = false; return sceneBounds_; }

    // Light
    void setEffectLight(double e);

    void setDepthTest(bool on);

    void setPointSize(double size);
    void setLineWidth(double width);

    void setColor(const std::array<double, 3>& rgb);
    void setColor(const std::array<double, 4>& rgba);
    void setColor(double r, double g, double b, double a = 1.0);
    void setTextColor(double r, double g, double b);

    void getColor(double& r, double& g, double& b);
    void getColor(double& r, double& g, double& b, double& a);
    void getTextColor(double& r, double& g, double& b);

    // Set from rendering callbacks, like setColor(). Reset at each render pass.
    void setMaterial(const Material& material);
    const Material& material() const { return material_; }

    // draw functions
    using vec3d = std::array<double, 3>;
    using vec9d = std::array<double, 9>;

    const double* getData(const double* x) const { return x; }
    template <typename T> auto getData(const T& x) const -> decltype(static_cast<const double*>(std::data(x))) { return std::data(x); }

    template <size_t N, typename T>
    auto getVec(const T& x) {
        if constexpr (std::is_pointer_v<T>) {
            if constexpr (std::is_same_v<double*, T>) {
                return x;
            } else {
                std::array<double, N> a;
                for (size_t i = 0; i < N; i++) {
                    a[i] = x[i];
                }
                return a;
            }
        } else {
            if constexpr (std::is_same_v<const double*, decltype(std::data(x))>)
            {
                return std::data(x);
            } else {
                const auto* p = std::data(x);
                std::array<double, N> a;
                for (size_t i = 0; i < N; i++) {
                    a[i] = p[i];
                }
                return a;
            }
        }
    }

    void drawPoint(const double pos[3]);
    template <typename T = vec3d>
    void drawPoint(const T& pos) { drawPoint(getData(getVec<3>(pos))); }

    void drawLine(const double pos1[3], const double pos2[3]);
    template <typename T = vec3d, typename U = vec3d>
    void drawLine(const T& pos1, const U& pos2) { drawLine(getData(getVec<3>(pos1)), getData(getVec<3>(pos2))); }

    void drawBox(const double pos[3], const double R[9], const double sides[3]);
    template <typename T = vec3d, typename U = vec9d, typename V = vec3d>
    void drawBox(const T& pos, const U& R, const V& sides) { drawBox(getData(getVec<3>(pos)), getData(getVec<9>(R)), getData(getVec<3>(sides))); }

    // Rounded primitives share all scene passes and cache up to 16 parameter
    // combinations per renderer. Geometry changes require edge-cache invalidation
    // when the view opts into static caching. Invalid dimensions/radii/segments
    // throw std::invalid_argument; see PrimitiveShapeVertex for radius limits.
    void drawRoundedBox(const double pos[3], const double R[9], const double sides[3], double radius, int segments = 6);
    template <typename T = vec3d, typename U = vec9d, typename V = vec3d>
    void drawRoundedBox(const T& pos, const U& R, const V& sides, double radius, int segments = 6) {
        drawRoundedBox(getData(getVec<3>(pos)), getData(getVec<9>(R)), getData(getVec<3>(sides)), radius, segments);
    }
    void drawRoundedCylinder(const double pos[3], const double R[9], double length, double radius, double filletRadius, int segments = 6);
    template <typename T = vec3d, typename U = vec9d>
    void drawRoundedCylinder(const T& pos, const U& R, double length, double radius, double filletRadius, int segments = 6) {
        drawRoundedCylinder(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, radius, filletRadius, segments);
    }
    void drawRoundedCone(const double pos[3], const double R[9], double length, double radius, double filletRadius, int segments = 6);
    template <typename T = vec3d, typename U = vec9d>
    void drawRoundedCone(const T& pos, const U& R, double length, double radius, double filletRadius, int segments = 6) {
        drawRoundedCone(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, radius, filletRadius, segments);
    }

    virtual void drawSphere(const double pos[3], const double R[9], double r);
    template <typename T = vec3d, typename U = vec9d>
    void drawSphere(const T& pos, const U& R, double r) { drawSphere(getData(getVec<3>(pos)), getData(getVec<9>(R)), r); }

    virtual void drawCylinder(const double pos[3], const double R[9], double length, double radius, bool cap = true);
    template <typename T = vec3d, typename U = vec9d>
    void drawCylinder(const T& pos, const U& R, double length, double radius, bool cap = true) { drawCylinder(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, radius, cap); }

    virtual void drawCapsule(const double pos[3], const double R[9], double length, double radius);
    template <typename T = vec3d, typename U = vec9d>
    void drawCapsule(const T& pos, const U& R, double length, double radius) { drawCapsule(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, radius); }

    void drawCone(const double pos[3], const double R[9], double length, double radius);
    template <typename T = vec3d, typename U = vec9d>
    void drawCone(const T& pos, const U& R, double length, double radius) { drawCone(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, radius); }

    void drawRing(const double pos[3], const double R[9], double length, double outer_radius, double inner_radius);
    template <typename T = vec3d, typename U = vec9d>
    void drawRing(const T& pos, const U& R, double length, double outer_radius, double inner_radius) { drawRing(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, outer_radius, inner_radius); }

    void drawCircle(const double pos[3], const double R[9], double r);
    template <typename T = vec3d, typename U = vec9d>
    void drawCircle(const T& pos, const U& R, double r) { drawCircle(getData(getVec<3>(pos)), getData(getVec<9>(R)), r); }

    void drawRingCircle(const double pos[3], const double R[9], double outer_radius, double inner_radius);
    template <typename T = vec3d, typename U = vec9d>
    void drawRingCircle(const T& pos, const U& R, double length, double outer_radius, double inner_radius) { drawRingCircle(getData(getVec<3>(pos)), getData(getVec<9>(R)), length, outer_radius, inner_radius); }

    void drawRect(const double pos[3], const double R[9], double xw, double yh);
    template <typename T = vec3d, typename U = vec9d>
    void drawRect(const T& pos, const U& R, double xw, double yh) { drawRect(getData(getVec<3>(pos)), getData(getVec<9>(R)), xw, yh); }

    void drawArrow(const double pos1[3], const double pos2[3], double r);
    template <typename T = vec3d, typename U = vec3d>
    void drawArrow(const T& pos1, const U& pos2, double r) { drawArrow(getData(getVec<3>(pos1)), getData(getVec<3>(pos2)), r); }

    void drawArrowCone(const double pos1[3], const double pos2[3], double r);
    template <typename T = vec3d, typename U = vec3d>
    void drawArrowCone(const T& pos1, const U& pos2, double r) { drawArrowCone(getData(getVec<3>(pos1)), getData(getVec<3>(pos2)), r); }

    void drawAxis(const double pos[3], const double R[9], double length);
    template <typename T = vec3d, typename U = vec9d>
    void drawAxis(const T& pos, const U& R, double length) { drawAxis(getData(getVec<3>(pos)), getData(getVec<9>(R)), length); }

//    void drawGrid(double w, int div);

    void drawPoints(const double* p, int numpoints);
    void drawLines(const double* lines, int numlines);
    void drawLineStrip(const double* lines, int numlines);
    void drawLineLoop(const double* lines, int numlines);

    void drawPoint(double x, double y, double z);
    void drawLine(double x1, double y1, double z1, double x2, double y2, double z2);
//    void drawRect(double x1, double y1, double x2, double y2);

    void beginDrawPoints();
    void addDrawPoints(const double* p, int numpoints);
    void addDrawPoints(double x, double y, double z);
    template <typename T = vec3d>
    void addDrawPoints(const T& pos1) { addDrawPoints(getData(getVec<3>(pos1)), 1); }
    void endDrawPoinhts();

    template <typename T>
    void drawPoints(const T& points) {
        beginDrawPoints();
        for (const auto& p : points) {
            addDrawPoints(p);
        }
        endDrawPoinhts();
    }

    void drawWorldGrid();
    void drawWorldAxis(double length = 10);

    // vertex (not use index)
    static void drawVertexBufferObject(GLuint vbo, GLint attrVertex, GLenum mode, int offset, int n);
    // vertex
    static void drawVertexBufferObject(const ptgl::VertexBufferObject& vbo, GLint attrVertex, GLenum mode);
    // vertex, normal
    static void drawVertexBufferObject(const ptgl::VertexBufferObject& vbo, GLint attrVertex, GLint attrNormal, GLenum mode);

    // vertex (not use index)
    static void updateVertexBufferObject(GLuint vbo, const GLfloat* vertex, size_t size);

    // vertex (not use index)
    static void updateVertexBufferObject(GLuint vbo, const Vertex* vertex, size_t size);

    // index
    static void updateIndexBufferObject(GLuint vbo, const GLuint* indices, size_t size);

    // draw vertices
    void registerVertices(const std::string& name, const VertexList& vertices, bool override = true);
    void registerVertices(const std::string& name, const VertexList& vertices, const IndexList& indices, bool override = true);
    bool isRegisteredVertices(const std::string& name);
    void drawRegisteredVertices(const std::string& name);

    // Optional preprocessed/explicit edges are drawn in CAD mode only.
    void drawMesh(const VertexSet& mesh);
    void registerMesh(const std::string& name, const VertexSet& mesh, bool override = false);

    void drawVertex(const VertexList& vertices, const IndexList& indices);
    void drawVertex(const double pos[3], const double R[9], const VertexList& vertices, const IndexList& indices);
    template <typename T = vec3d, typename U = vec9d>
    void drawVertex(const T& pos, const U& R, const VertexList& vertices, const IndexList& indices) { drawVertex(getData(getVec<3>(pos)), getData(getVec<9>(R)), vertices, indices); }

    // transform stack
    void pushMatrix() { tf_.push(); }
    void popMatrix() { tf_.pop(); }

    void identiry() { tf_.identiry(); }

    void scale(double s) { tf_.scale(s); }
    void scale(double sx, double sy, double sz) { tf_.scale(sx, sy, sz); }
    void scale(const Eigen::Vector3d& s) { tf_.scale(s); }

    void translate(double x, double y, double z) { tf_.translate(x, y, z); }
    void translate(const Eigen::Vector3d& t) { tf_.translate(t); }

    void rotate(const Eigen::Matrix3d& R) { tf_.rotate(R); }
    void rotate(double angle, const Eigen::Vector3d& axis) { tf_.rotate(angle, axis); }
    void rotateRpy(double r, double p, double y) { tf_.rotateRpy(r, p, y); }
    void rotateX(double deg) { tf_.rotateX(deg); }
    void rotateY(double deg) { tf_.rotateY(deg); }
    void rotateZ(double deg) { tf_.rotateZ(deg); }

    void transform(const Eigen::Affine3d& t) { tf_.transform(t); }
    void transform(const Eigen::Matrix4d& m) { tf_.transform(m); }
    void transform(const Eigen::Vector3d& p, const Eigen::Matrix3d& R) { tf_.transform(p, R); }

    // helper functions
    const double* p0() const { static const double p0_[] = {0,0,0}; return p0_; }
    const double* R0() const { static const double R0_[] = {1,0,0,0,1,0,0,0,1}; return R0_; }

    const double* v1() const { static const double v1_[] = {1,1,1}; return v1_; }

    const double* p(double x, double y, double z) { p_ = {{x, y, z}}; return p_.data();    }

    template <typename T>
    const double* p(const T* v) { return p((double)v[0], (double)v[1], (double)v[2]); }

    template <typename T>
    const double* R(const T* fR) { for (int i = 0; i < 9; ++i) R_[i] = fR[i]; return R_.data(); }

    template <typename T>
    const double* tR(const T* R) {
        R_[0] = R[0]; R_[3] = R[1]; R_[6] = R[2];
        R_[1] = R[3]; R_[4] = R[4]; R_[7] = R[5];
        R_[2] = R[6]; R_[5] = R[7]; R_[8] = R[8];
        return R_.data();
    }

    // value holder
    template <int N>
    const double* v(double x, double y, double z) {
        thread_local static std::array<double, 3> v_;
        v_ = {{x,y,z}};
        return v_.data();
    }

    template <int N>
    const double* v(double x) {
        return v<N>(x, x, x);
    }

    const GraphicsView* graphicsView() const { return graphicsView_; }

protected:

    void initializePrimitiveShapes();
    void drawLines(const double* lines, int numlines, GLenum mode);

    Eigen::Matrix3d calcNormalMatrix(const Eigen::Affine3d& modelAffine);
    void updateShaderParameterLocation();
    void updateGeneralVBO(const GLfloat* vertex, size_t size);

    bool isPickingRenderMode() {
        return renderState_ == RenderPickingState || renderState_ == RenderPickingOverlayState;
    }

    void updateModelMatrixParameter(const Eigen::Affine3d& affine) {
        modelTransform_ = affine;
        shaderProgram()->setParameter(unifModelMatrixLocation_, affine.matrix());
        if (!isPickingRenderMode() && renderState_ != RenderShadowState) {
            shaderProgram()->setParameter(unifNormalMatrixLocation_, calcNormalMatrix(affine));
        }
    }

    GraphicsView* graphicsView_ = nullptr;
    virtual void submitMesh(const VertexBufferObject& vbo, GLenum mode = GL_TRIANGLES);
    virtual bool acceptsFeatureEdges() const { return false; }
    virtual void submitFeatureEdges(std::shared_ptr<const VertexList>) {}
    bool collectingBounds_ = false;
    Eigen::AlignedBox3d sceneBounds_;
    Eigen::Affine3d modelTransform_ = Eigen::Affine3d::Identity();

    RenderState renderState_ = RenderNoneState;

    std::array<double, 4> color_{{1,1,1,1}};
    std::array<double, 3> textColor_{{1,1,1}};
    Material material_;

    double pointSize_ = 1;
    double lineWidth_ = 1;
    DrawMode drawMode_ = Solid;
    bool cullFace_ = true;

    std::array<double, 3> p_{{0,0,0}};
    std::array<double, 9> R_{{1,0,0,0,1,0,0,0,1}};

    TransformStack tf_;

    ShaderProgramPtr shaderProgram_;    // never null
    ShaderProgramPtr defaultShaderProgram_;
    ShaderProgramPtr forceUseShaderProgram_;    // do not use if null

    // Light
    // TOOD
    double lightEffectRate_ = 1.0;

    // PrimitiveShape VBO
    size_t generalVBOBufferSize_ = 1024;    // initial size
    std::vector<std::array<GLfloat, 3>> generalVertexBuffer_;
    VertexBufferObject generalVBO_;    // line, point
    VertexBufferObject vertexVBO_;    // draw vertex
    VertexBufferObject primitiveWorldGridVBO_;
    VertexBufferObject primitiveBoxVBO_;
    VertexBufferObject primitiveSphereVBO_;
    VertexBufferObject primitiveCylinderVBO_;
    VertexBufferObject primitiveCylinderSideVBO_;
    VertexBufferObject primitiveUpperSphereVBO_;
    VertexBufferObject primitiveLowerSphereVBO_;
    VertexBufferObject primitiveConeVBO_;
    VertexBufferObject primitiveCircleVBO_;
    VertexBufferObject primitiveRectVBO_;
    VertexBufferObject primitiveAxisVBO_;

    struct VBOInfo {
        VertexBufferObject vbo;
        std::shared_ptr<const VertexList> featureEdges;
        size_t numVertices = 0;
        size_t numIndices = 0;
    };
    std::unordered_map<std::string, VBOInfo> registeredVerticesVBOInfoMap_;
    struct RoundedMesh {
        std::array<double, 6> key;
        VBOInfo info;
    };
    std::list<RoundedMesh> roundedMeshes_;
    void drawRoundedPrimitive(const double pos[3], const double R[9], const std::array<double, 6>& key);

    // attribute/uniform location
    GLint attrVertexLocation_ = -1;
    GLint attrNormalLocation_ = -1;
    GLint unifColorLocation_ = -1;
    GLint unifModelMatrixLocation_ = -1;
    GLint unifNormalMatrixLocation_ = -1;
    GLint unifLightEffectRateLocation_ = -1;
    GLint unifPointSizeLocation_ = -1;
    GLint unifMaterialRoughnessLocation_ = -1;
    GLint unifMaterialReflectanceLocation_ = -1;

protected:
    // for GraphicsView
    virtual void beginRender(RenderState state);
    virtual void endRender(RenderState state);
    void setRenderState(RenderState state) { renderState_ = state; }
    void setProjectionViewMatrix(const Eigen::Matrix4d& m);

#if 0
    Eigen::Matrix4d projectionViewMatrix_ = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d inverseProjectionViewMatrix_ = Eigen::Matrix4d::Identity();
#else
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> projectionViewMatrix_ = Eigen::Matrix4d::Identity();
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> inverseProjectionViewMatrix_ = Eigen::Matrix4d::Identity();
#endif
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

} /* namespace ptgl */

#endif /* PTGL_CORE_RENDERER3D_H_ */
