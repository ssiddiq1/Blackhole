#pragma once
// Window.hpp — GLFW window + OpenGL 4.1 core context.
//
// Owns the GLFW window handle and all input state.  Keyboard state is
// queried per-frame (polling); mouse delta is accumulated between calls
// to consume_mouse_delta().

#define GLFW_INCLUDE_NONE       // we manage the GL header ourselves
#include <GLFW/glfw3.h>
#include <functional>
#include <string>

class Window {
public:
    // Create window + GL 4.1 core context.  Throws on failure.
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    bool should_close()  const;
    void poll_events();
    void swap_buffers();

    int width()  const { return m_width; }
    int height() const { return m_height; }

    // Returns true while the physical key is held down this frame.
    bool key(int glfw_key) const;

    // Returns true while the mouse button (GLFW_MOUSE_BUTTON_*) is held.
    bool mouse_button(int glfw_button) const;

    // Returns accumulated mouse movement since last call, then resets to (0,0).
    // Only non-zero when mouse is captured.
    void consume_mouse_delta(double& dx, double& dy);

    // Lock/unlock the cursor to the window center for free-fly look.
    void set_mouse_captured(bool captured);
    bool mouse_captured() const { return m_mouse_captured; }

    // Optional resize callback (called after the internal state is updated).
    std::function<void(int, int)> on_resize;

private:
    GLFWwindow* m_handle         = nullptr;
    int         m_width          = 0;
    int         m_height         = 0;
    bool        m_mouse_captured = false;
    double      m_mouse_dx       = 0.0;
    double      m_mouse_dy       = 0.0;
    double      m_last_x         = 0.0;
    double      m_last_y         = 0.0;
    bool        m_first_mouse    = true;

    // GLFW callbacks (static, forward to the Window instance stored as userdata)
    static void cb_framebuffer_size(GLFWwindow*, int w, int h);
    static void cb_cursor_pos(GLFWwindow*, double x, double y);
    static void cb_key(GLFWwindow*, int key, int scancode, int action, int mods);
};
