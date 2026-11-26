// Timeline.h - frame-by-frame display list commands of a sprite or the root.
//
// Flash does not store "what is on screen at frame N". It stores a list of
// edits (place / modify / remove at a depth) applied when the playhead enters
// each frame. We keep that representation as-is: the runtime replays the
// commands, which is also how it reproduces Flash's rules for jumping
// backwards (rebuild from frame 1) and for preserving objects across frames.
#pragma once

#include "swf/Types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace swf {

    /// PlaceObject2/3. Every field is optional because a "move" only carries the
    /// properties that change.
    struct PlaceObject {
        Depth depth = 0;
        bool isMove = false; ///< Modify the object already at `depth`.
        std::optional<CharacterId> characterId;
        std::optional<Matrix> matrix;
        std::optional<ColorTransform> colorTransform;
        std::optional<std::uint16_t> ratio; ///< Morph shape blend, 0..65535.
        std::optional<std::string> name;   ///< Instance name (e.g. "butLogo").
        std::optional<Depth> clipDepth;    ///< Present when this object is a mask.
        std::optional<std::vector<Filter>> filters;
        std::optional<BlendMode> blendMode;
        std::optional<bool> cacheAsBitmap;
        std::optional<bool> visible;
        std::optional<std::string> className;
    };

    struct RemoveObject {
        Depth depth = 0;
    };

    struct SoundEnvelopePoint {
        std::uint32_t position44 = 0; ///< Sample position at 44.1 kHz.
        std::uint16_t leftLevel = 32768;
        std::uint16_t rightLevel = 32768;
    };

    struct SoundInfo {
        bool syncStop = false;
        bool syncNoMultiple = false;
        std::optional<std::uint32_t> inPoint;
        std::optional<std::uint32_t> outPoint;
        std::uint16_t loopCount = 1;
        std::vector<SoundEnvelopePoint> envelope;
    };

    struct StartSound {
        CharacterId soundId = 0;
        SoundInfo info;
    };

    using DisplayCommand = std::variant<PlaceObject, RemoveObject>;

    struct Frame {
        std::vector<DisplayCommand> commands; ///< In file order.
        std::vector<StartSound> sounds;
        std::vector<std::string> labels; ///< Usually zero or one.
    };

    struct Timeline {
        std::vector<Frame> frames;

        [[nodiscard]] int frameCount() const { return static_cast<int>(frames.size()); }
    };

} // namespace swf
