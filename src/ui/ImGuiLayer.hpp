#pragma once
// ImGuiLayer.hpp — Dear ImGui wrapper (docking branch, OpenGL3 + GLFW backends).
//
// Panels:
//   "Controls" — BH mass, disk on/off + radii, spawn buttons, clear.
//   "Camera"   — elevation, orbit radius, FOV.
//   "Disk"     — base_gain, T_peak, turb_amp, h_scale.
//   "Post"     — exposure, bloom intensity/threshold, tonemap toggle.
//   "Stats"    — live FPS.

#include <functional>
#include "render/Renderer.hpp"   // PostParams, DiskVisParams

struct GLFWwindow;

namespace bh { class Scene; }

// ── Camera orbital parameters ─────────────────────────────────────────────
struct CameraParams {
    float elevation = 15.0f;   // degrees above equatorial plane, 0–45
    float orbit_r   = 20.0f;   // distance from BH in M units, 5–50
    float fov_deg   = 60.0f;   // vertical field of view in degrees, 30–120
};

class ImGuiLayer {
public:
    // Call once after the GL context is current.
    void init(GLFWwindow* window);
    void shutdown();

    // Call at the start of each frame, after glfwPollEvents.
    void begin_frame();

    // Render all panels.  Returns true if camera params changed (caller should
    // recompute cam.pos / cam.pitch from cam_params).
    bool draw_panels(bh::Scene& scene, float fps,
                     CameraParams&  cam_params,
                     DiskVisParams& disk,
                     PostParams&    post,
                     std::function<void()> on_spawn_particle,
                     std::function<void()> on_spawn_dumbbell,
                     std::function<void()> on_clear);

    // Call at the end of the frame, before swapping buffers.
    void end_frame();

    bool wants_mouse()    const;
    bool wants_keyboard() const;
};
