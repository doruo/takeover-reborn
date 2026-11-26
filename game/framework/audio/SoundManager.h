// SoundManager.h - plays library sounds by class name, with one volume per
// sound and one music track at a time (original: _SafePkg_1092).
#pragma once

#include "engine/audio/Sound.h"
#include "game/framework/core/Disposable.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace takeover {

/// All the channels of one library sound, sharing a volume (original:
/// _SafePkg_1092/SoundObject).
class SoundObject {
public:
    explicit SoundObject(engine::Sound* sound) : sound_(sound) {}

    [[nodiscard]] int playingCount() const { return static_cast<int>(channels_.size()); }
    [[nodiscard]] double volume() const { return transform_.volume; }

    std::shared_ptr<engine::SoundChannel> play(double startTime = 0, int loops = 0);
    void setVolume(double volume);                        ///< Original: _SafeStr_1089.
    void setTransform(const engine::SoundTransform& t);   ///< Original: _SafeStr_2516.
    void stop();

private:
    void onSoundComplete(engine::Event& event);

    engine::Sound* sound_;
    engine::SoundTransform transform_;
    std::set<std::shared_ptr<engine::SoundChannel>> channels_;
};

class SoundManager : public Disposable {
public:
    SoundManager() = default;

    /// Volume applied on top of each sound's own (original: _SafeStr_1087).
    [[nodiscard]] double masterVolume() const { return masterVolume_; }
    void setMasterVolume(double value);

    [[nodiscard]] bool enabled() const { return enabled_; }
    void setEnabled(bool value);

    void setVolume(const std::string& soundClass, double volume);                         ///< _SafeStr_1089
    void setTransform(const std::string& soundClass, const engine::SoundTransform& t);    ///< _SafeStr_2516
    std::shared_ptr<engine::SoundChannel> play(const std::string& soundClass, double startTime = 0,
                                               int loops = 0);                            ///< _SafeStr_1926
    /// Starts a looping music, stopping the previous one (original: _SafeStr_1088).
    std::shared_ptr<engine::SoundChannel> playMusic(const std::string& soundClass);
    void stopMusic();  ///< Original: _SafeStr_745.
    void stopSounds(); ///< Every sound but the music (original: _SafeStr_2515).

protected:
    void customDispose() override;

private:
    SoundObject* objectFor(const std::string& soundClass); ///< Original: _SafeStr_2513.

    std::map<std::string, std::unique_ptr<SoundObject>> sounds_;
    std::shared_ptr<engine::SoundChannel> musicChannel_;
    SoundObject* music_ = nullptr;
    double masterVolume_ = 1;
    bool enabled_ = true;
};

} // namespace takeover
