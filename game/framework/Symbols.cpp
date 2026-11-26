#include "game/framework/Symbols.h"

#include "engine/platform/Application.h"

#include <set>
#include <stdexcept>
#include <string>

namespace takeover::symbols {

namespace {

/// Exported classes declared `extends Sprite` in the original: they only
/// ever show their first frame.
const std::set<std::string, std::less<>>& spriteClasses() {
    static const std::set<std::string, std::less<>> classes = {
        "GamePanelClass", "ResourcePanelClass", "scrPauseClass", "scrYesVsNoClass", "scrBattleClass",
        "scrCompaignMapClass", "scrChooseCampaignClass", "scrTitleClass", "scrMissionOverClass",
        "SmallLogoPanelA10Class", "SmallLogoPanelClass", "VersionNumClass",
    };
    return classes;
}

bool isSpriteClass(std::string_view name) {
    if (spriteClasses().contains(name)) return true;
    // Unit icons and level clips are Sprites too.
    const bool unitIcon = name.starts_with("IconT") && name.ends_with("Class");
    const bool level = name.starts_with("Level") && name.ends_with("Class") && !name.ends_with("InfoPanelClass");
    return unitIcon || level;
}

} // namespace

bool exists(std::string_view className) {
    return engine::Application::instance().library().findClass(std::string(className)).has_value();
}

std::shared_ptr<engine::MovieClip> create(std::string_view className) {
    engine::Library& library = engine::Application::instance().library();
    const auto id = library.findClass(std::string(className));
    if (!id) throw std::runtime_error("no library symbol named " + std::string(className));
    auto clip = std::dynamic_pointer_cast<engine::MovieClip>(engine::createDisplayObject(library, *id));
    if (!clip) throw std::runtime_error(std::string(className) + " is not a sprite symbol");
    if (isSpriteClass(className)) clip->stop();
    return clip;
}

bool isInstance(const engine::DisplayObject& object, std::string_view className) {
    const auto* clip = dynamic_cast<const engine::MovieClip*>(&object);
    if (clip == nullptr) return false;
    const auto id = engine::Application::instance().library().findClass(std::string(className));
    return id && *id == clip->spriteId();
}

void setHtmlText(const engine::DisplayObject& parent, std::string_view path, const std::string& html) {
    if (auto* field = find<engine::TextField>(parent, path)) field->setHtmlText(html);
}

void registerSymbolScripts() {
    // (symbol id, 0-based frame) -> stop(), from the decompiled sources
    // (the symbol ids are the same in the original SWF and the asset pack).
    struct StopAt {
        std::uint16_t spriteId;
        int frameIndex;
    };
    const StopAt stops[] = {
        {5782, 105}, // IriySoftIntro
        {2883, 216}, // ArmorGamesIntro
        {1658, 148}, // ATenSplashScreenClass
        {1657, 148}, // PlagoFlameOnModel_fla/A10splashscreen_2
        {5151, 8},   // IriySoftIntro_fla/leafs_4
        {4438, 52},  // IriySoftIntro_fla/logo_5
        {3002, 0},   // gui_fla/progress_bar1_118
        {4682, 114}, // intro_fla/bandit2shadow_17
        {3834, 74},  // intro_fla/banditshadow_22
        {6584, 58},  // intro_fla/horseshadow_33
    };
    for (const StopAt& s : stops)
        engine::MovieClip::registerSymbolScript(s.spriteId, s.frameIndex, [](engine::MovieClip& clip) { clip.stop(); });
}

} // namespace takeover::symbols
