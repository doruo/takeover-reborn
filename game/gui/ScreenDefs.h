// ScreenDefs.h - what screens are built from (original: gui/_SafeStr_683,
// gui/_SafeStr_678, gui/_SafeStr_691 and gui/_SafeStr_679).
#pragma once

#include "game/sim/battle/MissionStats.h"

#include <memory>
#include <string>

namespace takeover {

class HintLayer;
class KeyboardDevice;
class MouseDevice;
class Profile;
class TOResManager;

/// Original: _SafeStr_683.
struct ScreenDef {
    virtual ~ScreenDef() = default;
    /// Library class of the screen's view; empty for a plain Sprite
    /// (original: _SafeStr_757).
    std::string viewClass;
    virtual void dispose() { viewClass.clear(); }
};

/// A screen of a profile (original: _SafeStr_678).
struct GameScreenDef : ScreenDef {
    Profile* profile = nullptr;
    MouseDevice* mouse = nullptr;          ///< Original: _SafeStr_723.
    KeyboardDevice* keyboard = nullptr;    ///< Original: _SafeStr_722.
    TOResManager* resources = nullptr;  ///< Original: _SafeStr_755 (TOResManager).
    HintLayer* hintLayer = nullptr;        ///< Original: _SafeStr_751.
};

/// Original: _SafeStr_691.
struct MissionOverDef : GameScreenDef {
    std::shared_ptr<MissionResult> missionResult; ///< Original: _SafeStr_762.
    double xpEarned = 0;                          ///< Original: _SafeStr_772.
    int completeStatus = 0;                       ///< LevelProgress rank + 1.
    void dispose() override {
        missionResult.reset();
        GameScreenDef::dispose();
    }
};

/// Devices and profile without a view (original: _SafeStr_679).
struct InputDef {
    Profile* profile = nullptr;
    MouseDevice* mouse = nullptr;
    KeyboardDevice* keyboard = nullptr;
};

} // namespace takeover
