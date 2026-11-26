// RasterObject.h - a picture copied into a bitmap at a position (original:
// _SafePkg_1310/_SafeStr_1616 and the interface _SafeCls_1544).
//
// The battlefield is not made of display objects: every frame, each visible
// thing copies its current picture into the 800x600 battlefield bitmap
// (see RasterGraphicLayer). A RasterObject is the simplest such thing: one
// BitmapData with an anchor point.
#pragma once

#include "game/framework/core/Disposable.h"

#include "engine/display/BitmapData.h"

#include <memory>
#include <optional>

namespace engine {
    class DisplayObject;
}

namespace takeover {

    /// Something that paints itself into a bitmap (original: _SafeCls_1544).
    class IRasterDrawable {
    public:
        virtual ~IRasterDrawable() = default;

        /// Copies the picture into `place`, offset by (x, y). The y position is
        /// multiplied by `yScale` (the battlefield is stored twice as tall as
        /// it is shown). A color transform tints the copy (original: _SafeStr_1545).
        virtual void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                            const engine::ColorTransform* color = nullptr) = 0;
    };

    class RasterObject : public Disposable, public IRasterDrawable {
    public:
        /// `anchorX/Y`: the pixel of the bitmap placed at (x, y).
        RasterObject(std::shared_ptr<engine::BitmapData> bitmap, double x, double y, double anchorX = 0, double anchorY = 0,
                     std::optional<engine::PixelBox> subImage = std::nullopt);

        /// Renders a display object into a new RasterObject placed at (x, y)
        /// (original: _SafeStr_1634).
        static std::shared_ptr<RasterObject> fromDisplayObject(engine::DisplayObject& source, double x, double y);

        [[nodiscard]] const engine::PixelBox& subImage() const { return subImage_; } ///< _SafeStr_2803
        [[nodiscard]] const std::shared_ptr<engine::BitmapData>& bitmapData() const { return bitmap_; }
        void setBitmapData(std::shared_ptr<engine::BitmapData> bitmap) { bitmap_ = std::move(bitmap); }
        [[nodiscard]] double anchorX() const { return anchorX_; } ///< _SafeStr_2801
        [[nodiscard]] double anchorY() const { return anchorY_; } ///< _SafeStr_2802
        void setAnchorX(double value) { anchorX_ = value; }
        void setAnchorY(double value) { anchorY_ = value; }

        double x = 0;
        double y = 0;

        void drawTo(engine::BitmapData& place, double offsetX = 0, double offsetY = 0, double yScale = 1,
                    const engine::ColorTransform* color = nullptr) override;
        /// Is there a non-black pixel at (px, py)? (original: _SafeStr_3054).
        [[nodiscard]] bool hitTest(double px, double py) const;
        [[nodiscard]] std::shared_ptr<RasterObject> clone() const;

    protected:
        void customDispose() override { bitmap_.reset(); }

    private:
        engine::PixelBox subImage_;               ///< Original: _SafeStr_3052.
        std::shared_ptr<engine::BitmapData> bitmap_; ///< Original: _SafeStr_1779.
        double anchorX_ = 0;                      ///< Original: _SafeStr_3049.
        double anchorY_ = 0;                      ///< Original: _SafeStr_3050.
    };

} // namespace takeover
