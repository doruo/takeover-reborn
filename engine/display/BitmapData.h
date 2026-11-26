// BitmapData.h - flash.display.BitmapData: an image in memory that the game
// paints into pixel by pixel.
//
// Take Over draws its battlefield like many Flash games of its time: unit
// animations are pre-rendered into bitmaps once (draw), then every frame the
// visible sprites are copied into one 800x600 bitmap (fillRect, copyPixels)
// that a Bitmap shows on screen. Pixels stay in main memory, as in Flash, so
// the game can also read them back (getPixel, hit masks).
//
//     auto layer = std::make_shared<BitmapData>(800, 600);
//     layer->fillRect(layer->rect(), 0);
//     layer->copyPixels(frame, frame.rect(), {x, y}, true);
//     stage.addChild(std::make_shared<Bitmap>(layer));
//
// Pixels are stored premultiplied, BGRA (the layout of vector::Image and of
// the GL textures), like the Flash Player does internally.
#pragma once

#include "engine/geom/Matrix.h"
#include "vector/Rasterizer.h"

#include <cstdint>
#include <memory>

namespace engine {

    class DisplayObject;

    /// An integer rectangle of pixels.
    struct PixelBox {
        int x = 0, y = 0, width = 0, height = 0;

        [[nodiscard]] bool contains(double px, double py) const {
            return px >= x && py >= y && px < x + width && py < y + height;
        }
    };

    class BitmapData {
    public:
        /// A `width` x `height` image filled with `fillColor` (0xAARRGGBB; with
        /// `transparent` false the alpha is forced to 0xFF).
        BitmapData(int width, int height, bool transparent = true, std::uint32_t fillColor = 0xFFFFFFFF);

        [[nodiscard]] int width() const { return image_.width; }
        [[nodiscard]] int height() const { return image_.height; }
        [[nodiscard]] PixelBox rect() const { return {0, 0, width(), height()}; }
        [[nodiscard]] bool transparent() const { return transparent_; }
        [[nodiscard]] const vector::Image& image() const { return image_; }
        /// Changes every time the pixels change (the Bitmap showing it then
        /// uploads them again).
        [[nodiscard]] std::uint64_t version() const { return version_; }

        [[nodiscard]] std::shared_ptr<BitmapData> clone() const;

        /// 0xRRGGBB of a pixel, un-multiplied; 0 outside the image.
        [[nodiscard]] std::uint32_t getPixel(int x, int y) const;

        /// 0xAARRGGBB of a pixel, un-multiplied; 0 outside the image.
        [[nodiscard]] std::uint32_t getPixel32(int x, int y) const;
        void setPixel32(int x, int y, std::uint32_t argb);

        void fillRect(const PixelBox& rect, std::uint32_t argb);

        /// Copies `sourceRect` of `source` to `destPoint`. With `mergeAlpha`, the
        /// source is blended over the destination ("normal" blend mode);
        /// otherwise its pixels replace the destination's.
        void copyPixels(const BitmapData& source, const PixelBox& sourceRect, Point destPoint, bool mergeAlpha = false);

        /// Applies a color transform to the pixels of `rect`.
        void colorTransform(const PixelBox& rect, const ColorTransform& transform);

        /// Renders a display object into the bitmap, over the current pixels.
        /// `matrix` replaces the object's own transform (as in Flash).
        void draw(DisplayObject& source, const Matrix& matrix = {}, const ColorTransform& color = {});

        /// Flash's lock()/unlock() only delay screen updates: nothing to do here.
        void lock() {}
        void unlock() {}
        /// Frees the pixels (the object is unusable afterwards).
        void dispose();

        /// Marks the pixels as changed (after editing image() directly).
        void touch() { ++version_; }
        /// Direct access to the premultiplied BGRA pixels; call touch() after.
        [[nodiscard]] vector::Image& pixels() { return image_; }

    private:
        [[nodiscard]] PixelBox clip(const PixelBox& rect) const;

        vector::Image image_;
        bool transparent_ = true;
        std::uint64_t version_ = 1;
    };

} // namespace engine
