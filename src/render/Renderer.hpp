#pragma once
// Renderer.hpp — two-pass renderer.
//
//  Pass A: geodesic.frag writes HDR radiance → RGBA16F FBO.
//  Pass B: bloom extract/blur (2 half-res passes) + ACES composite → default FB.
//  Pass C: particle trail line strips on default FB.

#include <string>
#include <vector>
#include <utility>
#include <glm/glm.hpp>

// ── Scene uniform block — mirrors layout(std140) uniform SceneUBO ─────────
struct SceneUBO {
    glm::vec3 cam_pos;    float M;           // bytes  0–15
    glm::vec3 cam_fwd;    float rs;          // bytes 16–31
    glm::vec3 cam_right;  float dphi;        // bytes 32–47
    glm::vec3 cam_up;     float fov_tan;     // bytes 48–63
    int32_t   width, height, max_steps, disk_on;   // bytes 64–79
    float     disk_r_in, disk_r_out;               // bytes 80–87
    float     base_gain,  T_peak;                  // bytes 88–95
    float     turb_amp,   h_scale;                 // bytes 96–103
    float     adisk_noise_lod; float adisk_speed;  // bytes 104–111
};
static_assert(sizeof(SceneUBO) == 112, "SceneUBO size mismatch");
static_assert(alignof(glm::vec3) == 4, "glm::vec3 must be 4-byte aligned");

// ── Post-processing parameters (regular uniforms in post.frag) ────────────
struct PostParams {
    float exposure        =  0.0f;   // stops, range −2 … +3
    float bloom_intensity =  0.6f;   // range 0 … 3
    float bloom_threshold =  0.8f;   // luminance threshold 0.5 … 2.0
    int   tonemap_on      =  1;      // 1 = ACES filmic
};


// ── Disk visual parameters (fill SceneUBO fields) ─────────────────────────
struct DiskVisParams {
    float base_gain       = 300.0f;   // emission scale, range 10–1000
    float T_peak          = 12000.0f; // blackbody peak at ISCO (K), range 1e3–1e5
    float turb_amp        =   0.4f;   // simplex fBm amplitude modifier, 0–1
    float h_scale         =   0.08f;  // disk half-height / r, 0.01–0.2
    float adisk_noise_lod =   5.0f;   // noise octave count, 1–12
    float adisk_speed     =   0.5f;   // disk animation speed, 0–1
};

// ── Particle trail vertex ─────────────────────────────────────────────────
struct TrailVertex {
    glm::vec3 pos;
    float     age;   // 0 = newest, 1 = oldest
    float     type;  // 0=particle  1=db-p0  2=db-p1  3=broken
};
static_assert(sizeof(TrailVertex) == 20, "TrailVertex size changed");

class Renderer {
public:
    explicit Renderer(const std::string& shader_dir, int width, int height);
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Full render sequence: geodesic HDR → bloom → composite.
    // elapsed_time drives disk animation in the geodesic shader.
    void render(const SceneUBO& scene, const PostParams& post, float elapsed_time);

    // Draw alpha-blended particle trails on the default framebuffer.
    void render_particles(const std::vector<TrailVertex>&         verts,
                          const std::vector<std::pair<int,int>>&  strips,
                          const glm::mat4&                         mvp);

    // Recreates all FBO textures at the new size.
    void resize(int width, int height);

    int width()  const { return m_width; }
    int height() const { return m_height; }

private:
    // ── Geodesic pass ─────────────────────────────────────────────────────
    unsigned int m_geo_prog    = 0;
    unsigned int m_geo_vao     = 0;
    unsigned int m_ubo         = 0;
    unsigned int m_galaxy_tex  = 0;   // cubemap (nebula skybox)
    unsigned int m_color_map_tex = 0; // 2D color gradient for disk tint
    int          m_u_galaxy    = -1;
    int          m_u_color_map = -1;
    int          m_u_time      = -1;

    static constexpr int kBloom = 8;

    // ── HDR FBO ───────────────────────────────────────────────────────────
    unsigned int m_fbo_hdr = 0;
    unsigned int m_tex_hdr = 0;   // RGBA16F, full-res

    // ── Bloom pyramid ─────────────────────────────────────────────────────
    unsigned int m_fbo_bright = 0;
    unsigned int m_tex_bright = 0;            // bright extract, full-res
    unsigned int m_fbo_down[kBloom] = {};
    unsigned int m_tex_down[kBloom] = {};
    unsigned int m_fbo_up  [kBloom] = {};
    unsigned int m_tex_up  [kBloom] = {};
    int          m_down_w[kBloom]   = {};
    int          m_down_h[kBloom]   = {};

    // ── Bloom programs ────────────────────────────────────────────────────
    unsigned int m_bright_prog  = 0;
    int          m_bright_u_src = -1;
    int          m_bright_u_vp  = -1;
    int          m_bright_u_thr = -1;

    unsigned int m_down_prog  = 0;
    int          m_down_u_src = -1;
    int          m_down_u_vp  = -1;

    unsigned int m_up_prog     = 0;
    int          m_up_u_blur   = -1;
    int          m_up_u_detail = -1;
    int          m_up_u_vp     = -1;

    // ── Composite (post) pass ─────────────────────────────────────────────
    unsigned int m_post_prog  = 0;
    unsigned int m_post_vao   = 0;

    int m_u_hdr_tex   = -1;
    int m_u_bloom_tex = -1;
    int m_u_vp_size   = -1;
    int m_u_exposure  = -1;
    int m_u_bloom_int = -1;
    int m_u_tonemap   = -1;

    // ── Particle trail pass ───────────────────────────────────────────────
    unsigned int m_trail_prog = 0;
    unsigned int m_trail_vao  = 0;
    unsigned int m_trail_vbo  = 0;
    int          m_mvp_loc    = -1;

    int m_width  = 0;
    int m_height = 0;

    // ── Helpers ───────────────────────────────────────────────────────────
    void init_fbos(int w, int h);
    void destroy_fbos();

    static unsigned int compile_shader(unsigned int type, const std::string& path);
    static unsigned int link_program  (unsigned int vert, unsigned int frag);
};
