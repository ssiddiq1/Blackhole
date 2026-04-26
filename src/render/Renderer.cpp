// Renderer.cpp
#include <OpenGL/gl3.h>

#include "render/Renderer.hpp"
#include "render/TextureLoader.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cstddef>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

#ifndef BH_ASSET_DIR
#error "BH_ASSET_DIR must be defined by CMake"
#endif

// ── Layout sanity ──────────────────────────────────────────────────────────
static_assert(offsetof(SceneUBO, M)         == 12);
static_assert(offsetof(SceneUBO, rs)        == 28);
static_assert(offsetof(SceneUBO, dphi)      == 44);
static_assert(offsetof(SceneUBO, fov_tan)   == 60);
static_assert(offsetof(SceneUBO, width)     == 64);
static_assert(offsetof(SceneUBO, max_steps) == 72);
static_assert(offsetof(SceneUBO, disk_on)   == 76);
static_assert(offsetof(SceneUBO, disk_r_in) == 80);
static_assert(offsetof(SceneUBO, base_gain) == 88);
static_assert(offsetof(SceneUBO, turb_amp)  == 96);

// ── helpers ────────────────────────────────────────────────────────────────
static std::string read_file(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Renderer: cannot open shader: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool check_gl_errors(const char* ctx)
{
    bool found = false;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[GL] " << ctx << " 0x" << std::hex << err << std::dec << "\n";
        found = true;
    }
    return found;
}

// ── shader compilation ─────────────────────────────────────────────────────
unsigned int Renderer::compile_shader(unsigned int type, const std::string& path)
{
    std::string src = read_file(path);
    const char* c   = src.c_str();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &c, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        glDeleteShader(sh);
        throw std::runtime_error("Shader compile (" + path + "):\n" + log);
    }
    return sh;
}

unsigned int Renderer::link_program(unsigned int vert, unsigned int frag)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        glDeleteProgram(prog);
        throw std::runtime_error("Shader link:\n" + log);
    }
    return prog;
}

// ── FBO helpers ────────────────────────────────────────────────────────────
static void make_fbo_texture(GLuint& fbo, GLuint& tex, int w, int h, GLenum ifmt)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Renderer: incomplete framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ── Renderer ──────────────────────────────────────────────────────────────
Renderer::Renderer(const std::string& shader_dir, int width, int height)
    : m_width(width), m_height(height)
{
    // ── Geodesic pass ─────────────────────────────────────────────────────
    {
        GLuint vert = compile_shader(GL_VERTEX_SHADER,   shader_dir + "/screen.vert");
        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, shader_dir + "/geodesic.frag");
        m_geo_prog  = link_program(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);

        GLuint bi = glGetUniformBlockIndex(m_geo_prog, "SceneUBO");
        if (bi == GL_INVALID_INDEX)
            throw std::runtime_error("'SceneUBO' block not found in geodesic.frag");
        glUniformBlockBinding(m_geo_prog, bi, 0);

        m_u_galaxy    = glGetUniformLocation(m_geo_prog, "u_galaxy");
        m_u_color_map = glGetUniformLocation(m_geo_prog, "u_color_map");
        m_u_time      = glGetUniformLocation(m_geo_prog, "u_time");

        glGenVertexArrays(1, &m_geo_vao);

        glGenBuffers(1, &m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        m_galaxy_tex    = load_cubemap(std::string(BH_ASSET_DIR) + "/skybox_nebula_dark");
        m_color_map_tex = load_texture_2d(std::string(BH_ASSET_DIR) + "/color_map.png");
    }

    // ── Bloom: brightness extract ─────────────────────────────────────────
    {
        GLuint vert = compile_shader(GL_VERTEX_SHADER,   shader_dir + "/post.vert");
        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, shader_dir + "/bloom_brightness.frag");
        m_bright_prog = link_program(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);
        m_bright_u_src = glGetUniformLocation(m_bright_prog, "u_src_tex");
        m_bright_u_vp  = glGetUniformLocation(m_bright_prog, "u_vp_size");
        m_bright_u_thr = glGetUniformLocation(m_bright_prog, "u_bloom_thr");
    }

    // ── Bloom: downsample ─────────────────────────────────────────────────
    {
        GLuint vert = compile_shader(GL_VERTEX_SHADER,   shader_dir + "/post.vert");
        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, shader_dir + "/bloom_downsample.frag");
        m_down_prog = link_program(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);
        m_down_u_src = glGetUniformLocation(m_down_prog, "u_src_tex");
        m_down_u_vp  = glGetUniformLocation(m_down_prog, "u_vp_size");
    }

    // ── Bloom: upsample ───────────────────────────────────────────────────
    {
        GLuint vert = compile_shader(GL_VERTEX_SHADER,   shader_dir + "/post.vert");
        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, shader_dir + "/bloom_upsample.frag");
        m_up_prog = link_program(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);
        m_up_u_blur   = glGetUniformLocation(m_up_prog, "u_blur_tex");
        m_up_u_detail = glGetUniformLocation(m_up_prog, "u_detail_tex");
        m_up_u_vp     = glGetUniformLocation(m_up_prog, "u_vp_size");
    }

    // ── Composite (post) pass ─────────────────────────────────────────────
    {
        GLuint vert = compile_shader(GL_VERTEX_SHADER,   shader_dir + "/post.vert");
        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, shader_dir + "/post.frag");
        m_post_prog = link_program(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);

        m_u_hdr_tex   = glGetUniformLocation(m_post_prog, "u_hdr_tex");
        m_u_bloom_tex = glGetUniformLocation(m_post_prog, "u_bloom_tex");
        m_u_vp_size   = glGetUniformLocation(m_post_prog, "u_vp_size");
        m_u_exposure  = glGetUniformLocation(m_post_prog, "u_exposure");
        m_u_bloom_int = glGetUniformLocation(m_post_prog, "u_bloom_int");
        m_u_tonemap   = glGetUniformLocation(m_post_prog, "u_tonemap_on");

        glGenVertexArrays(1, &m_post_vao);
    }

    // ── Particle trail pass ───────────────────────────────────────────────
    {
        GLuint vert = compile_shader(GL_VERTEX_SHADER,   shader_dir + "/particle_trail.vert");
        GLuint frag = compile_shader(GL_FRAGMENT_SHADER, shader_dir + "/particle_trail.frag");
        m_trail_prog = link_program(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);
        m_mvp_loc = glGetUniformLocation(m_trail_prog, "u_mvp");

        glGenVertexArrays(1, &m_trail_vao);
        glGenBuffers(1, &m_trail_vbo);

        glBindVertexArray(m_trail_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_trail_vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 20, (void*)12);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 20, (void*)16);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    init_fbos(width, height);
    check_gl_errors("Renderer::Renderer");
}

Renderer::~Renderer()
{
    destroy_fbos();
    if (m_geo_prog)       glDeleteProgram(m_geo_prog);
    if (m_geo_vao)        glDeleteVertexArrays(1, &m_geo_vao);
    if (m_ubo)            glDeleteBuffers(1, &m_ubo);
    if (m_galaxy_tex)     glDeleteTextures(1, &m_galaxy_tex);
    if (m_color_map_tex)  glDeleteTextures(1, &m_color_map_tex);
    if (m_bright_prog) glDeleteProgram(m_bright_prog);
    if (m_down_prog)   glDeleteProgram(m_down_prog);
    if (m_up_prog)     glDeleteProgram(m_up_prog);
    if (m_post_prog)   glDeleteProgram(m_post_prog);
    if (m_post_vao)    glDeleteVertexArrays(1, &m_post_vao);
    if (m_trail_prog) glDeleteProgram(m_trail_prog);
    if (m_trail_vao)  glDeleteVertexArrays(1, &m_trail_vao);
    if (m_trail_vbo)  glDeleteBuffers(1, &m_trail_vbo);
}

void Renderer::init_fbos(int w, int h)
{
    make_fbo_texture(m_fbo_hdr,    m_tex_hdr,    w, h, GL_RGBA16F);
    make_fbo_texture(m_fbo_bright, m_tex_bright, w, h, GL_RGBA16F);

    int lw = std::max(w / 2, 1);
    int lh = std::max(h / 2, 1);
    for (int i = 0; i < kBloom; ++i) {
        m_down_w[i] = lw;
        m_down_h[i] = lh;
        make_fbo_texture(m_fbo_down[i], m_tex_down[i], lw, lh, GL_RGBA16F);
        make_fbo_texture(m_fbo_up[i],   m_tex_up[i],   lw, lh, GL_RGBA16F);
        lw = std::max(lw / 2, 1);
        lh = std::max(lh / 2, 1);
    }
    check_gl_errors("Renderer::init_fbos");
}

void Renderer::destroy_fbos()
{
    auto del = [](GLuint& fbo, GLuint& tex) {
        if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
        if (tex) { glDeleteTextures    (1, &tex); tex = 0; }
    };
    del(m_fbo_hdr,    m_tex_hdr);
    del(m_fbo_bright, m_tex_bright);
    for (int i = 0; i < kBloom; ++i) {
        del(m_fbo_down[i], m_tex_down[i]);
        del(m_fbo_up[i],   m_tex_up[i]);
    }
}

// ── Helper: draw a fullscreen triangle ────────────────────────────────────
static void fullscreen_tri(GLuint vao)
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

// ── render ─────────────────────────────────────────────────────────────────
void Renderer::render(const SceneUBO& scene, const PostParams& post, float elapsed_time)
{
    // ── A. Geodesic pass → HDR FBO ────────────────────────────────────────
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneUBO), &scene);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_hdr);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_geo_prog);

    // Bind galaxy cubemap (unit 0) and color_map (unit 1)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_galaxy_tex);
    glUniform1i(m_u_galaxy, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_color_map_tex);
    glUniform1i(m_u_color_map, 1);
    glUniform1f(m_u_time, elapsed_time);

    fullscreen_tri(m_geo_vao);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── B. Bloom pyramid ──────────────────────────────────────────────────

    // B1. Bright extract → full-res bright texture
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_bright);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_bright_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_hdr);
    glUniform1i(m_bright_u_src, 0);
    glUniform2f(m_bright_u_vp,  (float)m_width, (float)m_height);
    glUniform1f(m_bright_u_thr, post.bloom_threshold);
    fullscreen_tri(m_post_vao);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // B2. Downsample: bright → down[0] → down[1] → … → down[kBloom-1]
    glUseProgram(m_down_prog);
    for (int i = 0; i < kBloom; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_down[i]);
        glViewport(0, 0, m_down_w[i], m_down_h[i]);
        glClear(GL_COLOR_BUFFER_BIT);
        GLuint src = (i == 0) ? m_tex_bright : m_tex_down[i - 1];
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, src);
        glUniform1i(m_down_u_src, 0);
        glUniform2f(m_down_u_vp,  (float)m_down_w[i], (float)m_down_h[i]);
        fullscreen_tri(m_post_vao);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // B3. Upsample: from smallest level back to level 0
    //   Level kBloom-1 (seed): box-filter down[kBloom-1] into up[kBloom-1]
    //   Level i < kBloom-1:    box-filter up[i+1] + add down[i] → up[i]
    glUseProgram(m_up_prog);
    for (int i = kBloom - 1; i >= 0; --i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_up[i]);
        glViewport(0, 0, m_down_w[i], m_down_h[i]);
        glClear(GL_COLOR_BUFFER_BIT);
        GLuint blur_src = (i == kBloom - 1) ? m_tex_down[i] : m_tex_up[i + 1];
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blur_src);
        glUniform1i(m_up_u_blur, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_tex_down[i]);
        glUniform1i(m_up_u_detail, 1);
        glUniform2f(m_up_u_vp, (float)m_down_w[i], (float)m_down_h[i]);
        fullscreen_tri(m_post_vao);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ── C. Composite HDR + up[0] → default framebuffer ───────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_post_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_hdr);
    glUniform1i(m_u_hdr_tex,   0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_tex_up[0]);
    glUniform1i(m_u_bloom_tex, 1);
    glUniform2f(m_u_vp_size,   (float)m_width, (float)m_height);
    glUniform1f(m_u_exposure,  post.exposure);
    glUniform1f(m_u_bloom_int, post.bloom_intensity);
    glUniform1i(m_u_tonemap,   post.tonemap_on);
    fullscreen_tri(m_post_vao);

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);

#ifndef NDEBUG
    check_gl_errors("Renderer::render");
#endif
}

void Renderer::render_particles(
    const std::vector<TrailVertex>&         verts,
    const std::vector<std::pair<int,int>>&  strips,
    const glm::mat4&                         mvp)
{
    if (verts.empty() || strips.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_trail_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(verts.size() * sizeof(TrailVertex)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(m_trail_prog);
    glUniformMatrix4fv(m_mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(m_trail_vao);

    for (auto [start, count] : strips)
        if (count >= 2)
            glDrawArrays(GL_LINE_STRIP, start, count);

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);

#ifndef NDEBUG
    check_gl_errors("Renderer::render_particles");
#endif
}

void Renderer::resize(int width, int height)
{
    if (width == m_width && height == m_height) return;
    m_width  = width;
    m_height = height;
    destroy_fbos();
    init_fbos(width, height);
}
