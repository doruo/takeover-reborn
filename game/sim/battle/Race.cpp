#include "game/sim/battle/Race.h"

namespace takeover {

std::string raceTag(Race race) {
    switch (race) {
    case Race::TheEmpire: return "TheEmpire";
    case Race::Westaria: return "Westaria";
    case Race::TheKhaganate: return "TheKhaganate";
    case Race::TheCult: return "TheCult";
    case Race::Bandits: return "Bandits";
    }
    return "";
}

namespace unit_types {

UnitType fromString(const std::string& name, bool* ok) {
    if (ok) *ok = true;
    if (name == "Infantry") return UnitType::Infantry;
    if (name == "Archers") return UnitType::Archers;
    if (name == "Cavalry") return UnitType::Cavalry;
    if (name == "Siege") return UnitType::Siege;
    if (name == "Magic") return UnitType::Magic;
    if (ok) *ok = false; // the original returns null
    return UnitType::Infantry;
}

std::string toString(UnitType type) {
    switch (type) {
    case UnitType::Infantry: return "Infantry";
    case UnitType::Archers: return "Archers";
    case UnitType::Cavalry: return "Cavalry";
    case UnitType::Siege: return "Siege";
    case UnitType::Magic: return "Magic";
    }
    return "";
}

UnitType fromInt(int value) {
    switch (value) {
    case 0: return UnitType::Infantry;
    case 1: return UnitType::Archers;
    case 2: return UnitType::Cavalry;
    case 3: return UnitType::Siege;
    default: return UnitType::Magic;
    }
}

} // namespace unit_types

} // namespace takeover
