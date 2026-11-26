#include "engine/render/Renderer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <variant>

namespace engine {

    namespace {

        // /----- SHADERS -----/

        //
        // Textures hold premultiplied color. Flash applies a color transform to the
        // straight color, so shaders unpremultiply, transform, clamp and premultiply.

        // Quads: the unit square scaled to the texture size, mapped by an affine
        // matrix to framebuffer pixels, then to clip space. u_uvOffset/u_uvScale
        // select the part of the texture to show (and flip render targets).
        constexpr const char* kQuadVertex = R"(#version 330 core
in vec2 a_unit;
uniform vec4 u_matrix;    // a, b, c, d
uniform vec2 u_translate; // tx, ty (framebuffer pixels)
uniform vec2 u_size;      // size of the drawn area in texels
uniform vec2 u_viewport;  // framebuffer size
uniform vec2 u_uvOffset;
uniform vec2 u_uvScale;
out vec2 v_uv;
void main() {
    vec2 local = a_unit * u_size;
    vec2 p = vec2(u_matrix.x * local.x + u_matrix.z * local.y + u_translate.x,
                  u_matrix.y * local.x + u_matrix.w * local.y + u_translate.y);
    gl_Position = vec4(p.x / u_viewport.x * 2.0 - 1.0, 1.0 - p.y / u_viewport.y * 2.0, 0.0, 1.0);
    v_uv = u_uvOffset + a_unit * u_uvScale;
}
)";

        constexpr const char* kColorTransformFunction = R"(
uniform vec4 u_multiply;
uniform vec4 u_add;     // already divided by 255
uniform bool u_useColorTransform;
vec4 colorTransform(vec4 c) {
    if (!u_useColorTransform) return c;
    vec4 straight = c.a > 0.0 ? vec4(c.rgb / c.a, c.a) : vec4(0.0);
    straight = clamp(straight * u_multiply + u_add, 0.0, 1.0);
    return vec4(straight.rgb * straight.a, straight.a);
}
)";

        constexpr const char* kQuadFragment = R"(
in vec2 v_uv;
uniform sampler2D u_texture;
uniform bool u_maskMode;
out vec4 fragColor;
void main() {
    vec4 c = texture(u_texture, v_uv);
    if (u_maskMode) {
        // Flash masks are binary: a pixel is inside the mask shape or not.
        if (c.a < 0.5) discard;
        fragColor = vec4(1.0);
        return;
    }
    fragColor = colorTransform(c);
}
)";

        // Shader blend modes: combines the layer (source) with a copy of what is
        // under it (destination), with the separable blend formula
        //   out = S * (1 - Da) + D * (1 - Sa) + Sa * Da * B(Cs, Cd)
        constexpr const char* kBlendFragment = R"(
in vec2 v_uv;
uniform sampler2D u_texture;
uniform sampler2D u_destination;
uniform vec2 u_destinationOrigin; // window pixels of the copy's corner
uniform vec2 u_destinationSize;   // texture size of the copy
uniform int u_mode;               // swf::BlendMode value
out vec4 fragColor;

vec3 blendColor(vec3 s, vec3 d) {
    if (u_mode == 3) return s * d;                                  // multiply
    if (u_mode == 4) return s + d - s * d;                          // screen
    if (u_mode == 5) return max(s, d);                              // lighten
    if (u_mode == 6) return min(s, d);                              // darken
    if (u_mode == 7) return abs(s - d);                             // difference
    if (u_mode == 10) return 1.0 - d;                               // invert
    if (u_mode == 13)                                               // overlay
        return mix(2.0 * s * d, 1.0 - 2.0 * (1.0 - s) * (1.0 - d), step(0.5, d));
    if (u_mode == 14)                                               // hard light
        return mix(2.0 * s * d, 1.0 - 2.0 * (1.0 - s) * (1.0 - d), step(0.5, s));
    return s;
}

void main() {
    vec4 S = colorTransform(texture(u_texture, v_uv));
    vec4 D = texture(u_destination, (gl_FragCoord.xy - u_destinationOrigin) / u_destinationSize);
    if (u_mode == 8) { fragColor = min(S + D, 1.0); return; }               // add
    if (u_mode == 9) {                                                     // subtract
        fragColor = vec4(max(D.rgb - S.rgb, 0.0), S.a + D.a - S.a * D.a);
        return;
    }
    vec3 cs = S.a > 0.0 ? S.rgb / S.a : vec3(0.0);
    vec3 cd = D.a > 0.0 ? D.rgb / D.a : vec3(0.0);
    vec3 rgb = S.rgb * (1.0 - D.a) + D.rgb * (1.0 - S.a) + S.a * D.a * blendColor(cs, cd);
    fragColor = vec4(rgb, S.a + D.a - S.a * D.a);
}
)";

        // Fullscreen passes over a layer: pixel coordinates come from gl_FragCoord,
        // so source and destination line up whatever their texture sizes.
        constexpr const char* kPassVertex = R"(#version 330 core
in vec2 a_unit;
void main() { gl_Position = vec4(a_unit * 2.0 - 1.0, 0.0, 1.0); }
)";

        constexpr const char* kPassCommon = R"(
uniform sampler2D u_source;
uniform vec2 u_sourceSize; // texels
uniform vec2 u_area;       // valid pixels of the layer
out vec4 fragColor;
// Reads the layer; anything outside its area is transparent.
vec4 sampleSource(vec2 pixel) {
    if (pixel.x < 0.0 || pixel.y < 0.0 || pixel.x >= u_area.x || pixel.y >= u_area.y) return vec4(0.0);
    return texture(u_source, pixel / u_sourceSize);
}
)";

        // One axis of Flash's box blur: the average over a window of `2 * radius`
        // pixels, with fractional weights at the window ends.
        constexpr const char* kBlurFragment = R"(
uniform vec2 u_direction;
uniform float u_radius;
void main() {
    vec2 pixel = gl_FragCoord.xy;
    int reach = int(ceil(u_radius));
    vec4 sum = vec4(0.0);
    float weights = 0.0;
    for (int i = -reach; i <= reach; ++i) {
        float w = clamp(u_radius + 0.5 - abs(float(i)), 0.0, 1.0);
        sum += sampleSource(pixel + u_direction * float(i)) * w;
        weights += w;
    }
    fragColor = sum / max(weights, 1e-5);
}
)";

        constexpr const char* kColorMatrixFragment = R"(
uniform vec4 u_rows[4];  // the 4x4 part of the matrix
uniform vec4 u_offsets;  // 5th column, divided by 255
void main() {
    vec4 c = sampleSource(gl_FragCoord.xy);
    vec4 s = c.a > 0.0 ? vec4(c.rgb / c.a, c.a) : vec4(0.0);
    vec4 r = clamp(vec4(dot(u_rows[0], s), dot(u_rows[1], s), dot(u_rows[2], s), dot(u_rows[3], s)) + u_offsets,
                   0.0, 1.0);
    fragColor = vec4(r.rgb * r.a, r.a);
}
)";

        // Glow and drop shadow. Step 0 extracts the (offset, maybe inverted) alpha
        // of the object. It is then blurred. Step 1 tints it and combines it with
        // the object.
        constexpr const char* kGlowFragment = R"(
uniform int u_step;
uniform vec2 u_offset;          // shadow offset in pixels (GL orientation)
uniform bool u_inner;
uniform bool u_knockout;
uniform bool u_hideObject;
uniform vec4 u_color;           // straight RGBA
uniform float u_strength;
uniform sampler2D u_blurred;
uniform vec2 u_blurredSize;
void main() {
    vec2 pixel = gl_FragCoord.xy;
    if (u_step == 0) {
        float a = sampleSource(pixel - u_offset).a;
        fragColor = vec4(0.0, 0.0, 0.0, u_inner ? 1.0 - a : a);
        return;
    }
    vec4 src = sampleSource(pixel);
    float m = texture(u_blurred, pixel / u_blurredSize).a;
    float g = clamp(m * u_strength, 0.0, 1.0) * u_color.a;
    if (u_inner) {
        vec4 glow = vec4(u_color.rgb, 1.0) * g * src.a;
        fragColor = u_knockout ? glow : glow + src * (1.0 - g);
    } else {
        vec4 glow = vec4(u_color.rgb * g, g);
        if (u_knockout) fragColor = glow * (1.0 - src.a);
        else if (u_hideObject) fragColor = glow;
        else fragColor = src + glow * (1.0 - src.a);
    }
}
)";

        GLuint compile(GLenum type, const std::string& source) {
            const GLuint shader = gl::glCreateShader(type);
            const char* text = source.c_str();
            gl::glShaderSource(shader, 1, &text, nullptr);
            gl::glCompileShader(shader);
            GLint ok = 0;
            gl::glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

            if (!ok) {
                char log[2048];
                gl::glGetShaderInfoLog(shader, sizeof log, nullptr, log);
                throw std::runtime_error(std::string("shader compilation failed: ") + log);
            }

            return shader;
        }

        GLuint link(const std::string& vertex, const std::string& fragment) {
            const GLuint vs = compile(GL_VERTEX_SHADER, vertex);
            const GLuint fs = compile(GL_FRAGMENT_SHADER, fragment);
            const GLuint program = gl::glCreateProgram();
            gl::glAttachShader(program, vs);
            gl::glAttachShader(program, fs);
            gl::glBindAttribLocation(program, 0, "a_unit");
            gl::glLinkProgram(program);
            gl::glDeleteShader(vs);
            gl::glDeleteShader(fs);
            GLint ok = 0;
            gl::glGetProgramiv(program, GL_LINK_STATUS, &ok);

            if (!ok) {
                char log[2048];
                gl::glGetProgramInfoLog(program, sizeof log, nullptr, log);
                throw std::runtime_error(std::string("shader link failed: ") + log);
            }

            return program;
        }

        constexpr const char* kVersion = "#version 330 core\n";

        /// Pooled render targets are rounded up to limit reallocations.
        int roundUp(int value, int step) { return (value + step - 1) / step * step; }

        void setColorTransform(GLuint program, const ColorTransform& cx) {
            const bool use = !cx.isIdentity();
            gl::glUniform1i(gl::glGetUniformLocation(program, "u_useColorTransform"), use);

            if (!use)
                return;

            gl::glUniform4f(gl::glGetUniformLocation(program, "u_multiply"), cx.redMultiplier, cx.greenMultiplier,
                            cx.blueMultiplier, cx.alphaMultiplier);
            gl::glUniform4f(gl::glGetUniformLocation(program, "u_add"), cx.redOffset / 255.0f, cx.greenOffset / 255.0f,
                            cx.blueOffset / 255.0f, cx.alphaOffset / 255.0f);
        }

    } // namespace

    GLint Renderer::Program::uniform(const char* name) const { return gl::glGetUniformLocation(id, name); }

    // /----- RENDER TARGET -----/

    RenderTarget::RenderTarget(int width, int height) : width_(width), height_(height) {
        gl::glGenTextures(1, &color_);
        gl::glBindTexture(GL_TEXTURE_2D, color_);
        gl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        gl::glGenRenderbuffers(1, &stencil_);
        gl::glBindRenderbuffer(GL_RENDERBUFFER, stencil_);
        gl::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        gl::glGenFramebuffers(1, &framebuffer_);
        gl::glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        gl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);
        gl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil_);

        if (gl::glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("incomplete framebuffer");
    }

    RenderTarget::~RenderTarget() {
        gl::glDeleteFramebuffers(1, &framebuffer_);
        gl::glDeleteRenderbuffers(1, &stencil_);
        gl::glDeleteTextures(1, &color_);
    }

    void RenderTarget::bind() const { gl::glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_); }

    vector::Image RenderTarget::readPremultipliedBgra() const {
        vector::Image image(width_, height_);
        std::vector<std::uint8_t> pixels(image.pixels.size());
        bind();
        gl::glPixelStorei(GL_PACK_ALIGNMENT, 4);
        gl::glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
        const std::size_t row = static_cast<std::size_t>(width_) * 4;

        for (int y = 0; y < height_; ++y)
            std::copy_n(&pixels[(height_ - 1 - y) * row], row, &image.pixels[y * row]);

        return image;
    }

    std::vector<std::uint8_t> RenderTarget::readRgba() const {
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width_) * height_ * 4);
        bind();
        gl::glPixelStorei(GL_PACK_ALIGNMENT, 4);
        gl::glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // GL rows go bottom-up. Flip, and convert premultiplied to straight alpha.
        std::vector<std::uint8_t> out(pixels.size());
        const std::size_t row = static_cast<std::size_t>(width_) * 4;

        for (int y = 0; y < height_; ++y) {
            const std::uint8_t* src = &pixels[(height_ - 1 - y) * row];
            std::uint8_t* dst = &out[y * row];

            for (int x = 0; x < width_; ++x) {
                const int a = src[x * 4 + 3];

                for (int c = 0; c < 3; ++c)
                    dst[x * 4 + c] = a == 0 ? 0 : static_cast<std::uint8_t>(std::min(255, (src[x * 4 + c] * 255 + a / 2) / a));

                dst[x * 4 + 3] = static_cast<std::uint8_t>(a);
            }
        }

        return out;
    }

    // /----- RENDERER -----/

    Renderer::Renderer() {
        const std::string colorTransform = kColorTransformFunction;
        quad_.id = link(kQuadVertex, kVersion + colorTransform + kQuadFragment);
        blend_.id = link(kQuadVertex, kVersion + colorTransform + kBlendFragment);
        blur_.id = link(kPassVertex, std::string(kVersion) + kPassCommon + kBlurFragment);
        colorMatrix_.id = link(kPassVertex, std::string(kVersion) + kPassCommon + kColorMatrixFragment);
        glow_.id = link(kPassVertex, std::string(kVersion) + kPassCommon + kGlowFragment);

        const float quad[] = {0, 0, 1, 0, 0, 1, 1, 1};
        gl::glGenVertexArrays(1, &vao_);
        gl::glBindVertexArray(vao_);
        gl::glGenBuffers(1, &vbo_);
        gl::glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        gl::glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
        gl::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        gl::glEnableVertexAttribArray(0);
    }

    Renderer::~Renderer() {
        layers_.clear();
        pool_.clear();

        if (destinationCopy_ != 0)
            gl::glDeleteTextures(1, &destinationCopy_);

        gl::glDeleteBuffers(1, &vbo_);
        gl::glDeleteVertexArrays(1, &vao_);

        for (const Program* p : {&quad_, &blend_, &blur_, &colorMatrix_, &glow_})
            gl::glDeleteProgram(p->id);
    }

    void Renderer::bindTarget(GLuint framebuffer, int width, int height) {
        gl::glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl::glViewport(0, 0, width, height);
        currentFramebuffer_ = framebuffer;
        viewportWidth_ = width;
        viewportHeight_ = height;
    }

    void Renderer::beginFrame(int framebufferWidth, int framebufferHeight, int stageWidth, int stageHeight, float clearR,
                              float clearG, float clearB) {
        GLint bound = 0;
        gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
        layers_.clear();
        bindTarget(static_cast<GLuint>(bound), framebufferWidth, framebufferHeight);

        const double scale = std::min(static_cast<double>(framebufferWidth) / stageWidth,
                                      static_cast<double>(framebufferHeight) / stageHeight);
        stageToFramebuffer_ = {scale, 0, 0, scale, std::round((framebufferWidth - stageWidth * scale) / 2),
                               std::round((framebufferHeight - stageHeight * scale) / 2)};

        gl::glDisable(GL_SCISSOR_TEST);
        gl::glClearColor(0, 0, 0, 1);
        gl::glClearStencil(0);
        gl::glStencilMask(0xFF);
        gl::glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Paint the stage background inside the letterbox.
        letterboxed_ = true;
        gl::glEnable(GL_SCISSOR_TEST);
        gl::glScissor(static_cast<GLint>(stageToFramebuffer_.tx),
                      static_cast<GLint>(framebufferHeight - stageToFramebuffer_.ty - stageHeight * scale),
                      static_cast<GLsizei>(stageWidth * scale + 0.5), static_cast<GLsizei>(stageHeight * scale + 0.5));
        gl::glClearColor(clearR, clearG, clearB, 1);
        gl::glClear(GL_COLOR_BUFFER_BIT);

        // The scissor stays on: nothing is drawn in the letterbox bands. Layers
        // are offscreen, so it's switched off there (see pushLayer).

        gl::glEnable(GL_BLEND);
        gl::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied "over"
        gl::glBindVertexArray(vao_);
        gl::glActiveTexture(GL_TEXTURE0);
        stencilMode_ = StencilMode::Normal;
        maskDepth_ = 0;
        applyStencilState();
    }

    void Renderer::beginOffscreen(int width, int height) {
        GLint bound = 0;
        gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
        layers_.clear();
        bindTarget(static_cast<GLuint>(bound), width, height);
        stageToFramebuffer_ = Matrix{};
        letterboxed_ = false;
        gl::glDisable(GL_SCISSOR_TEST);
        gl::glClearColor(0, 0, 0, 0);
        gl::glClearStencil(0);
        gl::glStencilMask(0xFF);
        gl::glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        gl::glEnable(GL_BLEND);
        gl::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        gl::glBindVertexArray(vao_);
        gl::glActiveTexture(GL_TEXTURE0);
        stencilMode_ = StencilMode::Normal;
        maskDepth_ = 0;
        applyStencilState();
    }

    void Renderer::drawTexture(const Texture& texture, const Matrix& textureToStage, const ColorTransform& cx) {
        const Matrix m = textureToStage.then(stageToFramebuffer_);
        gl::glUseProgram(quad_.id);
        gl::glBindTexture(GL_TEXTURE_2D, texture.id());
        gl::glUniform4f(quad_.uniform("u_matrix"), static_cast<float>(m.a), static_cast<float>(m.b),
                        static_cast<float>(m.c), static_cast<float>(m.d));
        gl::glUniform2f(quad_.uniform("u_translate"), static_cast<float>(m.tx), static_cast<float>(m.ty));
        gl::glUniform2f(quad_.uniform("u_size"), static_cast<float>(texture.width()), static_cast<float>(texture.height()));
        gl::glUniform2f(quad_.uniform("u_viewport"), static_cast<float>(viewportWidth_), static_cast<float>(viewportHeight_));
        gl::glUniform2f(quad_.uniform("u_uvOffset"), 0, 0);
        gl::glUniform2f(quad_.uniform("u_uvScale"), 1, 1);
        gl::glUniform1i(quad_.uniform("u_maskMode"), stencilMode_ != StencilMode::Normal);
        setColorTransform(quad_.id, cx);
        gl::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // /----- MASKS -----/

    // The stencil value of a pixel counts how many nested masks cover it. Content
    // is drawn where stencil == maskDepth_, i.e. inside every active mask.

    void Renderer::beginMask() {
        stencilMode_ = StencilMode::WritingMask;
        applyStencilState();
    }

    void Renderer::endMask() {
        ++maskDepth_;
        stencilMode_ = StencilMode::Normal;
        applyStencilState();
    }

    void Renderer::beginUnmask() {
        stencilMode_ = StencilMode::ErasingMask;
        applyStencilState();
    }

    void Renderer::endUnmask() {
        --maskDepth_;
        stencilMode_ = StencilMode::Normal;
        applyStencilState();
    }

    void Renderer::applyStencilState() {
        if (maskDepth_ == 0 && stencilMode_ == StencilMode::Normal) {
            gl::glDisable(GL_STENCIL_TEST);
            gl::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

            return;
        }

        gl::glEnable(GL_STENCIL_TEST);
        gl::glStencilFunc(GL_EQUAL, maskDepth_, 0xFF);

        switch (stencilMode_) {
        case StencilMode::Normal:
            gl::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            gl::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        case StencilMode::WritingMask:
            gl::glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            gl::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        case StencilMode::ErasingMask:
            gl::glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
            gl::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            break;
        }
    }

    // /----- LAYERS -----/

    std::unique_ptr<RenderTarget> Renderer::acquireTarget(int width, int height) {
        // Best fit among the free targets, so big ones stay available.
        auto best = pool_.end();

        for (auto it = pool_.begin(); it != pool_.end(); ++it)
            if ((*it)->width() >= width && (*it)->height() >= height &&
                (best == pool_.end() || (*it)->width() * (*it)->height() < (*best)->width() * (*best)->height()))
                best = it;
        std::unique_ptr<RenderTarget> target;

        if (best != pool_.end()) {
            target = std::move(*best);
            pool_.erase(best);
        } else {
            target = std::make_unique<RenderTarget>(roundUp(width, 128), roundUp(height, 128));
        }

        return target;
    }

    void Renderer::releaseTarget(std::unique_ptr<RenderTarget> target) {
        constexpr std::size_t kMaxPooled = 12;
        pool_.push_back(std::move(target));

        if (pool_.size() > kMaxPooled)
            pool_.erase(pool_.begin()); // drop the oldest
    }

    void Renderer::pushLayer(const PixelRect& area) {
        Layer layer;
        layer.area = area;
        layer.savedStageToTarget = stageToFramebuffer_;
        layer.savedFramebuffer = currentFramebuffer_;
        layer.savedViewportWidth = viewportWidth_;
        layer.savedViewportHeight = viewportHeight_;
        layer.savedMaskDepth = maskDepth_;
        layer.target = acquireTarget(area.width, area.height);

        bindTarget(layer.target->framebuffer(), area.width, area.height);
        gl::glDisable(GL_SCISSOR_TEST);
        gl::glClearColor(0, 0, 0, 0);
        gl::glClearStencil(0);
        gl::glStencilMask(0xFF);
        gl::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gl::glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        stageToFramebuffer_ = stageToFramebuffer_.then(Matrix::translation(-area.x, -area.y));
        maskDepth_ = 0;
        applyStencilState();
        layers_.push_back(std::move(layer));
    }

    void Renderer::runPass(const Program& program, GLuint source, RenderTarget& destination, const PixelRect& area) {
        gl::glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer());
        gl::glViewport(0, 0, area.width, area.height);
        gl::glUseProgram(program.id);
        gl::glActiveTexture(GL_TEXTURE0);
        gl::glBindTexture(GL_TEXTURE_2D, source);
        gl::glUniform1i(program.uniform("u_source"), 0);
        gl::glUniform2f(program.uniform("u_area"), static_cast<float>(area.width), static_cast<float>(area.height));
        gl::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void Renderer::blurInPlace(Layer& layer, float blurX, float blurY, int passes) {
        const double scale = stageScale();
        const float radiusX = static_cast<float>(blurX * scale / 2), radiusY = static_cast<float>(blurY * scale / 2);
        if (radiusX < 0.5f && radiusY < 0.5f)
            return;

        std::unique_ptr<RenderTarget> scratch = acquireTarget(layer.target->width(), layer.target->height());
        gl::glUseProgram(blur_.id);

        for (int pass = 0; pass < std::max(1, passes); ++pass) {
            for (int axis = 0; axis < 2; ++axis) {
                const float radius = axis == 0 ? radiusX : radiusY;
                if (radius < 0.5f)
                    continue;

                gl::glUseProgram(blur_.id);
                gl::glUniform2f(blur_.uniform("u_sourceSize"), static_cast<float>(layer.target->width()),
                                static_cast<float>(layer.target->height()));
                gl::glUniform2f(blur_.uniform("u_direction"), axis == 0 ? 1.0f : 0.0f, axis == 0 ? 0.0f : 1.0f);
                gl::glUniform1f(blur_.uniform("u_radius"), radius);
                runPass(blur_, layer.target->texture(), *scratch, layer.area);
                std::swap(layer.target, scratch);
            }
        }

        releaseTarget(std::move(scratch));
    }

    void Renderer::glowOrShadow(Layer& layer, const swf::Rgba& color, float blurX, float blurY, float strength,
                                int passes, float offsetX, float offsetY, bool inner, bool knockout, bool hideObject) {
        const auto setCommon = [&](GLuint sourceWidth, GLuint sourceHeight) {
            gl::glUniform2f(glow_.uniform("u_sourceSize"), static_cast<float>(sourceWidth), static_cast<float>(sourceHeight));
            gl::glUniform1i(glow_.uniform("u_inner"), inner);
            gl::glUniform1i(glow_.uniform("u_knockout"), knockout);
            gl::glUniform1i(glow_.uniform("u_hideObject"), hideObject);
        };

        // 1. The object's alpha, shifted and inverted as needed. Offsets are in
        //    stage pixels, y down. GL rows go up.
        Layer mask;
        mask.area = layer.area;
        mask.target = acquireTarget(layer.target->width(), layer.target->height());
        gl::glUseProgram(glow_.id);
        setCommon(static_cast<GLuint>(layer.target->width()), static_cast<GLuint>(layer.target->height()));
        gl::glUniform1i(glow_.uniform("u_step"), 0);
        gl::glUniform2f(glow_.uniform("u_offset"), static_cast<float>(offsetX * stageScale()),
                        static_cast<float>(-offsetY * stageScale()));
        runPass(glow_, layer.target->texture(), *mask.target, layer.area);

        // 2. Blurred.
        blurInPlace(mask, blurX, blurY, passes);

        // 3. Tinted and combined with the object.
        std::unique_ptr<RenderTarget> result = acquireTarget(layer.target->width(), layer.target->height());
        gl::glUseProgram(glow_.id);
        setCommon(static_cast<GLuint>(layer.target->width()), static_cast<GLuint>(layer.target->height()));
        gl::glUniform1i(glow_.uniform("u_step"), 1);
        gl::glUniform4f(glow_.uniform("u_color"), color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        gl::glUniform1f(glow_.uniform("u_strength"), strength);
        gl::glActiveTexture(GL_TEXTURE1);
        gl::glBindTexture(GL_TEXTURE_2D, mask.target->texture());
        gl::glUniform1i(glow_.uniform("u_blurred"), 1);
        gl::glUniform2f(glow_.uniform("u_blurredSize"), static_cast<float>(mask.target->width()),
                        static_cast<float>(mask.target->height()));
        gl::glActiveTexture(GL_TEXTURE0);
        runPass(glow_, layer.target->texture(), *result, layer.area);

        std::swap(layer.target, result);
        releaseTarget(std::move(result));
        releaseTarget(std::move(mask.target));
    }

    int Renderer::filterPadding(const swf::Filter& filter) const {
        const double scale = stageScale();

        const auto spread = [&](float blurX, float blurY, int passes) {
            return std::max(blurX, blurY) / 2 * std::max(1, passes) * scale;
        };

        double pad = 0;

        if (const auto* f = std::get_if<swf::BlurFilter>(&filter))
            pad = spread(f->blurX, f->blurY, f->passes);

        if (const auto* f = std::get_if<swf::GlowFilter>(&filter))
            pad = f->inner ? 0 : spread(f->blurX, f->blurY, f->passes);

        if (const auto* f = std::get_if<swf::DropShadowFilter>(&filter))
            pad = f->inner ? 0 : spread(f->blurX, f->blurY, f->passes) + f->distance * scale;

        return static_cast<int>(std::ceil(pad)) + 1;
    }

    void Renderer::applyFilter(const swf::Filter& filter) {
        if (layers_.empty())
            return;

        Layer& layer = layers_.back();
        gl::glDisable(GL_BLEND);
        gl::glDisable(GL_STENCIL_TEST);

        if (const auto* blur = std::get_if<swf::BlurFilter>(&filter)) {
            blurInPlace(layer, blur->blurX, blur->blurY, blur->passes);
        } else if (const auto* matrix = std::get_if<swf::ColorMatrixFilter>(&filter)) {
            std::unique_ptr<RenderTarget> result = acquireTarget(layer.target->width(), layer.target->height());
            gl::glUseProgram(colorMatrix_.id);
            const auto& m = matrix->matrix;
            const float rows[16] = {m[0], m[1], m[2], m[3], m[5], m[6], m[7], m[8],
                                    m[10], m[11], m[12], m[13], m[15], m[16], m[17], m[18]};
            gl::glUniform4fv(colorMatrix_.uniform("u_rows"), 4, rows);
            gl::glUniform4f(colorMatrix_.uniform("u_offsets"), m[4] / 255.0f, m[9] / 255.0f, m[14] / 255.0f, m[19] / 255.0f);
            gl::glUniform2f(colorMatrix_.uniform("u_sourceSize"), static_cast<float>(layer.target->width()),
                            static_cast<float>(layer.target->height()));
            runPass(colorMatrix_, layer.target->texture(), *result, layer.area);
            std::swap(layer.target, result);
            releaseTarget(std::move(result));
        } else if (const auto* glow = std::get_if<swf::GlowFilter>(&filter)) {
            glowOrShadow(layer, glow->color, glow->blurX, glow->blurY, glow->strength, glow->passes, 0, 0, glow->inner,
                         glow->knockout, false);
        } else if (const auto* shadow = std::get_if<swf::DropShadowFilter>(&filter)) {
            glowOrShadow(layer, shadow->color, shadow->blurX, shadow->blurY, shadow->strength, shadow->passes,
                         static_cast<float>(std::cos(shadow->angle) * shadow->distance),
                         static_cast<float>(std::sin(shadow->angle) * shadow->distance), shadow->inner, shadow->knockout,
                         !shadow->compositeSource);
        }

        // Other filters (bevel, gradient, convolution) are not used by the game.

        // Back to drawing into the layer.
        gl::glEnable(GL_BLEND);
        gl::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        bindTarget(layer.target->framebuffer(), layer.area.width, layer.area.height);
        applyStencilState();
    }

    void Renderer::popLayer(swf::BlendMode blendMode, const ColorTransform& cx) {
        if (layers_.empty())
            return;

        Layer layer = std::move(layers_.back());
        layers_.pop_back();

        // Back to the parent target and its masks.
        bindTarget(layer.savedFramebuffer, layer.savedViewportWidth, layer.savedViewportHeight);
        stageToFramebuffer_ = layer.savedStageToTarget;
        maskDepth_ = layer.savedMaskDepth;

        // Back on the window: it keeps its letterbox clip. (Not when drawing
        // offscreen for BitmapData.draw: that target has no letterbox.)
        if (layers_.empty() && letterboxed_)
            gl::glEnable(GL_SCISSOR_TEST);

        applyStencilState();

        const PixelRect& a = layer.area;
        const float texW = static_cast<float>(layer.target->width()), texH = static_cast<float>(layer.target->height());

        const auto setQuad = [&](const Program& program) {
            gl::glUseProgram(program.id);
            gl::glActiveTexture(GL_TEXTURE0);
            gl::glBindTexture(GL_TEXTURE_2D, layer.target->texture());
            gl::glUniform1i(program.uniform("u_texture"), 0);
            gl::glUniform4f(program.uniform("u_matrix"), 1, 0, 0, 1);
            gl::glUniform2f(program.uniform("u_translate"), static_cast<float>(a.x), static_cast<float>(a.y));
            gl::glUniform2f(program.uniform("u_size"), static_cast<float>(a.width), static_cast<float>(a.height));
            gl::glUniform2f(program.uniform("u_viewport"), static_cast<float>(viewportWidth_),
                            static_cast<float>(viewportHeight_));
            // Render targets are stored bottom-up: flip while sampling.
            gl::glUniform2f(program.uniform("u_uvOffset"), 0, static_cast<float>(a.height) / texH);
            gl::glUniform2f(program.uniform("u_uvScale"), static_cast<float>(a.width) / texW,
                            -static_cast<float>(a.height) / texH);
            setColorTransform(program.id, cx);
        };

        switch (blendMode) {
        case swf::BlendMode::Normal:
        case swf::BlendMode::Layer:
            setQuad(quad_);
            gl::glUniform1i(quad_.uniform("u_maskMode"), 0);
            gl::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            break;
        case swf::BlendMode::Alpha: // keeps the destination where the layer is opaque
            setQuad(quad_);
            gl::glUniform1i(quad_.uniform("u_maskMode"), 0);
            gl::glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
            gl::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            gl::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case swf::BlendMode::Erase: // removes the destination where the layer is opaque
            setQuad(quad_);
            gl::glUniform1i(quad_.uniform("u_maskMode"), 0);
            gl::glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
            gl::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            gl::glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        default: {
            // Copy what is under the layer, then combine in the shader.
            const int x0 = std::max(0, a.x), y0 = std::max(0, a.y);
            const int x1 = std::min(viewportWidth_, a.x + a.width), y1 = std::min(viewportHeight_, a.y + a.height);
            if (x1 <= x0 || y1 <= y0)
                break;

            const int w = x1 - x0, h = y1 - y0;
            if (destinationCopy_ == 0 || destinationCopyWidth_ < w || destinationCopyHeight_ < h) {
                if (destinationCopy_ == 0)
                    gl::glGenTextures(1, &destinationCopy_);

                destinationCopyWidth_ = std::max(destinationCopyWidth_, roundUp(w, 256));
                destinationCopyHeight_ = std::max(destinationCopyHeight_, roundUp(h, 256));
                gl::glBindTexture(GL_TEXTURE_2D, destinationCopy_);
                gl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, destinationCopyWidth_, destinationCopyHeight_, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, nullptr);
                gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            const int glY = viewportHeight_ - y1; // GL rows go up
            gl::glActiveTexture(GL_TEXTURE1);
            gl::glBindTexture(GL_TEXTURE_2D, destinationCopy_);
            gl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x0, glY, w, h);

            setQuad(blend_);
            gl::glUniform1i(blend_.uniform("u_destination"), 1);
            gl::glUniform2f(blend_.uniform("u_destinationOrigin"), static_cast<float>(x0), static_cast<float>(glY));
            gl::glUniform2f(blend_.uniform("u_destinationSize"), static_cast<float>(destinationCopyWidth_),
                            static_cast<float>(destinationCopyHeight_));
            gl::glUniform1i(blend_.uniform("u_mode"), static_cast<int>(blendMode));
            gl::glDisable(GL_BLEND);
            gl::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            gl::glEnable(GL_BLEND);
            gl::glActiveTexture(GL_TEXTURE0);
            break;
        }
        }

        releaseTarget(std::move(layer.target));
    }

} // namespace engine
