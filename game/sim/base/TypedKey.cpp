#include "game/sim/base/TypedKey.h"

namespace takeover {

    namespace {
        std::map<std::string, int>& counters() {
            static std::map<std::string, int> table;
            return table;
        }
    } // namespace

    TypedKey::TypedKey(std::string kind) : kind_(std::move(kind)) {
        int& next = counters()[kind_];
        index_ = next;
        ++next;
    }

    void TypedKey::resetCounter(const std::string& kind) { counters()[kind] = 0; }

} // namespace takeover
