#include "game/level/LevelLand.h"

#include "game/framework/Symbols.h"

#include "engine/display/MovieClip.h"
#include "engine/display/Shape.h"

namespace takeover {

// --- LevelLandObject -------------------------------------------------------------------------------

LevelLandObject::LevelLandObject(int maxObjectIndex) : objects_(static_cast<std::size_t>(maxObjectIndex)) {}

void LevelLandObject::setView(int index, engine::DisplayObject* source) {
    index_ = index;
    auto& object = objects_[static_cast<std::size_t>(index_)];
    if (object == nullptr) {
        const engine::Point p = source->localToGlobal({0, 0});
        object = RasterObject::fromDisplayObject(*source, p.x, p.y / TRANSFORM_Y);
    }
}

void LevelLandObject::drawTo(engine::BitmapData& place, double x, double y, double yScale,
                             const engine::ColorTransform* color) {
    if (!visible_) return;
    attached_.drawBelow(place, x + getX(), y + getY(), yScale, color);
    objects_[static_cast<std::size_t>(index_)]->drawTo(place, x, y, yScale, color);
    attached_.drawAbove(place, x + getX(), y + getY(), yScale, color);
}

void LevelLandObject::customDispose() {
    for (auto& object : objects_)
        if (object) object->dispose();
    objects_.clear();
    attached_.clear();
}

// --- LevelLand -----------------------------------------------------------------------------------------

LevelLand& LevelLand::init(LevelLandDef def) {
    def_ = std::move(def);
    ground_ = std::make_shared<engine::BitmapData>(static_cast<int>(def_.width), static_cast<int>(def_.height), false, 0);
    groundBitmap_ = std::make_shared<engine::Bitmap>(ground_);
    return *this;
}

void LevelLand::redraw() {
    std::shared_ptr<engine::MovieClip> map = symbols::create(def_.mapClass);
    for (int location = 0; location < static_cast<int>(def_.locationViews.size()); ++location) {
        showLocation(*map, location, def_.locationViews[static_cast<std::size_t>(location)]);
        if (auto* objects = symbols::child<engine::DisplayObject>(*map, "mcObjects" + std::to_string(location)))
            objects->setVisible(false);
    }
    ground_->draw(*map);
}

void LevelLand::showLocation(engine::Sprite& map, int location, int view) {
    const int frame = view + 1;
    for (const char* layerName : {"mcArea", "mcRoads"}) {
        auto* layer = symbols::child<engine::DisplayObjectContainer>(map, layerName + std::to_string(location));
        if (layer == nullptr) continue;
        for (int i = 0; i < layer->numChildren(); ++i)
            if (auto* clip = dynamic_cast<engine::MovieClip*>(layer->getChildAt(i))) clip->gotoAndStop(frame);
    }
    auto* layer = symbols::child<engine::DisplayObjectContainer>(map, "mcObjects" + std::to_string(location));
    while (static_cast<int>(objects_.size()) <= location) objects_.emplace_back();
    if (layer == nullptr) return;
    auto& locationObjects = objects_[static_cast<std::size_t>(location)];
    int counter = 0;
    for (int i = 0; i < layer->numChildren(); ++i) {
        auto* clip = dynamic_cast<engine::MovieClip*>(layer->getChildAt(i));
        if (clip == nullptr) return; // like the original: stops at the first non-clip
        clip->gotoAndStop(frame);
        while (static_cast<int>(locationObjects.size()) <= counter)
            locationObjects.push_back(std::make_shared<LevelLandObject>());
        locationObjects[static_cast<std::size_t>(counter)]->setView(view, clip);
        ++counter;
    }
}

void LevelLand::customDispose() {
    for (auto& location : objects_)
        for (auto& object : location)
            if (object) object->dispose();
    objects_.clear();
}

} // namespace takeover
