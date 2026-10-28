// ASKOFA Engine - the renderer. Ours end to end: shaders written here,
// meshes uploaded by hand, one draw call per scene object. N0 lighting is
// a directional "sun" + hemisphere ambient in the fragment shader;
// shadow maps land in N1 (the interface below will not change).
#pragma once

#include <string>
#include <unordered_map>

#include "../core/scene.hpp"
#include "gl_loader.hpp"

namespace askofa {

class Renderer {
public:
    // Compiles shaders. Call after an OpenGL context exists and loadGL() ran.
    bool init(std::string& error);
    void shutdown();

    // Draws the scene from the given camera. Builds GPU meshes lazily and
    // caches them per object type.
    void drawScene(const Scene& scene, const Mat4& view, const Mat4& projection, const Vec3& cameraPosition);

private:
    struct GpuMesh {
        gl::GLuint vao = 0;
        gl::GLuint vbo = 0;
        gl::GLuint ebo = 0;
        gl::GLsizei indexCount = 0;
    };

    GpuMesh& meshFor(ObjectType type);
    bool buildProgram(std::string& error);

    gl::GLuint program_ = 0;
    gl::GLint uModel_ = -1, uView_ = -1, uProjection_ = -1;
    gl::GLint uColor_ = -1, uSunDir_ = -1, uSunColor_ = -1, uCamPos_ = -1;
    std::unordered_map<int, GpuMesh> meshes_;
};

} // namespace askofa
