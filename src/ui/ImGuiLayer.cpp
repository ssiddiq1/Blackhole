// ImGuiLayer.cpp
#include <OpenGL/gl3.h>

#include "ui/ImGuiLayer.hpp"
#include "sim/Scene.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

void ImGuiLayer::init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style  = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding  = 3.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.88f;

    ImGui_ImplGlfw_InitForOpenGL(window, /*install_callbacks=*/true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void ImGuiLayer::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::begin_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

bool ImGuiLayer::draw_panels(bh::Scene& scene, float fps,
    CameraParams&  cam_params,
    DiskVisParams& disk,
    PostParams&    post,
    std::function<void()> on_spawn_particle,
    std::function<void()> on_spawn_dumbbell,
    std::function<void()> on_clear)
{
    bool cam_changed = false;
    const auto& cfg = scene.config();

    // ── Controls ─────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(270, 260), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(12,  12),  ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("Black Hole");
    float M = cfg.M;
    if (ImGui::SliderFloat("Mass M", &M, 0.2f, 4.0f, "%.2f"))
        scene.set_mass(M);
    ImGui::Text("rs = %.2f   ISCO = %.2f", 2.0f*cfg.M, 6.0f*cfg.M);

    ImGui::SeparatorText("Accretion Disk");
    bool disk_on = cfg.disk_on;
    if (ImGui::Checkbox("Enabled", &disk_on))
        scene.set_disk_on(disk_on);

    float r_in  = cfg.disk_r_in;
    float r_out = cfg.disk_r_out;
    float r_in_min = 2.0f * cfg.M + 0.5f;
    if (ImGui::SliderFloat("r_in",  &r_in,  r_in_min, 30.0f))
        scene.set_disk_radii(r_in, cfg.disk_r_out);
    if (ImGui::SliderFloat("r_out", &r_out, r_in + 1.0f, 60.0f))
        scene.set_disk_radii(cfg.disk_r_in, r_out);

    ImGui::SeparatorText("Spawn");
    ImGui::TextWrapped("Mouse captured: LClick=particle, RClick=dumbbell");
    if (ImGui::Button("Spawn Particle"))  on_spawn_particle();
    ImGui::SameLine();
    if (ImGui::Button("Spawn Dumbbell")) on_spawn_dumbbell();
    if (ImGui::Button("Clear All"))      on_clear();
    ImGui::Text("Active: %d", scene.particles().count());

    ImGui::End();

    // ── Camera ────────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(270, 120), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(12, 280),  ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_NoCollapse);

    if (ImGui::SliderFloat("Elevation°", &cam_params.elevation, 0.0f, 45.0f, "%.1f°"))
        cam_changed = true;
    if (ImGui::SliderFloat("Orbit r",    &cam_params.orbit_r,   5.0f, 50.0f, "%.1f M"))
        cam_changed = true;
    if (ImGui::SliderFloat("FOV",        &cam_params.fov_deg,  30.0f, 120.0f, "%.0f°"))
        cam_changed = true;

    ImGui::End();

    // ── Disk ──────────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(270, 195), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(12, 408),  ImGuiCond_FirstUseEver);
    ImGui::Begin("Disk", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::SliderFloat("base_gain",  &disk.base_gain, 10.0f,  1000.0f,  "%.0f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("T_peak (K)", &disk.T_peak,  1000.0f, 100000.0f, "%.0f K",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("turb_amp",   &disk.turb_amp,         0.0f,  1.0f,  "%.2f");
    ImGui::SliderFloat("h_scale",    &disk.h_scale,          0.01f, 0.2f,  "%.3f");
    ImGui::SliderFloat("noise_lod",  &disk.adisk_noise_lod,  1.0f,  12.0f, "%.1f");
    ImGui::SliderFloat("anim_speed", &disk.adisk_speed,      0.0f,  1.0f,  "%.2f");

    ImGui::End();

    // ── Post ──────────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(270, 140), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(12, 556),  ImGuiCond_FirstUseEver);
    ImGui::Begin("Post", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::SliderFloat("Exposure",        &post.exposure,        -2.0f, 3.0f,  "%.2f stops");
    ImGui::SliderFloat("Bloom intensity", &post.bloom_intensity,  0.0f, 3.0f,  "%.2f");
    ImGui::SliderFloat("Bloom threshold", &post.bloom_threshold,  0.5f, 2.0f,  "%.2f");
    bool aces_on = (post.tonemap_on != 0);
    if (ImGui::Checkbox("ACES tonemap", &aces_on))
        post.tonemap_on = aces_on ? 1 : 0;

    ImGui::End();

    // ── Stats ─────────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(180, 52), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(12, 704), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stats", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("FPS: %.0f", fps);
    ImGui::End();

    return cam_changed;
}

void ImGuiLayer::end_frame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::wants_mouse()    const { return ImGui::GetIO().WantCaptureMouse; }
bool ImGuiLayer::wants_keyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }
