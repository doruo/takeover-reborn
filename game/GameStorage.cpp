#include "game/GameStorage.h"

#include "game/framework/storage/SaveFile.h"

#include <cstdio>

namespace takeover {

void GameStorage::init(const std::string& storageName) {
    storageName_ = storageName.empty() ? "TOVER" : storageName;
    current_ = 0;
    signalReady.dispatch();
}

void GameStorage::load() {
    profiles_.clear();
    
    for (int i = 0; i < PROFILES_COUNT; ++i)
        profiles_.push_back(std::make_unique<Profile>());

    SaveFile file(storageName_);
    
    if (!file.isComplete()) return;
    
    std::optional<ByteArray> data = file.loadData();
    if (!data) return;

    try {
        for (auto& profile : profiles_) 
            profile->deserialize(*data);
    } 
    catch (const std::exception& e) {
        // The original would stop with an uncaught error; start fresh instead.
        std::fprintf(
            stderr, "save file %s is damaged (%s), starting with empty profiles\n", 
            file.path().c_str(),
            e.what()
        );

        for (auto& profile : profiles_) 
            profile = std::make_unique<Profile>();
    }
}

void GameStorage::save() {
    
    ByteArray data;
    for (const auto& profile : profiles_) 
        profile->serialize(data);

    SaveFile file(storageName_);
    if (!file.saveData(std::move(data), true)) 
        std::fprintf(stderr, "cannot write %s\n", file.path().c_str());
}

void GameStorage::createProfile(int id) {
    profiles_[static_cast<std::size_t>(id)]->reset();
    profiles_[static_cast<std::size_t>(id)]->setEnabled(true);
}

void GameStorage::copyProfile(int source, int target) {
    ByteArray data;
    profiles_[static_cast<std::size_t>(source)]->serialize(data);
    data.setPosition(0);
    
    profiles_[static_cast<std::size_t>(target)] = std::make_unique<Profile>();
    profiles_[static_cast<std::size_t>(target)]->deserialize(data);
}

} // namespace takeover
