// main.cpp — Application entry point.
//
// Wires together Window, Renderer, Scene, and ImGuiLayer.
// Physics constants: G=c=1, M=1.  Black hole at the origin.
// Camera default: elevation=15°, orbit_r=20M → pos=(0, 5.176, 19.319)
//
// Controls:
//   WASD + Space/Shift  — move camera
//   Mouse drag          — look (when captured)
//   Escape              — toggle mouse capture / ImGui interaction
//   LClick (captured)   — spawn test particle at camera position
//   RClick (captured)   — spawn dumbbell aimed toward BH
//   Q                   — quit

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>

#include "app/Window.hpp"
#include "render/Renderer.hpp"
#include "sim/Scene.hpp"
#include "ui/ImGuiLayer.hpp"
#include "physics/Schwarzschild.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#ifndef BH_SHADER_DIR
#error "BH_SHADER_DIR must be defined by CMake"
#endif

// ── Camera ─────────────────────────────────────────────────────────────────
struct Camera {
    glm::vec3 pos   = glm::vec3(0.0f, 5.176f, 19.319f);
    float     yaw   = 0.0f;    // degrees; 0 → looking toward −z (BH)
    float     pitch = -15.0f;  // degrees; negative = looking slightly down

    glm::vec3 forward() const {
        float y = glm::radians(yaw);
        float p = glm::radians(pitch);
        return glm::normalize(glm::vec3(
            std::cos(p) * std::sin(y),
            std::sin(p),
           -std::cos(p) * std::cos(y)));
    }
    glm::vec3 right() const {
        return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    glm::vec3 up() const {
        return glm::normalize(glm::cross(right(), forward()));
    }
};

// ── Recompute camera position/orientation from orbital parameters ───────────
static void apply_cam_params(Camera& cam, const CameraParams& p)
{
    float elev_rad = glm::radians(p.elevation);
    cam.pos   = glm::vec3(0.0f,
                          p.orbit_r * std::sin(elev_rad),
                          p.orbit_r * std::cos(elev_rad));
    cam.pitch = -p.elevation;
    cam.yaw   = 0.0f;
}

// ── Trail buffer builder ────────────────────────────────────────────────────
static void build_trail_buffers(
    const bh::ParticleSystem&              psys,
    std::vector<TrailVertex>&              verts,
    std::vector<std::pair<int,int>>&       strips)
{
    verts.clear();
    strips.clear();

    auto add_trail = [&](const bh::Particle& p, float type_id) {
        if (p.trail_count < 2) return;
        int start = (int)verts.size();
        for (int k = 0; k < p.trail_count; ++k) {
            int idx = (p.trail_head - p.trail_count + k + bh::TRAIL_CAP) % bh::TRAIL_CAP;
            float age = 1.0f - (float)k / (float)(p.trail_count - 1);  // 1=oldest, 0=newest
            verts.push_back({ p.trail[idx], age, type_id });
        }
        strips.emplace_back(start, (int)verts.size() - start);
    };

    for (const auto& p : psys.particles())
        add_trail(p, 0.0f);

    for (const auto& db : psys.dumbbells()) {
        float type_id = db.broken ? 3.0f : 1.0f;
        add_trail(db.p0, type_id);
        add_trail(db.p1, db.broken ? 3.0f : 2.0f);
    }
}

// ── Main ────────────────────────────────────────────────────────────────────
int main()
{
    constexpr int INIT_W = 1280;
    constexpr int INIT_H =  720;

    Window   window(INIT_W, INIT_H, "Schwarzschild Black Hole");
    Renderer renderer(std::string(BH_SHADER_DIR), window.width(), window.height());
    bh::Scene scene;
    ImGuiLayer imgui;

    window.on_resize = [&](int w, int h) { renderer.resize(w, h); };

    imgui.init(glfwGetCurrentContext());

    // ── Per-frame parameters ───────────────────────────────────────────────
    CameraParams  cam_params;                   // elevation=15°, orbit_r=20M, fov=60°
    DiskVisParams disk_vis;                     // base_gain=300, T_peak=12000, turb_amp=0.4, h_scale=0.08
    PostParams    post_params;                  // exposure=0, bloom_int=0.6, bloom_thr=0.8, aces=on

    Camera cam;                                 // initialized from cam_params defaults above
    // cam already matches cam_params defaults via Camera struct initializers

    int   max_steps = 1000;
    float dphi      = 0.02f;

    window.set_mouse_captured(true);

    bool prev_left  = false;
    bool prev_right = false;

    std::vector<TrailVertex>         trail_verts;
    std::vector<std::pair<int,int>>  trail_strips;

    double t_last    = glfwGetTime();
    double fps_acc   = 0.0;
    int    fps_frames = 0;
    float  smooth_fps = 0.0f;

    while (!window.should_close()) {
        double t_now = glfwGetTime();
        float  dt    = (float)(t_now - t_last);
        t_last       = t_now;

        fps_acc += dt;
        ++fps_frames;
        if (fps_acc >= 1.0) {
            smooth_fps = (float)fps_frames / (float)fps_acc;
            std::cout << "FPS: " << (int)smooth_fps
                      << "  r=" << glm::length(cam.pos) << "M" << std::endl;
            fps_acc    = 0.0;
            fps_frames = 0;
        }

        window.poll_events();
        imgui.begin_frame();

        // ── Camera mouse look (only when captured) ─────────────────────────
        {
            double dx, dy;
            window.consume_mouse_delta(dx, dy);
            const float sensitivity = 0.12f;
            cam.yaw   += (float)dx * sensitivity;
            cam.pitch -= (float)dy * sensitivity;
            cam.pitch  = glm::clamp(cam.pitch, -89.0f, 89.0f);
        }

        // ── Camera WASD movement ───────────────────────────────────────────
        if (!imgui.wants_keyboard()) {
            float r     = glm::length(cam.pos);
            float speed = glm::clamp(r * 0.5f, 0.5f, 50.0f) * dt;
            if (window.key(GLFW_KEY_W))          cam.pos += cam.forward() * speed;
            if (window.key(GLFW_KEY_S))          cam.pos -= cam.forward() * speed;
            if (window.key(GLFW_KEY_D))          cam.pos += cam.right()   * speed;
            if (window.key(GLFW_KEY_A))          cam.pos -= cam.right()   * speed;
            if (window.key(GLFW_KEY_SPACE))      cam.pos += cam.up()      * speed;
            if (window.key(GLFW_KEY_LEFT_SHIFT)) cam.pos -= cam.up()      * speed;
            if (window.key(GLFW_KEY_Q)) break;
        }

        // ── Click-to-spawn (only when mouse is captured) ───────────────────
        if (window.mouse_captured() && !imgui.wants_mouse()) {
            bool left  = window.mouse_button(GLFW_MOUSE_BUTTON_LEFT);
            bool right = window.mouse_button(GLFW_MOUSE_BUTTON_RIGHT);

            if (left && !prev_left)
                scene.spawn_particle(cam.pos, cam.forward());

            if (right && !prev_right) {
                float r0 = glm::length(cam.pos);
                glm::vec3 db_pos = glm::normalize(cam.pos) *
                    glm::clamp(0.4f * r0, 3.5f * scene.config().M, r0 * 0.9f);
                scene.spawn_dumbbell(db_pos);
            }

            prev_left  = left;
            prev_right = right;
        } else {
            prev_left  = false;
            prev_right = false;
        }

        // ── Simulation step ────────────────────────────────────────────────
        scene.update(dt);

        // ── Build SceneUBO ─────────────────────────────────────────────────
        {
            const auto& cfg = scene.config();
            int fb_w = renderer.width();
            int fb_h = renderer.height();

            SceneUBO s{};
            s.cam_pos    = cam.pos;
            s.M          = cfg.M;
            s.cam_fwd    = cam.forward();
            s.rs         = (float)bh::rs(cfg.M);
            s.cam_right  = cam.right();
            s.dphi       = dphi;
            s.cam_up     = cam.up();
            s.fov_tan    = std::tan(glm::radians(cam_params.fov_deg) * 0.5f);
            s.width      = fb_w;
            s.height     = fb_h;
            s.max_steps  = max_steps;
            s.disk_on    = cfg.disk_on ? 1 : 0;
            s.disk_r_in  = cfg.disk_r_in;
            s.disk_r_out = cfg.disk_r_out;
            s.base_gain  = disk_vis.base_gain;
            s.T_peak     = disk_vis.T_peak;
            s.turb_amp        = disk_vis.turb_amp;
            s.h_scale         = disk_vis.h_scale;
            s.adisk_noise_lod = disk_vis.adisk_noise_lod;
            s.adisk_speed     = disk_vis.adisk_speed;

            renderer.render(s, post_params, (float)t_now);
        }

        // ── Render particle trails ─────────────────────────────────────────
        {
            float fov_rad = glm::radians(cam_params.fov_deg);
            float aspect  = (float)renderer.width() / (float)renderer.height();
            glm::mat4 view = glm::lookAt(cam.pos, cam.pos + cam.forward(), cam.up());
            glm::mat4 proj = glm::perspective(fov_rad, aspect, 0.1f, 1000.0f);
            glm::mat4 mvp  = proj * view;

            build_trail_buffers(scene.particles(), trail_verts, trail_strips);
            renderer.render_particles(trail_verts, trail_strips, mvp);
        }

        // ── ImGui ──────────────────────────────────────────────────────────
        bool cam_changed = imgui.draw_panels(scene, smooth_fps,
            cam_params, disk_vis, post_params,
            /*on_spawn_particle=*/[&]() {
                scene.spawn_particle(cam.pos, cam.forward());
            },
            /*on_spawn_dumbbell=*/[&]() {
                float r0 = glm::length(cam.pos);
                glm::vec3 db_pos = glm::normalize(cam.pos) *
                    glm::clamp(0.4f * r0, 3.5f * scene.config().M, r0 * 0.9f);
                scene.spawn_dumbbell(db_pos);
            },
            /*on_clear=*/[&]() { scene.clear(); }
        );

        if (cam_changed)
            apply_cam_params(cam, cam_params);

        imgui.end_frame();

        window.swap_buffers();
    }

    imgui.shutdown();
    return 0;
}
