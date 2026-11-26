#include "engine/platform/Window.h"

#include "engine/render/Gl.h"

#include <stdexcept>

namespace engine {

    Window::Window(const Options& options) {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
            throw std::runtime_error(std::string("SDL video init failed: ") + SDL_GetError());

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;

        if (options.resizable)
            flags |= SDL_WINDOW_RESIZABLE;

        if (options.hidden)
            flags |= SDL_WINDOW_HIDDEN;

        window_ = SDL_CreateWindow(options.title.c_str(), options.width, options.height, flags);
        if (!window_)
            throw std::runtime_error(std::string("window creation failed: ") + SDL_GetError());

        context_ = SDL_GL_CreateContext(window_);
        if (!context_)
            throw std::runtime_error(std::string("OpenGL 3.3 context creation failed: ") + SDL_GetError());

        SDL_GL_SetSwapInterval(options.vsync ? 1 : 0);
        gl::load();
    }

    Window::~Window() {
        if (context_)
            SDL_GL_DestroyContext(context_);

        if (window_)
            SDL_DestroyWindow(window_);

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void Window::framebufferSize(int& width, int& height) const { SDL_GetWindowSizeInPixels(window_, &width, &height); }

    void Window::swap() const { SDL_GL_SwapWindow(window_); }

} // namespace engine
