// ASKOFA Engine core tests - no framework, just checks that count.
// Run with: ctest --test-dir build  (or run the askofa_tests binary directly)
#include <cmath>
#include <cstdio>
#include <string>

#include "../src/core/geometry.hpp"
#include "../src/core/json.hpp"
#include "../src/core/math.hpp"
#include "../src/core/scene.hpp"

using namespace askofa;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        ++checks;                                                       \
        if (!(cond)) {                                                  \
            ++failures;                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                               \
    } while (0)

static bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }
static bool nearVec(const Vec3& a, const Vec3& b, float eps = 1e-4f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

int main() {
    // --- math ---
    CHECK(nearVec(Vec3{1, 0, 0} + Vec3{0, 1, 0}, Vec3{1, 1, 0}));
    CHECK(near(dot(Vec3{1, 0, 0}, Vec3{0, 1, 0}), 0.0f));
    CHECK(nearVec(cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}), Vec3{0, 0, 1}));
    CHECK(near(Vec3{3, 4, 0}.length(), 5.0f));

    Mat4 t = Mat4::translation({1, 2, 3});
    CHECK(nearVec(t.transformPoint({0, 0, 0}), Vec3{1, 2, 3}));
    Mat4 trs = Mat4::trs({1, 0, 0}, {0, 90, 0}, {2, 2, 2});
    // scale (1,0,0)->(2,0,0), rotate Y 90deg -> (0,0,-2), translate -> (1,0,-2)
    CHECK(nearVec(trs.transformPoint({1, 0, 0}), Vec3{1, 0, -2}, 1e-3f));

    Mat4 persp = Mat4::perspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    Vec3 ndcNear = persp.transformPoint({0, 0, -0.1f});
    Vec3 ndcFar = persp.transformPoint({0, 0, -100.0f});
    CHECK(near(ndcNear.z, -1.0f, 1e-2f));
    CHECK(near(ndcFar.z, 1.0f, 1e-2f));

    Mat4 view = Mat4::lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    CHECK(nearVec(view.transformPoint({0, 0, 0}), Vec3{0, 0, -5}));

    // --- json ---
    {
        Json j; std::string err;
        CHECK(parseJson("{\"a\": [1, 2.5, \"x\"], \"b\": true}", j, err));
        const Json* a = j.find("a");
        CHECK(a && a->arrayValue.size() == 3);
        CHECK(a && near(static_cast<float>(a->arrayValue[1].numberValue), 2.5f));
        CHECK(j.find("b") && j.find("b")->boolValue);
        Json bad; CHECK(!parseJson("{oops", bad, err));
        std::string roundTrip = stringifyJson(j);
        Json j2; CHECK(parseJson(roundTrip, j2, err));
        CHECK(j2.find("a") && j2.find("a")->arrayValue.size() == 3);
    }

    // --- scene ---
    Scene starter = makeStarterScene();
    CHECK(starter.objects.size() == 6);
    bool hasPlayer = false, hasSun = false, hasGround = false;
    for (const auto& o : starter.objects) {
        if (o.behavior == Behavior::Player) hasPlayer = true;
        if (o.type == ObjectType::DirectionalLight) hasSun = true;
        if (o.type == ObjectType::Plane) hasGround = true;
    }
    CHECK(hasPlayer && hasSun && hasGround);

    std::string saved = saveSceneToString(starter);
    Scene loaded; std::string err;
    CHECK(loadSceneFromString(saved, loaded, err));
    CHECK(loaded.objects.size() == starter.objects.size());
    CHECK(loaded.name == starter.name);
    CHECK(loaded.objects[1].behavior == Behavior::Player);

    Scene bad; CHECK(!loadSceneFromString("{\"hello\":1}", bad, err));
    CHECK(!loadSceneFromString("not json", bad, err));

    Scene s = makeStarterScene();
    const std::string firstId = s.objects[0].id;
    SceneObject* copy = s.duplicate(firstId);
    CHECK(copy != nullptr);
    CHECK(copy && copy->id != firstId);
    CHECK(s.objects.size() == 7);
    CHECK(s.remove(copy->id));
    CHECK(!s.remove(copy->id));
    CHECK(s.objects.size() == 6);

    CHECK(toHexColor(parseHexColor("#ff0000")) == "#ff0000");
    CHECK(nearVec(parseHexColor("#000000"), Vec3{0, 0, 0}));
    CHECK(nearVec(parseHexColor("#ffffff"), Vec3{1, 1, 1}));

    // --- geometry ---
    MeshData cube = makeCubeGeometry();
    CHECK(cube.vertexCount() == 24);
    CHECK(cube.indices.size() == 36);
    MeshData sphere = makeSphereGeometry();
    CHECK(sphere.vertexCount() > 100);
    CHECK(!sphere.indices.empty());
    // Every normal in every built-in mesh must be unit length.
    for (ObjectType type : {ObjectType::Cube, ObjectType::Sphere, ObjectType::Cylinder,
                            ObjectType::Cone, ObjectType::Torus, ObjectType::Plane}) {
        MeshData mesh = geometryFor(type);
        bool normalsOk = true;
        for (size_t i = 3; i < mesh.vertices.size(); i += 6) {
            float len = std::sqrt(mesh.vertices[i] * mesh.vertices[i] +
                                  mesh.vertices[i + 1] * mesh.vertices[i + 1] +
                                  mesh.vertices[i + 2] * mesh.vertices[i + 2]);
            if (!near(len, 1.0f, 1e-3f)) { normalsOk = false; break; }
        }
        CHECK(normalsOk);
        // Indices must reference real vertices.
        bool indicesOk = true;
        for (unsigned int idx : mesh.indices) if (idx >= mesh.vertexCount()) indicesOk = false;
        CHECK(indicesOk);
    }

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
