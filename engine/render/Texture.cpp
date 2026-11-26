#include "engine/render/Texture.h"

#include <utility>

namespace engine {

Texture::Texture(const vector::Image& image, bool smooth) : width_(image.width), height_(image.height) {
    gl::glGenTextures(1, &id_);
    gl::glBindTexture(GL_TEXTURE_2D, id_);
    gl::glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    gl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.pixels.data());
    const GLint filter = smooth ? GL_LINEAR : GL_NEAREST;
    gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture::update(const vector::Image& image) {
    gl::glBindTexture(GL_TEXTURE_2D, id_);
    gl::glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    gl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, image.pixels.data());
}

Texture::~Texture() {
    if (id_ != 0) gl::glDeleteTextures(1, &id_);
}

Texture::Texture(Texture&& other) noexcept
    : id_(std::exchange(other.id_, 0)), width_(other.width_), height_(other.height_) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (id_ != 0) gl::glDeleteTextures(1, &id_);
        id_ = std::exchange(other.id_, 0);
        width_ = other.width_;
        height_ = other.height_;
    }
    return *this;
}

} // namespace engine
