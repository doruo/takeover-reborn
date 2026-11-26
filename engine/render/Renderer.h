// Renderer.h - draws textured quads the way Flash composites display objects.
//
// Everything the display list shows ends up as "a texture, an affine matrix,
// a color transform": shapes are rasterized to textures (ShapeCache), bitmaps
// already are. Coordinates passed in are stage pixels (800x600); the renderer
// maps the stage to the framebuffer, scaled and centred, like Flash's
// StageScaleMode.SHOW_ALL.
//
// Three Flash features need more than one quad:
//
//  * Masks use the stencil buffer (beginMask/endMask/beginUnmask/endUnmask).
//  * Filters (blur, glow, drop shadow, color matrix) and blend modes need the
//    object as an image first. The object is drawn into an offscreen *layer*
//    (pushLayer/popLayer), filters are applied to that image, then it is
//    composited into the target below with its blend mode (compositeLayer).
//    That is exactly what the Flash Player does internally.
//  * Blend modes other than "normal" read the destination: the area under
//    the layer is copied to a texture and a shader combines both images with
//    the Flash formulas.
#pragma once

#include "engine/geom/Matrix.h"
#include "engine/render/Gl.h"
#include "engine/render/Texture.h"
#include "swf/Types.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace engine {

/// An offscreen framebuffer with a color texture and a stencil buffer.
class RenderTarget {
public:
    RenderTarget(int width, int height);
    ~RenderTarget();
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void bind() const;
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] GLuint texture() const { return color_; }
    [[nodiscard]] GLuint framebuffer() const { return framebuffer_; }

    /// Reads the pixels back as straight-alpha RGBA, top row first.
    [[nodiscard]] std::vector<std::uint8_t> readRgba() const;
    /// Reads the pixels as they are stored (premultiplied), BGRA, top row first.
    [[nodiscard]] vector::Image readPremultipliedBgra() const;

private:
    GLuint framebuffer_ = 0;
    GLuint color_ = 0;
    GLuint stencil_ = 0;
    int width_ = 0;
    int height_ = 0;
};

/// An axis-aligned rectangle in framebuffer pixels (y down).
struct PixelRect {
    int x = 0, y = 0, width = 0, height = 0;
    [[nodiscard]] bool empty() const { return width <= 0 || height <= 0; }
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /// Starts a frame on the currently bound framebuffer of the given size.
    /// The stage (stageWidth x stageHeight) is fitted inside, letterboxed.
    void beginFrame(int framebufferWidth, int framebufferHeight, int stageWidth, int stageHeight,
                    float clearR, float clearG, float clearB);

    /// Starts drawing into the bound framebuffer, `width` x `height`, cleared
    /// transparent, with stage coordinates = pixels (for BitmapData.draw).
    void beginOffscreen(int width, int height);
    /// Stage-to-framebuffer scale: how many framebuffer pixels per stage pixel.
    [[nodiscard]] double stageScale() const { return stageToFramebuffer_.a; }
    /// Maps stage coordinates to the pixels of the current target.
    [[nodiscard]] const Matrix& stageToTarget() const { return stageToFramebuffer_; }
    /// The pixels of the current target that can be drawn to.
    [[nodiscard]] PixelRect targetRect() const { return {0, 0, viewportWidth_, viewportHeight_}; }

    /// Draws `texture` so that texel (u, v) lands at `textureToStage(u, v)`.
    void drawTexture(const Texture& texture, const Matrix& textureToStage, const ColorTransform& colorTransform);

    // --- Masks ---------------------------------------------------------------
    // beginMask(); <draw the mask shapes>; endMask(); <draw masked content>;
    // then, once the masked content is done: beginUnmask(); <draw the same mask
    // shapes again>; endUnmask().

    void beginMask();
    void endMask();
    void beginUnmask();
    void endUnmask();

    // --- Layers, filters, blend modes ---------------------------------------------

    /// Redirects drawing to a transparent offscreen image covering `area`
    /// (pixels of the current target). Stage coordinates keep working.
    void pushLayer(const PixelRect& area);
    /// Applies a filter to the current layer (the one pushed last).
    void applyFilter(const swf::Filter& filter);
    /// Ends the current layer and composites it into the target below.
    void popLayer(swf::BlendMode blendMode, const ColorTransform& colorTransform);

    /// Extra pixels a filter needs around the object (blur spread, shadow
    /// offset), in framebuffer pixels.
    [[nodiscard]] int filterPadding(const swf::Filter& filter) const;

private:
    struct Program {
        GLuint id = 0;
        GLint uniform(const char* name) const;
    };
    struct Layer {
        std::unique_ptr<RenderTarget> target;
        PixelRect area;          ///< In the parent target's pixels.
        Matrix savedStageToTarget;
        GLuint savedFramebuffer = 0;
        int savedViewportWidth = 0, savedViewportHeight = 0;
        int savedMaskDepth = 0;
    };

    void applyStencilState();
    void bindTarget(GLuint framebuffer, int width, int height);
    std::unique_ptr<RenderTarget> acquireTarget(int width, int height);
    void releaseTarget(std::unique_ptr<RenderTarget> target);
    /// Runs a fullscreen pass of `program` reading `source` into `destination`
    /// (same size; only the layer's area is processed).
    void runPass(const Program& program, GLuint source, RenderTarget& destination, const PixelRect& area);
    void blurInPlace(Layer& layer, float blurX, float blurY, int passes);
    void glowOrShadow(Layer& layer, const swf::Rgba& color, float blurX, float blurY, float strength, int passes,
                      float offsetX, float offsetY, bool inner, bool knockout, bool hideObject);

    Program quad_;       ///< Textured quad with color transform (everything normal).
    Program blur_;       ///< One box-blur pass along one axis.
    Program colorMatrix_;
    Program glow_;       ///< Glow / drop shadow composite.
    Program blend_;      ///< Shader blend modes (reads a copy of the destination).
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    Matrix stageToFramebuffer_;
    GLuint currentFramebuffer_ = 0;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    enum class StencilMode { Normal, WritingMask, ErasingMask };
    StencilMode stencilMode_ = StencilMode::Normal;
    int maskDepth_ = 0;
    bool letterboxed_ = false; ///< Drawing to the window, clipped to the stage (beginFrame).

    std::vector<Layer> layers_;
    std::vector<std::unique_ptr<RenderTarget>> pool_;
    GLuint destinationCopy_ = 0; ///< Scratch texture for shader blend modes.
    int destinationCopyWidth_ = 0, destinationCopyHeight_ = 0;
};

} // namespace engine
