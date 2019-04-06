#include "DefaultShaderSource.h"

namespace ptgl {

DefaultShaderSource::DefaultShaderSource() {

}

DefaultShaderSource::~DefaultShaderSource() {

}

// default shader source
const std::string DefaultShaderSource::DefaultVertexShaderSource =
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"//uniform mat4 modelViewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat3 normalMatrix;\n"
"uniform vec3 cameraPosition;\n"
"uniform vec4 color;\n"
"uniform float lightEffectRate;\n"
"uniform float pointSize;\n"
"attribute vec3 position;\n"
"attribute vec3 normal;\n"
"//attribute vec2 uv;\n"
"varying vec4 vColor;\n"
"varying float vPointSize;\n"
"const vec3 lightDirection1 = vec3(1,1,2);\n"
"const vec3 lightDirection2 = vec3(1,-1,2);\n"
"float mapRange(float x, float imin, float imax, float omin, float omax)\n"
"{\n"
"    x = clamp(x, imin, imax);\n"
"    float id = imax - imin;\n"
"    float od = omax - omin;\n"
"    float s = od / id;\n"
"    return omin + s* x;\n"
"}\n"
"void main() {\n"
"    // position\n"
"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1);\n"
"    // color\n"
"    vec3 tvnormal = normalMatrix * normal;\n"
"    vec3 vnormal =  normalize(tvnormal);  \n"
"    // light 1\n"
"    vec3 vlightDirection1 = normalize(lightDirection1);\n"
"    float diffuse1 = clamp(dot(vnormal, vlightDirection1), 0.1, 1.0); \n"
"    // light 2\n"
"    vec3 vlightDirection2 = normalize(lightDirection2);\n"
"    float diffuse2 = clamp(dot(vnormal, vlightDirection2), 0.1, 1.0);    \n"
"    float diffuse = 0.5 * (diffuse1 + diffuse2);\n"
"    diffuse = mapRange(diffuse, 0.1, 1.0, 0.5, 1.0);\n"
"    //vColor         = color * vec4(vec3(vdiffuse), 1.0);\n"
"    vec4 diffColor = color * vec4(vec3(diffuse), 1.0);\n"
"    vColor = mix(color, diffColor, lightEffectRate);   \n"
"    // point size\n"
"    //gl_PointSize = pointSize / gl_Position.w;\n"
"    gl_PointSize = pointSize;\n"
"    vPointSize = pointSize;\n"
"}\n";

const std::string DefaultShaderSource::DefaultFragmentShaderSource =
"precision mediump float;\n"
"varying vec4 vColor;\n"
"varying float vPointSize;\n"
"void main() {\n"
"    // point\n"
"    // todo efficiency\n"
"    if (vPointSize > 1.0) {\n"
"        // http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20110324\n"
"        vec3 n;\n"
"        n.xy = gl_PointCoord * 2.0 - 1.0;\n"
"        n.z = 1.0 - dot(n.xy, n.xy);\n"
"        if (n.z < 0.0) discard;\n"
"    }\n"
"    vec4 finalColor = vColor;\n"
"    gl_FragColor = finalColor;\n"
"}\n";

// default picking up shader source
const std::string DefaultShaderSource::PickingUpVertexShaderSource =
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"uniform vec4 pickingUpColor;\n"
"attribute vec3 position;\n"
"varying vec4 vColor;\n"
"void main() {\n"
"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1);\n"
"    vColor = pickingUpColor;\n"
"}\n";

const std::string DefaultShaderSource::PickingUpFragmentShaderSource =
"precision mediump float;\n"
"varying vec4 vColor;\n"
"\n"
"void main() {\n"
"    gl_FragColor = vColor;\n"
"}\n";

// default depth render shader source
const std::string DefaultShaderSource::DepthRenderVertexShaderSource =
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"attribute vec3 position;\n"
"uniform float perspectiveZnear;\n"
"uniform float perspectiveZfar;\n"
"varying vec3 vDepthRGB;\n"
"vec3 valueToRGB(float x)\n"
"{\n"
"   float v1 = (255.0*255.0*255.0) * x;\n"
"    float v2 = (255.0*255.0) * x;\n"
"    float v3 = (255.0) * x;\n"
"    float fm1 = floor(mod(v1,255.0));\n"
"    float fm2 = floor(mod(v2,255.0));\n"
"    float fm3 = floor(mod(v3,255.0));\n"
"    float r = fm1/255.0;\n"
"    float g = fm2/255.0;\n"
"    float b = fm3/255.0;\n"
"    return vec3(r, g, b);\n"
"}\n"
"void main() {\n"
"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1);\n"
"    float far = perspectiveZfar;\n"
"    float near = perspectiveZnear;\n"
"    float z = gl_Position.z;\n"
"    float depth = (z - near) / (far - near);\n"
"    vDepthRGB = valueToRGB(depth);\n"
"}\n";

const std::string DefaultShaderSource::DepthRenderFragmentShaderSource =
"precision mediump float;\n"
"varying vec3 vDepthRGB;\n"
"void main() {\n"
"    gl_FragColor = vec4(vDepthRGB, 1.0);\n"
"}\n";

// 2D scene
// default shader source
const std::string DefaultShaderSource::Default2DVertexShaderSource =
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"uniform vec4 color;\n"
"uniform float pointSize;\n"
"attribute vec3 position;\n"
"varying vec4 vColor;\n"
"varying float vPointSize;\n"
"void main() {\n"
"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1);\n"
"    vColor = color;\n"
"    gl_PointSize = pointSize;\n"
"    vPointSize = pointSize;\n"
"}\n";

// http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20110324
const std::string DefaultShaderSource::Default2DFragmentShaderSource =
"precision mediump float;\n"
"varying vec4 vColor;\n"
"varying float vPointSize;\n"
"void main() {\n"
"    if (vPointSize > 1.0) {\n"
"        vec3 n;\n"
"        n.xy = gl_PointCoord * 2.0 - 1.0;\n"
"        n.z = 1.0 - dot(n.xy, n.xy);\n"
"        if (n.z < 0.0) discard;\n"
"    }\n"
"    gl_FragColor = vColor;\n"
"}\n";

} /* namespace ptgl */
