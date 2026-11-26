#include "engine/render/Gl.h"

#include <SDL3/SDL_video.h>

#include <stdexcept>
#include <string>

namespace gl {

    void load() {
#define TAKEOVER_LOAD_GL(name)                                                                                      \
    name = reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress(#name));                                           \
    if (!name) throw std::runtime_error("missing OpenGL function " #name);
        TAKEOVER_GL_FUNCTIONS(TAKEOVER_LOAD_GL)
#undef TAKEOVER_LOAD_GL
    }

} // namespace gl
