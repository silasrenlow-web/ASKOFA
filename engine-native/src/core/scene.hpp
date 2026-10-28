// ASKOFA Engine - scene model. A scene is plain data: a list of objects
// with readable properties. Same format as the web prototype's JSON, so
// scenes can move between tools. The renderer never owns scene data.
#pragma once

#include <string>
#include <vector>

#include "json.hpp"
#include "math.hpp"

namespace askofa {

enum class ObjectType { Cube, Sphere, Cylinder, Cone, Torus, Plane, DirectionalLight, PointLight, Camera, Empty };

enum class Behavior { None, Spin, Bob, SpinBob, Player };

std::string toString(ObjectType t);
bool objectTypeFromString(const std::string& s, ObjectType& out);
std::string toString(Behavior b);
bool behaviorFromString(const std::string& s, Behavior& out);

struct SceneObject {
    std::string id;
    ObjectType type = ObjectType::Cube;
    std::string name = "Cube";
    Vec3 position{0, 1, 0};
    Vec3 rotationDeg{0, 0, 0};
    Vec3 scale{1, 1, 1};
    std::string color = "#e2554d"; // hex, like the web prototype
    bool visible = true;
    Behavior behavior = Behavior::None;
    float behaviorSpeed = 1.0f;
    std::string script; // reserved for the scripting pass (N2+)
};

struct Scene {
    int version = 1;
    std::string name = "Untitled Scene";
    std::string background = "#101216";
    bool fog = true;
    float gravity = 18.0f;
    float groundY = 0.0f;
    std::vector<SceneObject> objects;

    SceneObject* find(const std::string& id);
    const SceneObject* find(const std::string& id) const;
    SceneObject& add(const SceneObject& obj);
    bool remove(const std::string& id);
    SceneObject* duplicate(const std::string& id);
};

SceneObject makeObject(ObjectType type);
Scene makeStarterScene();

std::string newId();

// Hex "#rrggbb" -> linear-ish RGB in [0,1]. (Renderer treats colors in
// sRGB and converts; this helper just parses.)
Vec3 parseHexColor(const std::string& hex);
std::string toHexColor(const Vec3& rgb);

// Scene <-> JSON (the .askofa.json format).
Json sceneToJson(const Scene& scene);
bool sceneFromJson(const Json& json, Scene& out, std::string& error);
std::string saveSceneToString(const Scene& scene);
bool loadSceneFromString(const std::string& text, Scene& out, std::string& error);

} // namespace askofa
