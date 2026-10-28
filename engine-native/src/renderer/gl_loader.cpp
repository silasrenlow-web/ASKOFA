#include "gl_loader.hpp"

#include <type_traits>

namespace askofa::gl {

bool loadGL(void* (*getProcAddress)(const char*), const char*& missing) {
    auto load = [&](auto& fn, const char* name) -> bool {
        fn = reinterpret_cast<std::decay_t<decltype(fn)>>(getProcAddress(name));
        if (!fn) { missing = name; return false; }
        return true;
    };
    return load(clearColor, "glClearColor") && load(clear, "glClear") &&
           load(viewport, "glViewport") && load(enable, "glEnable") &&
           load(disable, "glDisable") && load(cullFace, "glCullFace") &&
           load(frontFace, "glFrontFace") && load(depthFunc, "glDepthFunc") &&
           load(genVertexArrays, "glGenVertexArrays") && load(bindVertexArray, "glBindVertexArray") &&
           load(deleteVertexArrays, "glDeleteVertexArrays") && load(genBuffers, "glGenBuffers") &&
           load(bindBuffer, "glBindBuffer") && load(bufferData, "glBufferData") &&
           load(deleteBuffers, "glDeleteBuffers") && load(vertexAttribPointer, "glVertexAttribPointer") &&
           load(enableVertexAttribArray, "glEnableVertexAttribArray") &&
           load(createShader, "glCreateShader") && load(shaderSource, "glShaderSource") &&
           load(compileShader, "glCompileShader") && load(getShaderiv, "glGetShaderiv") &&
           load(getShaderInfoLog, "glGetShaderInfoLog") && load(deleteShader, "glDeleteShader") &&
           load(createProgram, "glCreateProgram") && load(attachShader, "glAttachShader") &&
           load(linkProgram, "glLinkProgram") && load(getProgramiv, "glGetProgramiv") &&
           load(getProgramInfoLog, "glGetProgramInfoLog") && load(deleteProgram, "glDeleteProgram") &&
           load(useProgram, "glUseProgram") && load(getUniformLocation, "glGetUniformLocation") &&
           load(uniformMatrix4fv, "glUniformMatrix4fv") && load(uniform3f, "glUniform3f") &&
           load(uniform3fv, "glUniform3fv") && load(uniform1f, "glUniform1f") &&
           load(drawElements, "glDrawElements") && load(drawArrays, "glDrawArrays");
}

} // namespace askofa::gl
