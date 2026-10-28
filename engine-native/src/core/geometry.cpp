#include "geometry.hpp"

namespace askofa {

namespace {

void pushVertex(MeshData& mesh, const Vec3& pos, const Vec3& normal) {
    mesh.vertices.push_back(pos.x);
    mesh.vertices.push_back(pos.y);
    mesh.vertices.push_back(pos.z);
    mesh.vertices.push_back(normal.x);
    mesh.vertices.push_back(normal.y);
    mesh.vertices.push_back(normal.z);
}

void pushQuad(MeshData& mesh, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& normal) {
    unsigned int base = static_cast<unsigned int>(mesh.vertexCount());
    pushVertex(mesh, a, normal);
    pushVertex(mesh, b, normal);
    pushVertex(mesh, c, normal);
    pushVertex(mesh, d, normal);
    mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

} // namespace

MeshData makeCubeGeometry() {
    MeshData mesh;
    const float h = 0.5f;
    // +Z front, -Z back, +X right, -X left, +Y top, -Y bottom.
    pushQuad(mesh, {-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}, {0, 0, 1});
    pushQuad(mesh, {h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}, {0, 0, -1});
    pushQuad(mesh, {h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}, {1, 0, 0});
    pushQuad(mesh, {-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}, {-1, 0, 0});
    pushQuad(mesh, {-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}, {0, 1, 0});
    pushQuad(mesh, {-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}, {0, -1, 0});
    return mesh;
}

MeshData makeSphereGeometry(int rings, int sectors) {
    MeshData mesh;
    const float radius = 0.6f;
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = kPi * static_cast<float>(ring) / static_cast<float>(rings); // 0..pi from top
        for (int sector = 0; sector <= sectors; ++sector) {
            float theta = 2.0f * kPi * static_cast<float>(sector) / static_cast<float>(sectors);
            Vec3 p{ radius * std::sin(phi) * std::cos(theta),
                    radius * std::cos(phi),
                    radius * std::sin(phi) * std::sin(theta) };
            pushVertex(mesh, p, p.normalized());
        }
    }
    for (int ring = 0; ring < rings; ++ring) {
        for (int sector = 0; sector < sectors; ++sector) {
            unsigned int a = static_cast<unsigned int>(ring * (sectors + 1) + sector);
            unsigned int b = a + static_cast<unsigned int>(sectors) + 1;
            mesh.indices.insert(mesh.indices.end(), {a, b, a + 1, b, b + 1, a + 1});
        }
    }
    return mesh;
}

MeshData makeCylinderGeometry(int sectors) {
    MeshData mesh;
    const float r = 0.5f, h = 0.6f; // total height 1.2
    for (int i = 0; i < sectors; ++i) {
        float t0 = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(sectors);
        float t1 = 2.0f * kPi * static_cast<float>(i + 1) / static_cast<float>(sectors);
        Vec3 p0{r * std::cos(t0), 0, r * std::sin(t0)};
        Vec3 p1{r * std::cos(t1), 0, r * std::sin(t1)};
        Vec3 n0 = p0.normalized(); n0.y = 0;
        Vec3 n1 = p1.normalized(); n1.y = 0;
        unsigned int base = static_cast<unsigned int>(mesh.vertexCount());
        pushVertex(mesh, {p0.x, -h, p0.z}, n0);
        pushVertex(mesh, {p1.x, -h, p1.z}, n1);
        pushVertex(mesh, {p1.x, h, p1.z}, n1);
        pushVertex(mesh, {p0.x, h, p0.z}, n0);
        mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        // caps (triangle fan around a center vertex)
        unsigned int topCenter = static_cast<unsigned int>(mesh.vertexCount());
        pushVertex(mesh, {0, h, 0}, {0, 1, 0});
        pushVertex(mesh, {p0.x, h, p0.z}, {0, 1, 0});
        pushVertex(mesh, {p1.x, h, p1.z}, {0, 1, 0});
        mesh.indices.insert(mesh.indices.end(), {topCenter, topCenter + 1, topCenter + 2});
        unsigned int botCenter = static_cast<unsigned int>(mesh.vertexCount());
        pushVertex(mesh, {0, -h, 0}, {0, -1, 0});
        pushVertex(mesh, {p1.x, -h, p1.z}, {0, -1, 0});
        pushVertex(mesh, {p0.x, -h, p0.z}, {0, -1, 0});
        mesh.indices.insert(mesh.indices.end(), {botCenter, botCenter + 1, botCenter + 2});
    }
    return mesh;
}

MeshData makeConeGeometry(int sectors) {
    MeshData mesh;
    const float r = 0.6f, h = 0.6f;
    for (int i = 0; i < sectors; ++i) {
        float t0 = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(sectors);
        float t1 = 2.0f * kPi * static_cast<float>(i + 1) / static_cast<float>(sectors);
        Vec3 p0{r * std::cos(t0), -h, r * std::sin(t0)};
        Vec3 p1{r * std::cos(t1), -h, r * std::sin(t1)};
        Vec3 apex{0, h, 0};
        // Slanted normal: perpendicular-ish to the cone slope (good enough for stylized lighting).
        Vec3 mid = (p0 + p1) * 0.5f;
        Vec3 slopeNormal = Vec3{mid.x, r * 0.5f, mid.z}.normalized();
        unsigned int base = static_cast<unsigned int>(mesh.vertexCount());
        pushVertex(mesh, p0, slopeNormal);
        pushVertex(mesh, p1, slopeNormal);
        pushVertex(mesh, apex, slopeNormal);
        mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2});
        unsigned int botCenter = static_cast<unsigned int>(mesh.vertexCount());
        pushVertex(mesh, {0, -h, 0}, {0, -1, 0});
        pushVertex(mesh, p1, {0, -1, 0});
        pushVertex(mesh, p0, {0, -1, 0});
        mesh.indices.insert(mesh.indices.end(), {botCenter, botCenter + 1, botCenter + 2});
    }
    return mesh;
}

MeshData makeTorusGeometry(int rings, int sides) {
    MeshData mesh;
    const float R = 0.6f, r = 0.22f;
    for (int ring = 0; ring <= rings; ++ring) {
        float u = 2.0f * kPi * static_cast<float>(ring) / static_cast<float>(rings);
        Vec3 center{R * std::cos(u), 0, R * std::sin(u)};
        for (int side = 0; side <= sides; ++side) {
            float v = 2.0f * kPi * static_cast<float>(side) / static_cast<float>(sides);
            Vec3 p{ (R + r * std::cos(v)) * std::cos(u),
                    r * std::sin(v),
                    (R + r * std::cos(v)) * std::sin(u) };
            pushVertex(mesh, p, (p - center).normalized());
        }
    }
    for (int ring = 0; ring < rings; ++ring) {
        for (int side = 0; side < sides; ++side) {
            unsigned int a = static_cast<unsigned int>(ring * (sides + 1) + side);
            unsigned int b = a + static_cast<unsigned int>(sides) + 1;
            mesh.indices.insert(mesh.indices.end(), {a, a + 1, b, b, a + 1, b + 1});
        }
    }
    return mesh;
}

MeshData makeGroundGeometry() {
    MeshData mesh;
    // Unit slab in X/Z (scaled by the object), 0.1 thick, top at y=0.
    pushQuad(mesh, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0.5f, 0, -0.5f}, {-0.5f, 0, -0.5f}, {0, 1, 0});
    pushQuad(mesh, {-0.5f, -0.1f, -0.5f}, {0.5f, -0.1f, -0.5f}, {0.5f, -0.1f, 0.5f}, {-0.5f, -0.1f, 0.5f}, {0, -1, 0});
    pushQuad(mesh, {-0.5f, -0.1f, 0.5f}, {0.5f, -0.1f, 0.5f}, {0.5f, 0, 0.5f}, {-0.5f, 0, 0.5f}, {0, 0, 1});
    pushQuad(mesh, {0.5f, -0.1f, -0.5f}, {-0.5f, -0.1f, -0.5f}, {-0.5f, 0, -0.5f}, {0.5f, 0, -0.5f}, {0, 0, -1});
    pushQuad(mesh, {0.5f, -0.1f, 0.5f}, {0.5f, -0.1f, -0.5f}, {0.5f, 0, -0.5f}, {0.5f, 0, 0.5f}, {1, 0, 0});
    pushQuad(mesh, {-0.5f, -0.1f, -0.5f}, {-0.5f, -0.1f, 0.5f}, {-0.5f, 0, 0.5f}, {-0.5f, 0, -0.5f}, {-1, 0, 0});
    return mesh;
}

MeshData makeMarkerGeometry() { return makeCubeGeometry(); }

MeshData geometryFor(ObjectType type) {
    switch (type) {
        case ObjectType::Cube: return makeCubeGeometry();
        case ObjectType::Sphere: return makeSphereGeometry();
        case ObjectType::Cylinder: return makeCylinderGeometry();
        case ObjectType::Cone: return makeConeGeometry();
        case ObjectType::Torus: return makeTorusGeometry();
        case ObjectType::Plane: return makeGroundGeometry();
        case ObjectType::Camera:
        case ObjectType::Empty: return makeMarkerGeometry();
        case ObjectType::DirectionalLight:
        case ObjectType::PointLight: return makeMarkerGeometry();
    }
    return makeCubeGeometry();
}

} // namespace askofa
