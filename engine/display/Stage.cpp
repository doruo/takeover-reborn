#include "engine/display/Stage.h"

#include "engine/display/MovieClip.h"
#include "engine/display/SimpleButton.h"

#include <algorithm>

namespace engine {

    namespace {

        std::vector<std::weak_ptr<DisplayObject>> chainOf(DisplayObject* target) {
            std::vector<std::weak_ptr<DisplayObject>> chain;

            for (DisplayObject* o = target; o != nullptr; o = o->parent())
                chain.push_back(o->weak_from_this());

            return chain;
        }

        bool chainContains(const std::vector<std::weak_ptr<DisplayObject>>& chain, const DisplayObject* object) {
            return std::any_of(chain.begin(), chain.end(), [&](const auto& w) { return w.lock().get() == object; });
        }

    } // namespace

    // /----- PICKING -----/

    Stage::Pick Stage::pick(const DisplayObjectContainer& container, Point stagePoint) const {
        for (int i = container.numChildren() - 1; i >= 0; --i) {
            DisplayObject* child = container.getChildAt(i);
            if (!child->visible() || child->clipDepth())
                continue;

            bool insideMasks = true;

            for (const DisplayObject* mask : container.masksOf(child))
                insideMasks = insideMasks && mask->hitTestPoint(stagePoint.x, stagePoint.y, true);

            if (!insideMasks)
                continue;

            // An interactive object with mouseEnabled false is transparent to
            // the mouse: what is below it gets the event (its children still
            // can, unless mouseChildren is false too).
            // A sprite with a hit area is picked by that area alone.
            if (const auto* sprite = dynamic_cast<const Sprite*>(child); sprite && sprite->hitArea()) {
                if (child->mouseEnabled() && sprite->hitArea()->hitTestPoint(stagePoint.x, stagePoint.y, true))
                    return {true, child};

                continue;
            }

            if (const auto* inner = dynamic_cast<const DisplayObjectContainer*>(child)) {
                if (!inner->mouseChildren()) {
                    if (child->mouseEnabled() && child->hitTestPoint(stagePoint.x, stagePoint.y, true))
                        return {true, child};

                    continue;
                }

                const Pick inside = pick(*inner, stagePoint);
                if (inside.target)
                    return inside;

                // A plain shape was hit: the nearest mouse-enabled container
                // receives the event.
                if (inside.hit && child->mouseEnabled())
                    return {true, child};

                continue;
            }

            if (child->hitTestPoint(stagePoint.x, stagePoint.y, true)) {
                if (!child->isInteractive())
                    return {true, nullptr}; // a shape: its container decides

                if (child->mouseEnabled())
                    return {true, child};
            }
        }

        return {};
    }

    DisplayObject* Stage::targetAt(Point stagePoint) const {
        const Pick p = pick(*this, stagePoint);
        return p.target ? p.target : const_cast<Stage*>(this);
    }

    // /----- DISPATCH -----/

    void Stage::dispatchMouse(DisplayObject* target, std::string_view type, DisplayObject* related) {
        const bool bubbles = type != MouseEvent::ROLL_OVER && type != MouseEvent::ROLL_OUT;
        MouseEvent event(type, bubbles);
        const Point local = target->globalToLocal(mouse_);
        event.localX = local.x;
        event.localY = local.y;
        event.stageX = mouse_.x;
        event.stageY = mouse_.y;
        event.buttonDown = buttonDown_;
        event.shiftKey = shift_;
        event.ctrlKey = ctrl_;
        event.altKey = alt_;
        event.relatedObject = related;
        target->dispatchEvent(event);
    }

    void Stage::applyButtonLook(DisplayObject* target, std::string_view state) {
        if (auto* button = dynamic_cast<SimpleButton*>(target)) {
            button->setState(state == "_over" ? SimpleButton::State::Over
                             : state == "_down" ? SimpleButton::State::Down
                                                : SimpleButton::State::Up);
            return;
        }

        // A MovieClip in button mode with the "_up" label behaves like a button
        // (Flash Player rule for AS3 clips).
        auto* clip = dynamic_cast<MovieClip*>(target);
        if (clip == nullptr || !clip->buttonMode() || !clip->hasLabel("_up"))
            return;

        if (clip->hasLabel(state))
            clip->gotoAndStop(state);
        else if (state == "_down" && clip->hasLabel("_over"))
            clip->gotoAndStop("_over");
    }

    void Stage::updateHover(DisplayObject* newTarget) {
        std::shared_ptr<DisplayObject> oldTarget = hover_.lock();
        if (oldTarget.get() == newTarget)
            return;

        std::shared_ptr<DisplayObject> keepNew = newTarget ? newTarget->weak_from_this().lock() : nullptr;

        const auto oldChain = hoverChain_;
        const auto newChain = chainOf(newTarget);
        hover_ = newTarget ? newTarget->weak_from_this() : std::weak_ptr<DisplayObject>{};
        hoverChain_ = newChain;

        if (oldTarget) {
            dispatchMouse(oldTarget.get(), MouseEvent::MOUSE_OUT, newTarget);
            applyButtonLook(oldTarget.get(), "_up");

            for (const auto& weak : oldChain)
                if (auto o = weak.lock(); o && !chainContains(newChain, o.get()))
                    dispatchMouse(o.get(), MouseEvent::ROLL_OUT, newTarget);
        }

        if (newTarget) {
            for (auto it = newChain.rbegin(); it != newChain.rend(); ++it)
                if (auto o = it->lock(); o && !chainContains(oldChain, o.get()))
                    dispatchMouse(o.get(), MouseEvent::ROLL_OVER, oldTarget.get());
            dispatchMouse(newTarget, MouseEvent::MOUSE_OVER, oldTarget.get());
            const bool pressedHere = buttonDown_ && pressed_.lock().get() == newTarget;
            applyButtonLook(newTarget, pressedHere ? "_down" : "_over");
        }
    }

    void Stage::refreshMouseTarget() {
        if (mouse_.x < 0 || mouse_.y < 0)
            return; // cursor outside the window

        updateHover(targetAt(mouse_));
    }

    void Stage::mouseMoved(double stageX, double stageY) {
        mouse_ = {stageX, stageY};
        DisplayObject* target = targetAt(mouse_);
        updateHover(target);
        dispatchMouse(target, MouseEvent::MOUSE_MOVE);
    }

    void Stage::mouseButton(bool pressed) {
        DisplayObject* target = targetAt(mouse_);
        updateHover(target);
        std::shared_ptr<DisplayObject> keep = target->weak_from_this().lock();

        if (pressed) {
            buttonDown_ = true;
            pressed_ = target->weak_from_this();
            dispatchMouse(target, MouseEvent::MOUSE_DOWN);
            applyButtonLook(target, "_down");
        } else {
            buttonDown_ = false;
            dispatchMouse(target, MouseEvent::MOUSE_UP);
            const bool click = pressed_.lock().get() == target;
            pressed_.reset();

            if (click)
                dispatchMouse(target, MouseEvent::CLICK);

            // The click may have removed or replaced the target.
            if (hover_.lock().get() == target)
                applyButtonLook(target, "_over");
        }
    }

    void Stage::mouseWheel(int lines) {
        DisplayObject* target = targetAt(mouse_);
        MouseEvent event(MouseEvent::MOUSE_WHEEL);
        event.delta = lines;
        event.stageX = mouse_.x;
        event.stageY = mouse_.y;
        target->dispatchEvent(event);
    }

    void Stage::key(bool pressed, unsigned keyCode, unsigned charCode, bool shift, bool ctrl, bool alt) {
        shift_ = shift;
        ctrl_ = ctrl;
        alt_ = alt;
        KeyboardEvent event(pressed ? KeyboardEvent::KEY_DOWN : KeyboardEvent::KEY_UP);
        event.keyCode = keyCode;
        event.charCode = charCode;
        event.shiftKey = shift;
        event.ctrlKey = ctrl;
        event.altKey = alt;
        dispatchEvent(event); // the stage always has the focus in this game
    }

    bool Stage::wantsHandCursor() const {
        for (DisplayObject* o = hover_.lock().get(); o != nullptr; o = o->parent()) {
            if (const auto* sprite = dynamic_cast<const Sprite*>(o))
                if (sprite->buttonMode())
                    return sprite->useHandCursor();
            if (dynamic_cast<const SimpleButton*>(o))
                return true;
        }

        return false;
    }

    void Stage::advanceFrame() {
        DisplayObjectContainer::advanceFrame();
        EventDispatcher::broadcastEnterFrame();
        refreshMouseTarget();
    }

} // namespace engine
