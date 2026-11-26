// Texture.h - RAII owner of an OpenGL 2D texture.
#pragma once

#include "engine/render/Gl.h"
#include "vector/Rasterizer.h"

namespace engine {

    class Texture {
    public:
        Texture() = default;

        /// Uploads premultiplied BGRA pixels (vector::Image layout).
        explicit Texture(const vector::Image& image, bool smooth = true);
        ~Texture();

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        /// Replaces the pixels (same size).
        void update(const vector::Image& image);

        [[nodiscard]] GLuint id() const { return id_; }
        [[nodiscard]] int width() const { return width_; }
        [[nodiscard]] int height() const { return height_; }
        [[nodiscard]] bool valid() const { return id_ != 0; }

    private:
        GLuint id_ = 0;
        int width_ = 0;
        int height_ = 0;
    };

} // namespace engine
