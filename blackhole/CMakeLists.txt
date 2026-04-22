cmake_minimum_required(VERSION 3.24)
project(BlackholeSim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(cmake/deps.cmake)
enable_testing()

# ── Physics library (no GPU deps) ──────────────────────────────────────────
add_library(bh_physics STATIC
    src/physics/Geodesic.cpp
)
target_include_directories(bh_physics PUBLIC src)
target_link_libraries(bh_physics PUBLIC glm::glm)

# ── Simulation library ─────────────────────────────────────────────────────
add_library(bh_sim STATIC
    src/sim/ParticleSystem.cpp
    src/sim/Scene.cpp
)
target_include_directories(bh_sim PUBLIC src)
target_link_libraries(bh_sim PUBLIC bh_physics glm::glm)

# ── Tests ──────────────────────────────────────────────────────────────────
add_executable(bh_tests tests/test_geodesic.cpp)
target_link_libraries(bh_tests PRIVATE bh_physics bh_sim doctest::doctest)
add_test(NAME physics_tests COMMAND bh_tests)

# ── Dear ImGui backends (GLFW + OpenGL3) ───────────────────────────────────
# The core imgui target is created by FetchContent_MakeAvailable(imgui) in deps.cmake.
# We add the backend sources and include paths here.
target_sources(imgui PRIVATE
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)
if(APPLE)
    target_link_libraries(imgui PUBLIC "-framework OpenGL")
endif()

# ── Application ────────────────────────────────────────────────────────────
add_executable(bh_app
    src/app/main.cpp
    src/app/Window.cpp
    src/render/Renderer.cpp
    src/ui/ImGuiLayer.cpp
)
target_include_directories(bh_app PRIVATE src)
target_compile_definitions(bh_app PRIVATE
    BH_SHADER_DIR="${CMAKE_SOURCE_DIR}/src/render/shaders"
)
target_link_libraries(bh_app PRIVATE bh_sim glfw glm::glm imgui)

if(APPLE)
    target_link_libraries(bh_app PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
    target_compile_definitions(bh_app PRIVATE GL_SILENCE_DEPRECATION)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(bh_app PRIVATE OpenGL::GL)
endif()
