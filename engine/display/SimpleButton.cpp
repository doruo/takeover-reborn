#include "engine/display/SimpleButton.h"

#include "engine/display/MovieClip.h"

namespace engine {

SimpleButton::SimpleButton(Library& library, const swf::ButtonDef& definition)
    : up_(std::make_shared<Sprite>()), over_(std::make_shared<Sprite>()), down_(std::make_shared<Sprite>()),
      hit_(std::make_shared<Sprite>()) {
    for (const auto& record : definition.records) {
        const auto addTo = [&](Sprite& state) {
            DisplayObjectPtr object = createDisplayObject(library, record.characterId);
            if (!object) return;
            object->setMatrix(Matrix::fromSwf(record.matrix));
            object->setColorTransform(ColorTransform::fromSwf(record.colorTransform));
            object->setFilters(record.filters);
            object->setBlendMode(record.blendMode);
            state.addChild(std::move(object));
        };
        if (record.stateUp) addTo(*up_);
        if (record.stateOver) addTo(*over_);
        if (record.stateDown) addTo(*down_);
        if (record.stateHitTest) addTo(*hit_);
    }
}

Sprite& SimpleButton::current() const {
    switch (state_) {
    case State::Over: return *over_;
    case State::Down: return *down_;
    default: return *up_;
    }
}

Rectangle SimpleButton::localBounds() const { return current().localBounds(); }

void SimpleButton::advanceFrame() { current().advanceFrame(); }

void SimpleButton::draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) {
    if (asMask) current().renderMask(context, world);
    else current().render(context, world, color);
}

} // namespace engine
