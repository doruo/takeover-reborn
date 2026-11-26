// XMLData.h - the list of clips the battle animates (original: XMLData).
//
// The original described them as XML snippets: <movie_clip libName=".."/>.
// For each special unit kind it lists the 4 directions x N phases of its
// animations ("RedArchers2Down_Attack"...) and gives the ArmyStat the
// naming rule; then come buildings, spell effects, cursors and missiles.
#pragma once

#include "game/framework/raster/ResourceManager.h"

#include <string>
#include <vector>

namespace takeover {

    class ArmyStat;
    enum class Race;
    enum class UnitType;

    class XMLData {
    public:
        /// The single instance, created on first use (original: _SafeStr_50).
        static XMLData& instance();

        /// "Red", "Green", "Blue", "Purple" or "Gray" (original: _SafeStr_1305).
        static std::string raceColor(Race race);

        /// Unit names of the older art set (original: _SafeStr_1308).
        static std::string unitTypeName(UnitType type);

        std::vector<ClipDef> battleClips;  ///< Original: _SafeStr_1644.
        std::vector<ClipDef> preloadClips; ///< Clips rendered at startup (original: _SafeStr_1646).

    private:
        XMLData();

        /// Lists the animations of a special unit and gives it its naming rule
        /// (original: _SafeStr_2952).
        void addSpecialUnit(ArmyStat& stat, int animCount, double scale = 1);
        void addAllUnits(); ///< _SafeStr_2955
    };

} // namespace takeover
