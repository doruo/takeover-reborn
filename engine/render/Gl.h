// Gl.h - the OpenGL 3.3 core entry points the engine uses.
//
// OpenGL functions must be looked up at runtime (the system libGL only
// exports OpenGL 1.1). Instead of a generated loader with thousands of
// functions, we list the ~60 we call in one X-macro and fetch them through
// SDL_GL_GetProcAddress. Adding a function = adding one line to the list.
//
// Call them as gl::glClear(...): the gl:: prefix makes it obvious that a
// call goes through a loaded pointer.
#pragma once

#define GL_GLEXT_PROTOTYPES 1 // declares (never defines) prototypes, for decltype
#include <SDL3/SDL_opengl.h>

#define TAKEOVER_GL_FUNCTIONS(X)                                                                                    \
    X(glGetString)                                                                                                  \
    X(glGetIntegerv)                                                                                                \
    X(glGetError)                                                                                                   \
    X(glViewport)                                                                                                   \
    X(glScissor)                                                                                                    \
    X(glClearColor)                                                                                                 \
    X(glClearStencil)                                                                                               \
    X(glClear)                                                                                                      \
    X(glEnable)                                                                                                     \
    X(glDisable)                                                                                                    \
    X(glBlendFunc)                                                                                                  \
    X(glBlendFuncSeparate)                                                                                          \
    X(glBlendEquation)                                                                                              \
    X(glColorMask)                                                                                                  \
    X(glStencilFunc)                                                                                                \
    X(glStencilOp)                                                                                                  \
    X(glStencilMask)                                                                                                \
    X(glPixelStorei)                                                                                                \
    X(glReadPixels)                                                                                                 \
    X(glFinish)                                                                                                     \
    X(glGenTextures)                                                                                                \
    X(glDeleteTextures)                                                                                             \
    X(glBindTexture)                                                                                                \
    X(glActiveTexture)                                                                                              \
    X(glTexImage2D)                                                                                                 \
    X(glTexSubImage2D)                                                                                              \
    X(glCopyTexSubImage2D)                                                                                          \
    X(glTexParameteri)                                                                                              \
    X(glCreateShader)                                                                                               \
    X(glShaderSource)                                                                                               \
    X(glCompileShader)                                                                                              \
    X(glGetShaderiv)                                                                                                \
    X(glGetShaderInfoLog)                                                                                           \
    X(glDeleteShader)                                                                                               \
    X(glCreateProgram)                                                                                              \
    X(glAttachShader)                                                                                               \
    X(glBindAttribLocation)                                                                                         \
    X(glLinkProgram)                                                                                                \
    X(glGetProgramiv)                                                                                               \
    X(glGetProgramInfoLog)                                                                                          \
    X(glDeleteProgram)                                                                                              \
    X(glUseProgram)                                                                                                 \
    X(glGetUniformLocation)                                                                                         \
    X(glUniform1i)                                                                                                  \
    X(glUniform1f)                                                                                                  \
    X(glUniform2f)                                                                                                  \
    X(glUniform4f)                                                                                                  \
    X(glUniform4fv)                                                                                                 \
    X(glGenVertexArrays)                                                                                            \
    X(glBindVertexArray)                                                                                            \
    X(glDeleteVertexArrays)                                                                                         \
    X(glGenBuffers)                                                                                                 \
    X(glBindBuffer)                                                                                                 \
    X(glBufferData)                                                                                                 \
    X(glDeleteBuffers)                                                                                              \
    X(glVertexAttribPointer)                                                                                        \
    X(glEnableVertexAttribArray)                                                                                    \
    X(glDrawArrays)                                                                                                 \
    X(glGenFramebuffers)                                                                                            \
    X(glDeleteFramebuffers)                                                                                         \
    X(glBindFramebuffer)                                                                                            \
    X(glFramebufferTexture2D)                                                                                       \
    X(glFramebufferRenderbuffer)                                                                                    \
    X(glCheckFramebufferStatus)                                                                                     \
    X(glGenRenderbuffers)                                                                                           \
    X(glDeleteRenderbuffers)                                                                                        \
    X(glBindRenderbuffer)                                                                                           \
    X(glRenderbufferStorage)                                                                                        \
    X(glBlitFramebuffer)

namespace gl {

#define TAKEOVER_DECLARE_GL(name) inline decltype(&::name) name = nullptr;
    TAKEOVER_GL_FUNCTIONS(TAKEOVER_DECLARE_GL)
#undef TAKEOVER_DECLARE_GL

    /// Resolves every function of the list. Requires a current GL context.
    /// Throws std::runtime_error naming the first missing function.
    void load();

} // namespace gl
