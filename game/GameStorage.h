// GameStorage.h - the three profile slots and their save file (original:
// GameStorage). The Armor Games online saves ("AGI") are not ported.
#pragma once

#include "game/framework/core/Signal.h"
#include "game/progress/Profile.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

    class GameStorage {
    public:
        static constexpr int PROFILES_COUNT = 3;

        /// Dispatched once the storage is ready (original: _SafeStr_766).
        Signal<> signalReady;

        void init(const std::string& storageName = "TOVER"); ///< _SafeStr_771
        void load();
        void save();

        [[nodiscard]] Profile& currentProfile() { return *profiles_[static_cast<std::size_t>(current_)]; } ///< _SafeStr_748
        [[nodiscard]] Profile& profile(int id) { return *profiles_[static_cast<std::size_t>(id)]; }        ///< _SafeStr_3585
        void selectProfile(int id) { current_ = id; }                                                   ///< _SafeStr_3531
        void createProfile(int id);                                                                     ///< _SafeStr_3532
        void eraseProfile(int id) { profiles_[static_cast<std::size_t>(id)]->reset(); }                 ///< _SafeStr_3535
        void copyProfile(int source, int target);                                                       ///< _SafeStr_3536
        [[nodiscard]] bool isOnline() const { return false; }                                           ///< _SafeStr_3520

    private:
        std::string storageName_ = "TOVER";
        std::vector<std::unique_ptr<Profile>> profiles_;
        int current_ = 0;
    };

} // namespace takeover
