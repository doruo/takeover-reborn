#include "engine/display/MovieClip.h"

#include "engine/assets/Library.h"
#include "engine/audio/Sound.h"
#include "engine/display/Shape.h"
#include "engine/display/SimpleButton.h"
#include "engine/display/TextField.h"

#include <algorithm>
#include <variant>

namespace engine {

namespace {

/// The net effect of the skipped frames on one depth during a goto.
struct PendingDepth {
    bool remove = false;           ///< The depth ends up empty.
    bool removedBeforePlace = false; ///< A remove preceded the placement: force a new instance.
    std::optional<swf::PlaceObject> place; ///< Accumulated placement (may be a move only).
    int placeFrame = 0;
};

/// Merges a "move" record into an accumulated placement: fields present in
/// the move override, others keep their accumulated value.
void mergeMove(swf::PlaceObject& into, const swf::PlaceObject& move) {
    if (move.matrix) into.matrix = move.matrix;
    if (move.colorTransform) into.colorTransform = move.colorTransform;
    if (move.ratio) into.ratio = move.ratio;
    if (move.name) into.name = move.name;
    if (move.clipDepth) into.clipDepth = move.clipDepth;
    if (move.filters) into.filters = move.filters;
    if (move.blendMode) into.blendMode = move.blendMode;
    if (move.visible) into.visible = move.visible;
    if (move.cacheAsBitmap) into.cacheAsBitmap = move.cacheAsBitmap;
}

} // namespace

namespace {

using SymbolScripts = std::map<std::uint16_t, std::map<int, std::function<void(MovieClip&)>>>;

SymbolScripts& symbolScripts() {
    static SymbolScripts scripts;
    return scripts;
}

} // namespace

void MovieClip::registerSymbolScript(std::uint16_t spriteId, int frameIndex, std::function<void(MovieClip&)> script) {
    symbolScripts()[spriteId][frameIndex + 1] = std::move(script);
}

MovieClip::MovieClip(Library& library, const swf::Timeline& timeline, std::uint16_t spriteId)
    : library_(&library), timeline_(&timeline), spriteId_(spriteId) {
    for (int i = 0; i < timeline.frameCount(); ++i)
        for (const auto& label : timeline.frames[static_cast<std::size_t>(i)].labels) labels_.push_back({label, i + 1});
    // Enter frame 1: behave as a jump from an imaginary frame 0.
    currentFrame_ = 0;
    gotoFrame(1);
}

int MovieClip::totalFrames() const { return timeline_ ? std::max(1, timeline_->frameCount()) : 1; }

std::string MovieClip::currentLabel() const {
    std::string label;
    for (const auto& l : labels_)
        if (l.frame <= currentFrame_) label = l.name;
    return label;
}

int MovieClip::frameForLabel(std::string_view label) const {
    for (const auto& l : labels_)
        if (l.name == label) return l.frame;
    return 0;
}

void MovieClip::gotoAndStop(int frame) {
    playing_ = false;
    gotoFrame(frame);
}

void MovieClip::gotoAndPlay(int frame) {
    playing_ = true;
    gotoFrame(frame);
}

void MovieClip::gotoAndStop(std::string_view label) {
    if (const int frame = frameForLabel(label)) gotoAndStop(frame);
}

void MovieClip::gotoAndPlay(std::string_view label) {
    if (const int frame = frameForLabel(label)) gotoAndPlay(frame);
}

void MovieClip::nextFrame() {
    playing_ = false;
    if (currentFrame_ < totalFrames()) gotoFrame(currentFrame_ + 1);
}

void MovieClip::prevFrame() {
    playing_ = false;
    if (currentFrame_ > 1) gotoFrame(currentFrame_ - 1);
}

void MovieClip::addFrameScript(int frameIndex, std::function<void()> script) {
    if (script) frameScripts_[frameIndex + 1] = std::move(script);
    else frameScripts_.erase(frameIndex + 1);
}

void MovieClip::advanceFrame() {
    // Children first: objects the parent places this frame start playing on
    // the next one, like in Flash.
    DisplayObjectContainer::advanceFrame();
    if (!playing_ || totalFrames() <= 1) return;
    gotoFrame(currentFrame_ < totalFrames() ? currentFrame_ + 1 : 1);
}

DisplayObjectPtr MovieClip::instantiate(std::uint16_t characterId) const {
    return library_ ? createDisplayObject(*library_, characterId) : nullptr;
}

void MovieClip::gotoFrame(int target) {
    target = std::clamp(target, 1, totalFrames());
    if (target == currentFrame_ || timeline_ == nullptr) {
        currentFrame_ = std::clamp(target, 1, totalFrames());
        return;
    }

    const bool rewind = target < currentFrame_;
    if (rewind) {
        // Objects placed after the target frame cannot exist there.
        for (int i = numChildren() - 1; i >= 0; --i) {
            const DisplayObject* child = getChildAt(i);
            if (child->placedByTimeline() && child->placeFrame() > target) removeChildAt(i);
        }
    }

    // Collapse the edits of every frame we pass through into one net edit
    // per depth.
    std::map<int, PendingDepth> pending;
    const int firstFrame = rewind ? 1 : currentFrame_ + 1;
    for (int frame = firstFrame; frame <= target; ++frame) {
        for (const auto& command : timeline_->frames[static_cast<std::size_t>(frame - 1)].commands) {
            if (const auto* remove = std::get_if<swf::RemoveObject>(&command)) {
                PendingDepth& p = pending[remove->depth];
                p.remove = true;
                p.place.reset();
                continue;
            }
            const auto& place = std::get<swf::PlaceObject>(command);
            PendingDepth& p = pending[place.depth];
            if (place.characterId && !place.isMove) {
                // A new character at this depth.
                p.removedBeforePlace = p.remove;
                p.remove = false;
                p.place = place;
                p.placeFrame = frame;
            } else if (p.place) {
                mergeMove(*p.place, place);
                if (place.characterId) p.place->characterId = place.characterId; // character swap
            } else if (!p.remove) {
                p.place = place; // modifies an object that already exists
            }
        }
    }
    currentFrame_ = target;

    for (const auto& [depth, p] : pending) {
        DisplayObject* existing = timelineChildAtDepth(depth);
        if (p.remove && !p.place) {
            if (existing) removeChild(existing);
            continue;
        }
        if (!p.place) continue;
        const swf::PlaceObject& place = *p.place;

        if (!place.characterId || place.isMove) {
            // Pure modification of an existing object.
            if (existing) {
                if (place.isMove && place.characterId && *place.characterId != existing->characterId()) {
                    // Move with a new character: replace, keeping the transform.
                    if (DisplayObjectPtr replacement = instantiate(*place.characterId)) {
                        replacement->setMatrix(existing->matrix());
                        replacement->setColorTransform(existing->colorTransform());
                        replacement->setName(existing->name());
                        replacement->characterId_ = *place.characterId;
                        replacement->placeFrame_ = existing->placeFrame();
                        replacement->applyPlacement(place);
                        removeChild(existing);
                        insertAtDepth(std::move(replacement), depth);
                    }
                } else {
                    existing->applyPlacement(place);
                }
            }
            continue;
        }

        const bool reuse = existing && !p.removedBeforePlace && existing->characterId() == *place.characterId &&
                           existing->placeFrame() <= p.placeFrame;
        if (reuse) {
            // Same character kept across the jump: reset it to the state the
            // timeline describes at the target frame.
            existing->setMatrix(place.matrix ? Matrix::fromSwf(*place.matrix) : Matrix{});
            existing->setColorTransform(place.colorTransform ? ColorTransform::fromSwf(*place.colorTransform)
                                                             : ColorTransform{});
            existing->setRatio(place.ratio.value_or(0));
            existing->applyPlacement(place);
            continue;
        }

        if (existing) removeChild(existing);
        DisplayObjectPtr object = instantiate(*place.characterId);
        if (!object) continue;
        object->characterId_ = *place.characterId;
        object->placeFrame_ = p.placeFrame;
        object->applyPlacement(place);
        insertAtDepth(std::move(object), depth);
    }

    runFrameActions();
}

void MovieClip::runFrameActions() {
    // Sounds placed on the frame (StartSound).
    AudioSystem* audio = timelineSoundsMuted ? nullptr : AudioSystem::instance();
    if (audio != nullptr)
        for (const auto& start : timeline_->frames[static_cast<std::size_t>(currentFrame_ - 1)].sounds)
            audio->playTimelineSound(start.soundId, start.info.syncStop, start.info.syncNoMultiple,
                                     start.info.loopCount);
    // Scripts: the symbol's own, then the ones added by code.
    if (auto symbol = symbolScripts().find(spriteId_); symbol != symbolScripts().end())
        if (auto script = symbol->second.find(currentFrame_); script != symbol->second.end()) script->second(*this);
    if (auto script = frameScripts_.find(currentFrame_); script != frameScripts_.end()) script->second();
}

DisplayObjectPtr createDisplayObject(Library& library, std::uint16_t characterId) {
    const swf::Character* character = library.find(characterId);
    if (!character) {
        // Bitmaps live outside library.swf (bitmaps/*.png).
        if (library.bitmap(characterId)) return std::make_shared<Bitmap>(library, characterId);
        return nullptr;
    }
    DisplayObjectPtr object = std::visit(
        [&](const auto& def) -> DisplayObjectPtr {
            using T = std::decay_t<decltype(def)>;
            if constexpr (std::is_same_v<T, swf::ShapeDef> || std::is_same_v<T, swf::MorphShapeDef> ||
                          std::is_same_v<T, swf::TextDef>)
                return std::make_shared<Shape>(library, characterId);
            else if constexpr (std::is_same_v<T, swf::SpriteDef>)
                return std::make_shared<MovieClip>(library, def.timeline, characterId);
            else if constexpr (std::is_same_v<T, swf::ButtonDef>)
                return std::make_shared<SimpleButton>(library, def);
            else if constexpr (std::is_same_v<T, swf::EditTextDef>)
                return std::make_shared<TextField>(library, def);
            else
                return nullptr; // fonts, sounds: not display objects
        },
        *character);
    return object;
}

} // namespace engine
