// ASKOFA Engine N0 - native app entry point.
// Window + input via GLFW; every pixel is drawn by our own renderer
// (src/renderer). Editor panels/gizmo arrive in N1; N0 proves the core
// loop: load a scene, orbit it, press Space to play it, Esc to stop.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#define GLFW_INCLUDE_NONE 1 // we load OpenGL ourselves (renderer/gl_loader) - GLFW must not pull GL headers
#include <GLFW/glfw3.h>

#include "core/scene.hpp"
#include "renderer/gl_loader.hpp"
#include "renderer/renderer.hpp"

using namespace askofa;

namespace {

struct OrbitCamera {
    float yawDeg = 35.0f;
    float pitchDeg = 25.0f;
    float distance = 14.0f;
    Vec3 target{0, 1, 0};

    Vec3 position() const {
        float yaw = degToRad(yawDeg), pitch = degToRad(pitchDeg);
        return {target.x + distance * std::cos(pitch) * std::sin(yaw),
                target.y + distance * std::sin(pitch),
                target.z + distance * std::cos(pitch) * std::cos(yaw)};
    }
    Mat4 view() const { return Mat4::lookAt(position(), target, {0, 1, 0}); }
};

bool g_dragging = false;
bool g_panning = false;
double g_lastX = 0, g_lastY = 0;
OrbitCamera g_camera;
Scene g_scene;
bool g_playing = false;
std::string g_playSnapshot;
Vec3 g_playCamOffset{0, 5, 9};
float g_playerVy = 0;
bool g_playerGrounded = false;
std::unordered_map<std::string, float> g_bobBaseY; // per-object base Y captured when Play starts

void* getProcAddressBridge(const char* name) {
    return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return "";
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void snapshotScene() {
    g_playSnapshot.clear();
    for (const auto& o : g_scene.objects) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s %.4f %.4f %.4f %.4f %.4f %.4f\n", o.id.c_str(),
                      o.position.x, o.position.y, o.position.z,
                      o.rotationDeg.x, o.rotationDeg.y, o.rotationDeg.z);
        g_playSnapshot += buf;
    }
}

void restoreScene() {
    std::stringstream ss(g_playSnapshot);
    std::string id;
    float px, py, pz, rx, ry, rz;
    while (ss >> id >> px >> py >> pz >> rx >> ry >> rz) {
        if (SceneObject* o = g_scene.find(id)) {
            o->position = {px, py, pz};
            o->rotationDeg = {rx, ry, rz};
        }
    }
}

void setPlaying(bool play, GLFWwindow* window) {
    if (play == g_playing) return;
    g_playing = play;
    if (play) {
        snapshotScene();
        g_playerVy = 0;
        g_playerGrounded = false;
        g_bobBaseY.clear();
        SceneObject* player = nullptr;
        SceneObject* cam = nullptr;
        for (auto& o : g_scene.objects) {
            if (o.behavior == Behavior::Player) player = &o;
            if (o.type == ObjectType::Camera) cam = &o;
            if (o.behavior == Behavior::Bob || o.behavior == Behavior::SpinBob) g_bobBaseY[o.id] = o.position.y;
        }
        if (player && cam) {
            g_playCamOffset = cam->position - player->position;
            if (g_playCamOffset.length() < 3.0f) g_playCamOffset = {0, 5, 9};
            if (g_playCamOffset.y < 2.0f) g_playCamOffset.y = 2.0f;
        }
        std::printf("[ASKOFA] Play - WASD moves the Player, Space jumps, Esc stops\n");
    } else {
        restoreScene();
        std::printf("[ASKOFA] Edit - drag to orbit, right-drag to pan, wheel to zoom, Space plays\n");
    }
    (void)window;
}

void keyCallback(GLFWwindow* window, int key, int, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_SPACE && !g_playing) setPlaying(true, window);
    else if (key == GLFW_KEY_ESCAPE && g_playing) setPlaying(false, window);
    else if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL)) {
        std::ofstream out("scene.askofa.json");
        out << saveSceneToString(g_scene);
        std::printf("[ASKOFA] Saved scene.askofa.json\n");
    }
}

void mouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) g_dragging = action == GLFW_PRESS;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) g_panning = action == GLFW_PRESS;
}

void cursorPosCallback(GLFWwindow*, double x, double y) {
    double dx = x - g_lastX, dy = y - g_lastY;
    g_lastX = x;
    g_lastY = y;
    if (g_playing) return;
    if (g_dragging) {
        g_camera.yawDeg -= static_cast<float>(dx) * 0.25f;
        g_camera.pitchDeg += static_cast<float>(dy) * 0.25f;
        if (g_camera.pitchDeg > 89.0f) g_camera.pitchDeg = 89.0f;
        if (g_camera.pitchDeg < -89.0f) g_camera.pitchDeg = -89.0f;
    } else if (g_panning) {
        g_camera.target.x -= static_cast<float>(dx) * 0.02f;
        g_camera.target.y += static_cast<float>(dy) * 0.02f;
    }
}

void scrollCallback(GLFWwindow*, double, double yOffset) {
    if (g_playing) return;
    g_camera.distance -= static_cast<float>(yOffset) * 1.2f;
    if (g_camera.distance < 3.0f) g_camera.distance = 3.0f;
    if (g_camera.distance > 60.0f) g_camera.distance = 60.0f;
}

// Play-mode behavior step: the same rules the web prototype used,
// re-implemented against the native scene data.
void stepPlay(float dt, float t, GLFWwindow* window) {
    for (auto& o : g_scene.objects) {
        float speed = o.behaviorSpeed;
        if (o.behavior == Behavior::Spin) {
            o.rotationDeg.y += 60.0f * speed * dt;
        } else if (o.behavior == Behavior::Bob || o.behavior == Behavior::SpinBob) {
            if (o.behavior == Behavior::SpinBob) o.rotationDeg.y += 60.0f * speed * dt;
            auto it = g_bobBaseY.find(o.id);
            float base = it != g_bobBaseY.end() ? it->second : o.position.y;
            o.position.y = base + std::sin(t * 2.0f * speed) * 0.35f;
        } else if (o.behavior == Behavior::Player) {
            float dx = 0, dz = 0;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dz -= 1;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dz += 1;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx -= 1;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx += 1;
            if (dx != 0 || dz != 0) {
                float len = std::sqrt(dx * dx + dz * dz);
                o.position.x += dx / len * 6.0f * dt;
                o.position.z += dz / len * 6.0f * dt;
                o.rotationDeg.y = radToDeg(std::atan2(dx, dz)) + 180.0f;
            }
            float floorY = g_scene.groundY + o.scale.y * 0.5f + 0.05f;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && g_playerGrounded) {
                g_playerVy = 8.5f;
                g_playerGrounded = false;
            }
            g_playerVy -= g_scene.gravity * dt;
            o.position.y += g_playerVy * dt;
            if (o.position.y <= floorY) {
                o.position.y = floorY;
                g_playerVy = 0;
                g_playerGrounded = true;
            }
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    g_scene = makeStarterScene();
    if (argc > 1) {
        std::string text = readFile(argv[1]);
        Scene loaded;
        std::string error;
        if (!text.empty() && loadSceneFromString(text, loaded, error)) {
            g_scene = loaded;
            std::printf("[ASKOFA] Loaded scene: %s\n", argv[1]);
        } else {
            std::printf("[ASKOFA] Could not load %s (%s) - using starter scene\n",
                        argv[1], error.empty() ? "file not found" : error.c_str());
        }
    }

    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW failed to start (no display / missing libraries)\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 800, "ASKOFA Engine N0", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Could not create a window / OpenGL 3.3 context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    const char* missing = nullptr;
    if (!gl::loadGL(getProcAddressBridge, missing)) {
        std::fprintf(stderr, "OpenGL function missing: %s\n", missing ? missing : "?");
        return 1;
    }
    Renderer renderer;
    std::string error;
    if (!renderer.init(error)) {
        std::fprintf(stderr, "Renderer failed: %s\n", error.c_str());
        return 1;
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwGetCursorPos(window, &g_lastX, &g_lastY);

    std::printf("[ASKOFA] Edit - drag to orbit, right-drag to pan, wheel to zoom, Space plays, Ctrl+S saves\n");

    double lastTime = glfwGetTime();
    double titleTimer = 0;
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f;

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width > 0 && height > 0) gl::viewport(0, 0, width, height);
        float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
        Mat4 projection = Mat4::perspective(60.0f, aspect, 0.1f, 200.0f);

        Mat4 view;
        Vec3 camPos;
        if (g_playing) {
            stepPlay(dt, static_cast<float>(now), window);
            SceneObject* player = nullptr;
            SceneObject* camObj = nullptr;
            for (auto& o : g_scene.objects) {
                if (o.behavior == Behavior::Player) player = &o;
                if (o.type == ObjectType::Camera) camObj = &o;
            }
            if (player) {
                camPos = player->position + g_playCamOffset;
                view = Mat4::lookAt(camPos, player->position + Vec3{0, 1, 0}, {0, 1, 0});
            } else if (camObj) {
                camPos = camObj->position;
                view = Mat4::lookAt(camPos, {0, 1, 0}, {0, 1, 0});
            } else {
                camPos = g_camera.position();
                view = g_camera.view();
            }
        } else {
            camPos = g_camera.position();
            view = g_camera.view();
        }

        renderer.drawScene(g_scene, view, projection, camPos);
        glfwSwapBuffers(window);
        glfwPollEvents();

        titleTimer += dt;
        if (titleTimer > 0.25) {
            titleTimer = 0;
            char title[128];
            std::snprintf(title, sizeof(title), "ASKOFA Engine N0 - %s - %s",
                          g_scene.name.c_str(), g_playing ? "PLAYING (Esc = stop)" : "Edit (Space = play)");
            glfwSetWindowTitle(window, title);
        }
    }

    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
