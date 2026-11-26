#include "game/framework/raster/RasterObject.h"

#include "engine/display/DisplayObject.h"

namespace takeover {

    RasterObject::RasterObject(std::shared_ptr<engine::BitmapData> bitmap, double px, double py, double anchorX,
                               double anchorY, std::optional<engine::PixelBox> subImage)
        : x(px), y(py), bitmap_(std::move(bitmap)), anchorX_(anchorX), anchorY_(anchorY) {
        subImage_ = subImage ? *subImage : bitmap_->rect();
    }

    std::shared_ptr<RasterObject> RasterObject::fromDisplayObject(engine::DisplayObject& source, double px, double py) {
        const engine::Rectangle frame = source.getBounds(&source);
        const double scaleX = source.scaleX(), scaleY = source.scaleY();
        auto bitmap = std::make_shared<engine::BitmapData>(static_cast<int>(frame.width * scaleX),
                                                           static_cast<int>(frame.height * scaleY), true, 0);
        const engine::Matrix matrix = engine::Matrix::translation(-frame.x, -frame.y).then(engine::Matrix::scale(scaleX, scaleY));
        bitmap->draw(source, matrix);

        return std::make_shared<RasterObject>(bitmap, px, py, -frame.x * scaleX, -frame.y * scaleY);
    }

    void RasterObject::drawTo(engine::BitmapData& place, double offsetX, double offsetY, double yScale,
                              const engine::ColorTransform* color) {
        const engine::Point at{offsetX + x - anchorX_, (offsetY + y) * yScale - anchorY_};

        if (color != nullptr) {
            const std::shared_ptr<engine::BitmapData> tinted = bitmap_->clone();
            tinted->colorTransform(subImage_, *color);
            place.copyPixels(*tinted, subImage_, at, true);
        } else {
            place.copyPixels(*bitmap_, subImage_, at, true);
        }
    }

    bool RasterObject::hitTest(double px, double py) const {
        const double localX = px - x + anchorX_;
        const double localY = py - y + anchorY_;
        if (subImage_.contains(localX, localY))
            return bitmap_->getPixel(static_cast<int>(localX), static_cast<int>(localY)) != 0;

        return false;
    }

    std::shared_ptr<RasterObject> RasterObject::clone() const {
        return std::make_shared<RasterObject>(bitmap_->clone(), x, y, anchorX_, anchorY_);
    }

} // namespace takeover
