#include "game/framework/raster/RasterLayer.h"

#include <algorithm>

namespace takeover {

    // /----- RENDERABLE OBJECTS ARRAY -----/

    RenderableObjectsArray::RenderableObjectsArray() : dead_(std::make_shared<DeadObject>()) {}

    void RenderableObjectsArray::sortObjects() {
        std::stable_sort(objects_.begin(), objects_.end(),
                         [](const auto& a, const auto& b) { return a->getY() < b->getY(); });
    }

    void RenderableObjectsArray::addRGLObject(std::shared_ptr<ILayerObject> object) {
        if (count_ >= static_cast<int>(objects_.size()))
            objects_.push_back(std::move(object));
        else
            objects_[static_cast<std::size_t>(count_)] = std::move(object);

        ++count_;
    }

    bool RenderableObjectsArray::removeRGLObject(const ILayerObject* object) {
        const auto it = std::find_if(objects_.begin(), objects_.end(), [&](const auto& o) { return o.get() == object; });
        const int pos = it == objects_.end() ? -1 : static_cast<int>(it - objects_.begin());
        if (pos >= 0 && pos < count_) {
            --count_;
            objects_[static_cast<std::size_t>(pos)] = objects_[static_cast<std::size_t>(count_)];
            objects_[static_cast<std::size_t>(count_)] = dead_;

            return true;
        }

        return false;
    }

    void RenderableObjectsArray::markObjectByNullFrom(int startIndex) {
        const int count = count_;
        for (int i = startIndex; i < count; ++i) {
            objects_[static_cast<std::size_t>(i)] = dead_;
            --count_;
        }
    }

    ILayerObject* RenderableObjectsArray::findFirstHitTest(double x, double y) const {
        for (int i = count_ - 1; i >= 0; --i) {
            ILayerObject* object = objects_[static_cast<std::size_t>(i)].get();
            if (object != nullptr && object->hitTest(x, y, 0.5))
                return object;
        }

        return nullptr;
    }

    void RenderableObjectsArray::enableHighlightsForAll(bool enable) {
        for (int i = 0; i < count_; ++i) {
            ILayerObject* object = objects_[static_cast<std::size_t>(i)].get();
            if (object == nullptr)
                continue;

            if (object->getY() >= DeadObject::DEAD_LINE_Y)
                break;

            if (object->isHighlighted() != enable)
                object->setHighlighted(enable);
        }
    }

    // /----- RENDERABLE OBJECTS CONTAINER -----/

    void RenderableObjectsContainer::attach(std::shared_ptr<IRasterDrawable> drawable, bool above) {
        (above ? above_ : below_).push_back(std::move(drawable));
    }

    void RenderableObjectsContainer::removeSwapLast(std::vector<std::shared_ptr<IRasterDrawable>>& list, std::size_t index) {
        list[index] = list.back();
        list.pop_back();
    }

    void RenderableObjectsContainer::detach(const IRasterDrawable* drawable) {
        const auto find = [&](auto& list) {
            return std::find_if(list.begin(), list.end(), [&](const auto& d) { return d.get() == drawable; });
        };

        if (auto it = find(above_); it != above_.end()) {
            removeSwapLast(above_, static_cast<std::size_t>(it - above_.begin()));
        } else if (auto it2 = find(below_); it2 != below_.end()) {
            removeSwapLast(below_, static_cast<std::size_t>(it2 - below_.begin()));
        }
    }

    void RenderableObjectsContainer::drawAll(const std::vector<std::shared_ptr<IRasterDrawable>>& list,
                                             engine::BitmapData& place, double x, double y, double yScale,
                                             const engine::ColorTransform* color) {
        // A copy: drawing may detach.
        const auto snapshot = list;
        for (const auto& drawable : snapshot)
            drawable->drawTo(place, x, y, yScale, color);
    }

    void RenderableObjectsContainer::drawBelow(engine::BitmapData& place, double x, double y, double yScale,
                                               const engine::ColorTransform* color) {
        drawAll(below_, place, x, y, yScale, color);
    }

    void RenderableObjectsContainer::drawAbove(engine::BitmapData& place, double x, double y, double yScale,
                                               const engine::ColorTransform* color) {
        drawAll(above_, place, x, y, yScale, color);
    }

    // /----- RASTER SPRITE -----/

    const engine::ColorTransform RasterSprite::DEFAULT_SELECTED_COLOR{0.65f, 0.65f, 0.65f, 1, 61, 89, 0, 0};

    RasterSprite::RasterSprite(std::shared_ptr<RasterObject> rasterObject)
        : raster_(std::move(rasterObject)), selectedColor_(DEFAULT_SELECTED_COLOR) {}

    bool RasterSprite::hitTestRaster(const RasterObject& raster, double x, double y, double yScale) {
        const double px = x - raster.x + raster.anchorX();
        const double py = (y - raster.y) * yScale + raster.anchorY();
        if (raster.subImage().contains(px, py))
            return raster.bitmapData()->getPixel32(static_cast<int>(px), static_cast<int>(py)) != 0;

        return false;
    }

    void RasterSprite::drawTo(engine::BitmapData& place, double x, double y, double yScale,
                              const engine::ColorTransform* color) {
        if (!visible_)
            return;

        if (highlighted_)
            color = &selectedColor_;

        attached_.drawBelow(place, x + getX(), y + getY(), yScale, color);
        raster_->drawTo(place, x, y, yScale, color);
        attached_.drawAbove(place, x + getX(), y + getY(), yScale, color);
    }

    void RasterSprite::customDispose() {
        attached_.clear();

        if (raster_)
            raster_->dispose();

        raster_.reset();
    }

    // /----- RASTER GRAPHIC LAYER -----/

    RasterGraphicLayer::RasterGraphicLayer(int width, int height)
        : bitmap_(std::make_shared<engine::BitmapData>(width, height, true, 0)), layers_(LAYER_COUNT) {}

    void RasterGraphicLayer::addRGLObject(std::shared_ptr<ILayerObject> object, int layer) {
        layers_[static_cast<std::size_t>(layer)].addRGLObject(std::move(object));
    }

    void RasterGraphicLayer::removeRGLObject(const ILayerObject* object) {
        for (int i = LAYER_COUNT - 1; i >= 0; --i)
            if (layers_[static_cast<std::size_t>(i)].removeRGLObject(object))
                return;
    }

    void RasterGraphicLayer::render() {
        for (auto& layer : layers_)
            layer.sortObjects();

        bitmap_->lock();
        bitmap_->fillRect(bitmap_->rect(), 0);

        for (auto& layer : layers_) {
            const int count = layer.getObjectsCount();
            int i = 0;
            for (; i < count; ++i) {
                ILayerObject* object = layer.getObject(i);
                if (object->getY() >= DeadObject::DEAD_LINE_Y)
                    break;

                object->drawTo(*bitmap_, 0, 0, 0.5);
            }

            layer.markObjectByNullFrom(i);
        }

        bitmap_->unlock();
    }

    void RasterGraphicLayer::customDispose() {
        layers_.clear();
        bitmap_->dispose();
    }

} // namespace takeover
