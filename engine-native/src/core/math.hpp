// ASKOFA Engine - core math. Written from scratch, no math library.
// Column-major Mat4 (same layout OpenGL expects), angles in degrees at the
// scene level and radians inside the math functions where noted.
#pragma once

#include <cmath>

namespace askofa {

constexpr float kPi = 3.14159265358979323846f;
inline float degToRad(float d) { return d * kPi / 180.0f; }
inline float radToDeg(float r) { return r * 180.0f / kPi; }

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-8f) return {0, 0, 0};
        return {x / len, y / len, z / len};
    }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Column-major 4x4 matrix: m[col * 4 + row].
struct Mat4 {
    float m[16] = {};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
    static Mat4 translation(const Vec3& t) {
        Mat4 r = identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }
    static Mat4 scaling(const Vec3& s) {
        Mat4 r = identity();
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
        return r;
    }
    static Mat4 rotationX(float rad) {
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
        return r;
    }
    static Mat4 rotationY(float rad) {
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
        return r;
    }
    static Mat4 rotationZ(float rad) {
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
        return r;
    }
    // Euler rotation in degrees, applied Z * Y * X (yaw/pitch/roll feel).
    static Mat4 eulerDeg(const Vec3& deg) {
        return rotationY(degToRad(deg.y)) * rotationX(degToRad(deg.x)) * rotationZ(degToRad(deg.z));
    }
    static Mat4 perspective(float fovDeg, float aspect, float nearPlane, float farPlane) {
        Mat4 r;
        float f = 1.0f / std::tan(degToRad(fovDeg) * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
        return r;
    }
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 f = (target - eye).normalized();
        Vec3 s = cross(f, up).normalized();
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8] = s.z;
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9] = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -dot(s, eye);
        r.m[13] = -dot(u, eye);
        r.m[14] = dot(f, eye);
        return r;
    }
    // Compose a model matrix from position / euler degrees / scale.
    static Mat4 trs(const Vec3& pos, const Vec3& rotDeg, const Vec3& scale) {
        return translation(pos) * eulerDeg(rotDeg) * scaling(scale);
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) sum += m[k * 4 + row] * o.m[col * 4 + k];
                r.m[col * 4 + row] = sum;
            }
        }
        return r;
    }
    // Transform a point (w = 1, with perspective divide).
    Vec3 transformPoint(const Vec3& p) const {
        float x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
        float y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
        float z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
        float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        if (std::fabs(w) > 1e-8f) { x /= w; y /= w; z /= w; }
        return {x, y, z};
    }
};

} // namespace askofa
