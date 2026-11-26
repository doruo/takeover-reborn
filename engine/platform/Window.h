// Window.h - an SDL3 window with an OpenGL 3.3 core context.
#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace engine {

class Window {
public:
    struct Options {
        std::string title = "Take Over";
        int width = 800;
        int height = 600;
        bool resizable = true;
        bool hidden = false;  ///< Offscreen rendering (tools, tests).
        bool vsync = true;
    };

    explicit Window(const Options& options);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] SDL_Window* handle() const { return window_; }
    /// Drawable size in pixels (differs from the window size on HiDPI).
    void framebufferSize(int& width, int& height) const;
    void swap() const;

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
};

} // namespace engine
