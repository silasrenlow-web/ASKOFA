#include "renderer.hpp"

#include <vector>

#include "../core/geometry.hpp"

namespace askofa {
namespace {

const char* kVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vWorldPos;
out vec3 vNormal;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    gl_Position = uProjection * uView * world;
}
)GLSL";

// Blinn-Phong sun + hemisphere ambient. Colors arrive in sRGB and are
// converted to linear here; output goes back through an approximate
// gamma curve so what you pick is roughly what you see.
const char* kFragmentShader = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
uniform vec3 uColor;
uniform vec3 uSunDir;    // direction TO the sun, normalized
uniform vec3 uSunColor;
uniform vec3 uCamPos;
out vec4 fragColor;
vec3 srgbToLinear(vec3 c) { return pow(c, vec3(2.2)); }
void main() {
    vec3 base = srgbToLinear(uColor);
    vec3 n = normalize(vNormal);
    vec3 viewDir = normalize(uCamPos - vWorldPos);
    float ndl = max(dot(n, uSunDir), 0.0);
    vec3 hemi = mix(vec3(0.08, 0.09, 0.11), vec3(0.35, 0.38, 0.45), n.y * 0.5 + 0.5);
    vec3 diffuse = base * uSunColor * ndl;
    vec3 halfVec = normalize(uSunDir + viewDir);
    float spec = pow(max(dot(n, halfVec), 0.0), 48.0) * 0.35 * step(0.0, ndl);
    vec3 lit = base * hemi + diffuse + spec * uSunColor;
    fragColor = vec4(pow(lit, vec3(1.0 / 2.2)), 1.0);
}
)GLSL";

bool compileShader(gl::GLenum type, const char* source, gl::GLuint& out, std::string& error) {
    out = gl::createShader(type);
    gl::shaderSource(out, 1, &source, nullptr);
    gl::compileShader(out);
    gl::GLint ok = 0;
    gl::getShaderiv(out, gl::COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl::getShaderInfoLog(out, sizeof(log), nullptr, log);
        error = std::string("shader compile failed: ") + log;
        gl::deleteShader(out);
        out = 0;
        return false;
    }
    return true;
}

} // namespace

bool Renderer::buildProgram(std::string& error) {
    gl::GLuint vs = 0, fs = 0;
    if (!compileShader(gl::VERTEX_SHADER, kVertexShader, vs, error)) return false;
    if (!compileShader(gl::FRAGMENT_SHADER, kFragmentShader, fs, error)) return false;
    program_ = gl::createProgram();
    gl::attachShader(program_, vs);
    gl::attachShader(program_, fs);
    gl::linkProgram(program_);
    gl::deleteShader(vs);
    gl::deleteShader(fs);
    gl::GLint ok = 0;
    gl::getProgramiv(program_, gl::LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl::getProgramInfoLog(program_, sizeof(log), nullptr, log);
        error = std::string("program link failed: ") + log;
        return false;
    }
    uModel_ = gl::getUniformLocation(program_, "uModel");
    uView_ = gl::getUniformLocation(program_, "uView");
    uProjection_ = gl::getUniformLocation(program_, "uProjection");
    uColor_ = gl::getUniformLocation(program_, "uColor");
    uSunDir_ = gl::getUniformLocation(program_, "uSunDir");
    uSunColor_ = gl::getUniformLocation(program_, "uSunColor");
    uCamPos_ = gl::getUniformLocation(program_, "uCamPos");
    return true;
}

bool Renderer::init(std::string& error) {
    if (!buildProgram(error)) return false;
    gl::enable(gl::DEPTH_TEST);
    gl::depthFunc(gl::LEQUAL);
    gl::enable(gl::CULL_FACE);
    gl::cullFace(gl::BACK);
    gl::frontFace(gl::CCW);
    return true;
}

void Renderer::shutdown() {
    for (auto& kv : meshes_) {
        gl::deleteVertexArrays(1, &kv.second.vao);
        gl::deleteBuffers(1, &kv.second.vbo);
        gl::deleteBuffers(1, &kv.second.ebo);
    }
    meshes_.clear();
    if (program_) gl::deleteProgram(program_);
    program_ = 0;
}

Renderer::GpuMesh& Renderer::meshFor(ObjectType type) {
    int key = static_cast<int>(type);
    auto it = meshes_.find(key);
    if (it != meshes_.end()) return it->second;
    MeshData data = geometryFor(type);
    GpuMesh mesh;
    gl::genVertexArrays(1, &mesh.vao);
    gl::genBuffers(1, &mesh.vbo);
    gl::genBuffers(1, &mesh.ebo);
    gl::bindVertexArray(mesh.vao);
    gl::bindBuffer(gl::ARRAY_BUFFER, mesh.vbo);
    gl::bufferData(gl::ARRAY_BUFFER,
                   static_cast<gl::GLsizeiptr>(data.vertices.size() * sizeof(float)),
                   data.vertices.data(), gl::STATIC_DRAW);
    gl::bindBuffer(gl::ELEMENT_ARRAY_BUFFER, mesh.ebo);
    gl::bufferData(gl::ELEMENT_ARRAY_BUFFER,
                   static_cast<gl::GLsizeiptr>(data.indices.size() * sizeof(unsigned int)),
                   data.indices.data(), gl::STATIC_DRAW);
    gl::vertexAttribPointer(0, 3, gl::FLOAT, gl::FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(0));
    gl::enableVertexAttribArray(0);
    gl::vertexAttribPointer(1, 3, gl::FLOAT, gl::FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(3 * sizeof(float)));
    gl::enableVertexAttribArray(1);
    gl::bindVertexArray(0);
    mesh.indexCount = static_cast<gl::GLsizei>(data.indices.size());
    auto [inserted, _] = meshes_.emplace(key, mesh);
    (void)_;
    return inserted->second;
}

void Renderer::drawScene(const Scene& scene, const Mat4& view, const Mat4& projection, const Vec3& cameraPosition) {
    Vec3 bg = parseHexColor(scene.background);
    gl::clearColor(bg.x, bg.y, bg.z, 1.0f);
    gl::clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

    // Sun: first directional light in the scene, else a default.
    Vec3 sunDir{0.4f, 0.8f, 0.45f};
    Vec3 sunColor{1.0f, 0.98f, 0.92f};
    const Vec3& camPos = cameraPosition;
    for (const auto& o : scene.objects) {
        if (o.type == ObjectType::DirectionalLight) {
            sunDir = o.position.normalized();
            sunColor = parseHexColor(o.color);
        }
    }

    gl::useProgram(program_);
    gl::uniformMatrix4fv(uView_, 1, gl::FALSE, view.m);
    gl::uniformMatrix4fv(uProjection_, 1, gl::FALSE, projection.m);
    gl::uniform3fv(uSunDir_, 1, &sunDir.x);
    gl::uniform3fv(uSunColor_, 1, &sunColor.x);

    for (const auto& o : scene.objects) {
        if (!o.visible) continue;
        if (o.type == ObjectType::DirectionalLight || o.type == ObjectType::PointLight) continue; // lights are not drawn in N0
        Mat4 model = Mat4::trs(o.position, o.rotationDeg, o.scale);
        Vec3 color = parseHexColor(o.color);
        // Markers (camera / empty) draw at a fixed readable size.
        if (o.type == ObjectType::Camera || o.type == ObjectType::Empty) {
            model = Mat4::trs(o.position, o.rotationDeg, {0.5f, 0.5f, 0.5f});
            color = o.type == ObjectType::Camera ? Vec3{0.3f, 0.55f, 1.0f} : Vec3{0.6f, 0.6f, 0.6f};
        }
        gl::uniformMatrix4fv(uModel_, 1, gl::FALSE, model.m);
        gl::uniform3fv(uColor_, 1, &color.x);
        gl::uniform3fv(uCamPos_, 1, &camPos.x);
        GpuMesh& mesh = meshFor(o.type);
        gl::bindVertexArray(mesh.vao);
        gl::drawElements(gl::TRIANGLES, mesh.indexCount, gl::UNSIGNED_INT, nullptr);
    }
    gl::bindVertexArray(0);
}

} // namespace askofa
