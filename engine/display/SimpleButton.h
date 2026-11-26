// SimpleButton.h - flash.display.SimpleButton: four state sprites (up, over,
// down, hit area) built from a DefineButton2 record list.
#pragma once

#include "engine/display/DisplayObjectContainer.h"
#include "swf/Characters.h"

namespace engine {

class Library;

class SimpleButton : public DisplayObject {
public:
    enum class State { Up, Over, Down };

    SimpleButton(Library& library, const swf::ButtonDef& definition);

    void setState(State state) { state_ = state; }
    [[nodiscard]] State state() const { return state_; }

    [[nodiscard]] Sprite& upState() { return *up_; }
    [[nodiscard]] Sprite& overState() { return *over_; }
    [[nodiscard]] Sprite& downState() { return *down_; }
    [[nodiscard]] Sprite& hitTestState() { return *hit_; }

    [[nodiscard]] Rectangle localBounds() const override;
    [[nodiscard]] bool hitTestLocal(Point local) const override { return hit_->hitTestLocal(local); }
    [[nodiscard]] bool isInteractive() const override { return true; }
    void advanceFrame() override;

protected:
    void draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) override;

private:
    [[nodiscard]] Sprite& current() const;

    std::shared_ptr<Sprite> up_, over_, down_, hit_;
    State state_ = State::Up;
};

} // namespace engine
