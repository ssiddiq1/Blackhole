// Window.cpp
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>   // must come before any GLFW include that might pull in GL

#include "app/Window.hpp"
#include <stdexcept>
#include <string>
#include <iostream>

// ── GLFW callbacks ────────────────────────────────────────────────────────
void Window::cb_framebuffer_size(GLFWwindow* handle, int w, int h)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    self->m_width  = w;
    self->m_height = h;
    if (self->on_resize) self->on_resize(w, h);
}

void Window::cb_cursor_pos(GLFWwindow* handle, double x, double y)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (!self->m_mouse_captured) {
        // Reset so we don't get a jump when we re-capture later
        self->m_first_mouse = true;
        return;
    }
    if (self->m_first_mouse) {
        self->m_last_x     = x;
        self->m_last_y     = y;
        self->m_first_mouse = false;
        return;
    }
    self->m_mouse_dx += x - self->m_last_x;
    self->m_mouse_dy += y - self->m_last_y;
    self->m_last_x    = x;
    self->m_last_y    = y;
}

void Window::cb_key(GLFWwindow* handle, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    // Escape: toggle mouse capture
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        self->set_mouse_captured(!self->m_mouse_captured);
    }
}

// ── Window ────────────────────────────────────────────────────────────────
Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    if (!glfwInit())
        throw std::runtime_error("Window: glfwInit() failed");

    // Request OpenGL 4.1 core profile.
    // macOS supports up to 4.1; this is also fine on Linux/Windows.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // required on macOS

    // Enable the GL debug context on debug builds.
    // (On macOS GL_ARB_debug_output is not exposed; we use glGetError polling instead.)
#if !defined(NDEBUG) && !defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        glfwTerminate();
        throw std::runtime_error("Window: glfwCreateWindow() failed");
    }

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, cb_framebuffer_size);
    glfwSetCursorPosCallback(m_handle,       cb_cursor_pos);
    glfwSetKeyCallback(m_handle,             cb_key);

    glfwMakeContextCurrent(m_handle);
    glfwSwapInterval(0);    // disable vsync for uncapped fps measurement

    // Print the actual GL version we got
    std::cout << "OpenGL: " << glGetString(GL_VERSION)
              << "  GLSL: "  << glGetString(GL_SHADING_LANGUAGE_VERSION)
              << "  GPU: "   << glGetString(GL_RENDERER) << std::endl;

    // Capture the actual framebuffer size (may differ from window size on HiDPI)
    glfwGetFramebufferSize(m_handle, &m_width, &m_height);
}

Window::~Window()
{
    if (m_handle) glfwDestroyWindow(m_handle);
    glfwTerminate();
}

bool Window::should_close() const
{
    return glfwWindowShouldClose(m_handle);
}

void Window::poll_events()
{
    glfwPollEvents();
}

void Window::swap_buffers()
{
    glfwSwapBuffers(m_handle);
}

bool Window::key(int glfw_key) const
{
    return glfwGetKey(m_handle, glfw_key) == GLFW_PRESS;
}

bool Window::mouse_button(int glfw_button) const
{
    return glfwGetMouseButton(m_handle, glfw_button) == GLFW_PRESS;
}

void Window::consume_mouse_delta(double& dx, double& dy)
{
    dx = m_mouse_dx;
    dy = m_mouse_dy;
    m_mouse_dx = 0.0;
    m_mouse_dy = 0.0;
}

void Window::set_mouse_captured(bool captured)
{
    m_mouse_captured = captured;
    glfwSetInputMode(m_handle, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (captured) m_first_mouse = true;   // suppress jump on recapture
}
