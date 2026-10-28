cmake_minimum_required(VERSION 3.20)
project(askofa_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# --- core: math, JSON, scene, geometry (no graphics, fully testable) ---
add_library(askofa_core
  src/core/json.cpp
  src/core/scene.cpp
  src/core/geometry.cpp
)
target_include_directories(askofa_core PUBLIC src/core)

# --- tests ---
include(CTest)
if(BUILD_TESTING)
  add_executable(askofa_tests tests/core_tests.cpp)
  target_link_libraries(askofa_tests PRIVATE askofa_core)
  add_test(NAME core_tests COMMAND askofa_tests)
endif()

# --- app: window (GLFW) + our OpenGL renderer ---
option(ASKOFA_BUILD_APP "Build the native engine app (needs GLFW)" ON)
if(ASKOFA_BUILD_APP)
  find_package(glfw3 3.3 QUIET)
  if(NOT glfw3_FOUND)
    include(FetchContent)
    FetchContent_Declare(glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
  endif()

  add_executable(askofa_engine
    src/main.cpp
    src/renderer/gl_loader.cpp
    src/renderer/renderer.cpp
  )
  target_include_directories(askofa_engine PRIVATE src)
  target_compile_definitions(askofa_engine PRIVATE GLFW_INCLUDE_NONE=1)
  target_link_libraries(askofa_engine PRIVATE askofa_core glfw)
  if(WIN32)
    target_link_libraries(askofa_engine PRIVATE opengl32)
  elseif(APPLE)
    target_link_libraries(askofa_engine PRIVATE "-framework OpenGL")
  else()
    target_link_libraries(askofa_engine PRIVATE GL dl)
  endif()
endif()
