#include "scene.hpp"

#include <atomic>
#include <cctype>
#include <cstdio>

namespace askofa {

std::string toString(ObjectType t) {
    switch (t) {
        case ObjectType::Cube: return "cube";
        case ObjectType::Sphere: return "sphere";
        case ObjectType::Cylinder: return "cylinder";
        case ObjectType::Cone: return "cone";
        case ObjectType::Torus: return "torus";
        case ObjectType::Plane: return "plane";
        case ObjectType::DirectionalLight: return "directionalLight";
        case ObjectType::PointLight: return "pointLight";
        case ObjectType::Camera: return "camera";
        case ObjectType::Empty: return "empty";
    }
    return "cube";
}

bool objectTypeFromString(const std::string& s, ObjectType& out) {
    static const ObjectType all[] = {ObjectType::Cube, ObjectType::Sphere, ObjectType::Cylinder,
                                     ObjectType::Cone, ObjectType::Torus, ObjectType::Plane,
                                     ObjectType::DirectionalLight, ObjectType::PointLight,
                                     ObjectType::Camera, ObjectType::Empty};
    for (ObjectType t : all) {
        if (toString(t) == s) { out = t; return true; }
    }
    return false;
}

std::string toString(Behavior b) {
    switch (b) {
        case Behavior::None: return "none";
        case Behavior::Spin: return "spin";
        case Behavior::Bob: return "bob";
        case Behavior::SpinBob: return "spinBob";
        case Behavior::Player: return "player";
    }
    return "none";
}

bool behaviorFromString(const std::string& s, Behavior& out) {
    static const Behavior all[] = {Behavior::None, Behavior::Spin, Behavior::Bob, Behavior::SpinBob, Behavior::Player};
    for (Behavior b : all) {
        if (toString(b) == s) { out = b; return true; }
    }
    return false;
}

std::string newId() {
    static std::atomic<unsigned long> counter{0};
    char buf[32];
    std::snprintf(buf, sizeof(buf), "obj_%lu", ++counter);
    return buf;
}

static std::string defaultName(ObjectType t) {
    std::string s = toString(t);
    if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

SceneObject makeObject(ObjectType type) {
    SceneObject o;
    o.id = newId();
    o.type = type;
    o.name = defaultName(type);
    if (type == ObjectType::Plane) {
        o.position = {0, 0, 0};
        o.scale = {10, 1, 10};
        o.color = "#3d4348";
        o.name = "Ground";
    } else if (type == ObjectType::DirectionalLight) {
        o.position = {5, 10, 5};
        o.color = "#ffffff";
        o.name = "Sun";
    } else if (type == ObjectType::PointLight) {
        o.color = "#ffd28a";
    } else if (type == ObjectType::Camera) {
        o.position = {0, 3, 8};
        o.color = "#ffffff";
    }
    return o;
}

Scene makeStarterScene() {
    Scene scene;
    scene.name = "My First Scene";
    scene.objects.push_back(makeObject(ObjectType::Plane));

    SceneObject player = makeObject(ObjectType::Cube);
    player.name = "Player";
    player.color = "#4da3ff";
    player.behavior = Behavior::Player;
    scene.objects.push_back(player);

    SceneObject ball = makeObject(ObjectType::Sphere);
    ball.name = "Ball";
    ball.position = {3, 1, -2};
    ball.color = "#ffc24d";
    ball.behavior = Behavior::Bob;
    scene.objects.push_back(ball);

    SceneObject ring = makeObject(ObjectType::Torus);
    ring.name = "Ring";
    ring.position = {-3, 2, -2};
    ring.color = "#7dff8a";
    ring.behavior = Behavior::Spin;
    scene.objects.push_back(ring);

    scene.objects.push_back(makeObject(ObjectType::DirectionalLight));

    SceneObject cam = makeObject(ObjectType::Camera);
    cam.name = "Game Camera";
    scene.objects.push_back(cam);
    return scene;
}

SceneObject* Scene::find(const std::string& id) {
    for (auto& o : objects) if (o.id == id) return &o;
    return nullptr;
}
const SceneObject* Scene::find(const std::string& id) const {
    for (const auto& o : objects) if (o.id == id) return &o;
    return nullptr;
}
SceneObject& Scene::add(const SceneObject& obj) {
    objects.push_back(obj);
    return objects.back();
}
bool Scene::remove(const std::string& id) {
    for (size_t i = 0; i < objects.size(); ++i) {
        if (objects[i].id == id) { objects.erase(objects.begin() + static_cast<long>(i)); return true; }
    }
    return false;
}
SceneObject* Scene::duplicate(const std::string& id) {
    SceneObject* src = find(id);
    if (!src) return nullptr;
    SceneObject copy = *src;
    copy.id = newId();
    copy.name = src->name + " Copy";
    copy.position.x += 1.0f;
    copy.position.z += 1.0f;
    objects.push_back(copy);
    return &objects.back();
}

Vec3 parseHexColor(const std::string& hex) {
    auto nibble = [](const std::string& s, size_t i) -> float {
        auto v = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        return static_cast<float>(v(s[i]) * 16 + v(s[i + 1])) / 255.0f;
    };
    if (hex.size() == 7 && hex[0] == '#') return {nibble(hex, 1), nibble(hex, 3), nibble(hex, 5)};
    return {1, 1, 1};
}

std::string toHexColor(const Vec3& rgb) {
    auto clamp255 = [](float v) {
        int i = static_cast<int>(v * 255.0f + 0.5f);
        if (i < 0) i = 0;
        if (i > 255) i = 255;
        return i;
    };
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", clamp255(rgb.x), clamp255(rgb.y), clamp255(rgb.z));
    return buf;
}

static Json vec3ToJson(const Vec3& v) {
    Json j = Json::makeArray();
    j.arrayValue.push_back(Json::makeNumber(v.x));
    j.arrayValue.push_back(Json::makeNumber(v.y));
    j.arrayValue.push_back(Json::makeNumber(v.z));
    return j;
}

static Vec3 vec3FromJson(const Json& j, const Vec3& fallback) {
    if (j.type != Json::Type::Array || j.arrayValue.size() != 3) return fallback;
    return {static_cast<float>(j.arrayValue[0].numberOr(fallback.x)),
            static_cast<float>(j.arrayValue[1].numberOr(fallback.y)),
            static_cast<float>(j.arrayValue[2].numberOr(fallback.z))};
}

Json sceneToJson(const Scene& scene) {
    Json root = Json::makeObject();
    root.objectValue.emplace_back("version", Json::makeNumber(scene.version));
    root.objectValue.emplace_back("name", Json::makeString(scene.name));

    Json settings = Json::makeObject();
    settings.objectValue.emplace_back("background", Json::makeString(scene.background));
    settings.objectValue.emplace_back("fog", Json::makeBool(scene.fog));
    settings.objectValue.emplace_back("gravity", Json::makeNumber(scene.gravity));
    settings.objectValue.emplace_back("groundY", Json::makeNumber(scene.groundY));
    root.objectValue.emplace_back("settings", settings);

    Json objs = Json::makeArray();
    for (const SceneObject& o : scene.objects) {
        Json jo = Json::makeObject();
        jo.objectValue.emplace_back("id", Json::makeString(o.id));
        jo.objectValue.emplace_back("type", Json::makeString(toString(o.type)));
        jo.objectValue.emplace_back("name", Json::makeString(o.name));
        jo.objectValue.emplace_back("position", vec3ToJson(o.position));
        jo.objectValue.emplace_back("rotation", vec3ToJson(o.rotationDeg));
        jo.objectValue.emplace_back("scale", vec3ToJson(o.scale));
        jo.objectValue.emplace_back("color", Json::makeString(o.color));
        jo.objectValue.emplace_back("visible", Json::makeBool(o.visible));
        jo.objectValue.emplace_back("behavior", Json::makeString(toString(o.behavior)));
        jo.objectValue.emplace_back("behaviorSpeed", Json::makeNumber(o.behaviorSpeed));
        jo.objectValue.emplace_back("script", Json::makeString(o.script));
        objs.arrayValue.push_back(std::move(jo));
    }
    root.objectValue.emplace_back("objects", std::move(objs));
    return root;
}

bool sceneFromJson(const Json& json, Scene& out, std::string& error) {
    static const Json kEmpty = Json::makeObject();
    const Json* objects = json.find("objects");
    if (!objects || objects->type != Json::Type::Array) {
        error = "Not an ASKOFA scene: missing objects list";
        return false;
    }
    Scene scene;
    scene.name = json.at("name", kEmpty).stringOr(scene.name);
    if (const Json* settings = json.find("settings")) {
        scene.background = settings->at("background", kEmpty).stringOr(scene.background);
        scene.fog = settings->at("fog", kEmpty).boolOr(scene.fog);
        scene.gravity = static_cast<float>(settings->at("gravity", kEmpty).numberOr(scene.gravity));
        scene.groundY = static_cast<float>(settings->at("groundY", kEmpty).numberOr(scene.groundY));
    }
    for (const Json& jo : objects->arrayValue) {
        ObjectType type;
        if (!objectTypeFromString(jo.at("type", kEmpty).stringOr(""), type)) continue; // skip unknown types
        SceneObject o = makeObject(type);
        o.id = jo.at("id", kEmpty).stringOr(o.id);
        o.name = jo.at("name", kEmpty).stringOr(o.name);
        o.position = vec3FromJson(jo.at("position", kEmpty), o.position);
        o.rotationDeg = vec3FromJson(jo.at("rotation", kEmpty), o.rotationDeg);
        o.scale = vec3FromJson(jo.at("scale", kEmpty), o.scale);
        o.color = jo.at("color", kEmpty).stringOr(o.color);
        o.visible = jo.at("visible", kEmpty).boolOr(true);
        Behavior b;
        if (behaviorFromString(jo.at("behavior", kEmpty).stringOr("none"), b)) o.behavior = b;
        o.behaviorSpeed = static_cast<float>(jo.at("behaviorSpeed", kEmpty).numberOr(1.0));
        o.script = jo.at("script", kEmpty).stringOr("");
        scene.objects.push_back(std::move(o));
    }
    out = std::move(scene);
    return true;
}

std::string saveSceneToString(const Scene& scene) { return stringifyJson(sceneToJson(scene)); }

bool loadSceneFromString(const std::string& text, Scene& out, std::string& error) {
    Json json;
    if (!parseJson(text, json, error)) return false;
    return sceneFromJson(json, out, error);
}

} // namespace askofa
