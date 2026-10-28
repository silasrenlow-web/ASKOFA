// ASKOFA Engine - our own OpenGL function loader. No GLAD, no GLEW:
// we ask the platform (GLFW) for each core-profile function pointer
// ourselves and store them in `gl`. This is what "writing the graphics
// ourselves" actually looks like under the hood.
#pragma once

namespace askofa::gl {

// Types (matching the OpenGL spec / glext headers, minimal subset).
using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLchar = char;
using GLsizeiptr = long long;
using GLfloat = float;
using GLvoid = void;
using GLintptr = long long;

inline constexpr GLenum COLOR_BUFFER_BIT = 0x00004000;
inline constexpr GLenum DEPTH_BUFFER_BIT = 0x00000100;
inline constexpr GLenum DEPTH_TEST = 0x0B71;
inline constexpr GLenum TRIANGLES = 0x0004;
inline constexpr GLenum LINES = 0x0001;
inline constexpr GLenum ARRAY_BUFFER = 0x8892;
inline constexpr GLenum ELEMENT_ARRAY_BUFFER = 0x8893;
inline constexpr GLenum STATIC_DRAW = 0x88E4;
inline constexpr GLenum FLOAT = 0x1406;
inline constexpr GLenum UNSIGNED_INT = 0x1405;
inline constexpr GLenum FALSE = 0;
inline constexpr GLenum TRUE = 1;
inline constexpr GLenum VERTEX_SHADER = 0x8B31;
inline constexpr GLenum FRAGMENT_SHADER = 0x8B30;
inline constexpr GLenum COMPILE_STATUS = 0x8B81;
inline constexpr GLenum LINK_STATUS = 0x8B82;
inline constexpr GLenum CULL_FACE = 0x0B44;
inline constexpr GLenum BACK = 0x0405;
inline constexpr GLenum CCW = 0x0901;
inline constexpr GLenum LEQUAL = 0x0203;

// Function pointers, filled by loadGL().
inline void (*clearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
inline void (*clear)(GLbitfield) = nullptr;
inline void (*viewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;
inline void (*enable)(GLenum) = nullptr;
inline void (*disable)(GLenum) = nullptr;
inline void (*cullFace)(GLenum) = nullptr;
inline void (*frontFace)(GLenum) = nullptr;
inline void (*depthFunc)(GLenum) = nullptr;

inline void (*genVertexArrays)(GLsizei, GLuint*) = nullptr;
inline void (*bindVertexArray)(GLuint) = nullptr;
inline void (*deleteVertexArrays)(GLsizei, const GLuint*) = nullptr;
inline void (*genBuffers)(GLsizei, GLuint*) = nullptr;
inline void (*bindBuffer)(GLenum, GLuint) = nullptr;
inline void (*bufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum) = nullptr;
inline void (*deleteBuffers)(GLsizei, const GLuint*) = nullptr;
inline void (*vertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*) = nullptr;
inline void (*enableVertexAttribArray)(GLuint) = nullptr;

inline GLuint (*createShader)(GLenum) = nullptr;
inline void (*shaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
inline void (*compileShader)(GLuint) = nullptr;
inline void (*getShaderiv)(GLuint, GLenum, GLint*) = nullptr;
inline void (*getShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
inline void (*deleteShader)(GLuint) = nullptr;
inline GLuint (*createProgram)() = nullptr;
inline void (*attachShader)(GLuint, GLuint) = nullptr;
inline void (*linkProgram)(GLuint) = nullptr;
inline void (*getProgramiv)(GLuint, GLenum, GLint*) = nullptr;
inline void (*getProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
inline void (*deleteProgram)(GLuint) = nullptr;
inline void (*useProgram)(GLuint) = nullptr;
inline GLint (*getUniformLocation)(GLuint, const GLchar*) = nullptr;
inline void (*uniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
inline void (*uniform3f)(GLint, GLfloat, GLfloat, GLfloat) = nullptr;
inline void (*uniform3fv)(GLint, GLsizei, const GLfloat*) = nullptr;
inline void (*uniform1f)(GLint, GLfloat) = nullptr;

inline void (*drawElements)(GLenum, GLsizei, GLenum, const GLvoid*) = nullptr;
inline void (*drawArrays)(GLenum, GLint, GLsizei) = nullptr;

// Loads every pointer via a platform callback (glfwGetProcAddress).
// Returns false (and names the function in `missing`) if one is absent.
bool loadGL(void* (*getProcAddress)(const char* name), const char*& missing);

} // namespace askofa::gl
