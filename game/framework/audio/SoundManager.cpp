#include "game/framework/audio/SoundManager.h"

#include "engine/platform/Application.h"

#include <climits>

namespace takeover {

// --- SoundObject ------------------------------------------------------------------

std::shared_ptr<engine::SoundChannel> SoundObject::play(double startTime, int loops) {
    if (sound_ == nullptr) return nullptr;
    auto channel = sound_->play(startTime, loops, transform_);
    if (!channel) return nullptr;
    channel->addEventListener(engine::Event::SOUND_COMPLETE, this, &SoundObject::onSoundComplete);
    channels_.insert(channel);
    return channel;
}

void SoundObject::setVolume(double volume) {
    transform_.volume = volume;
    for (const auto& channel : channels_) {
        engine::SoundTransform t = channel->soundTransform();
        t.volume = volume;
        channel->setSoundTransform(t);
    }
}

void SoundObject::setTransform(const engine::SoundTransform& t) {
    transform_ = t;
    for (const auto& channel : channels_) channel->setSoundTransform(t);
}

void SoundObject::stop() {
    for (const auto& channel : channels_) channel->stop();
    // Flash keeps stopped channels in the dictionary (no SOUND_COMPLETE);
    // forgetting them here changes nothing observable.
    channels_.clear();
}

void SoundObject::onSoundComplete(engine::Event& event) {
    auto* channel = static_cast<engine::SoundChannel*>(event.target());
    channel->removeEventListener(engine::Event::SOUND_COMPLETE, this, &SoundObject::onSoundComplete);
    std::erase_if(channels_, [&](const auto& c) { return c.get() == channel; });
}

// --- SoundManager -------------------------------------------------------------------

void SoundManager::setMasterVolume(double value) {
    for (auto& [name, sound] : sounds_) {
        const double own = masterVolume_ > 0 ? sound->volume() / masterVolume_ : sound->volume();
        sound->setVolume(value * own);
    }
    masterVolume_ = value;
}

void SoundManager::setEnabled(bool value) {
    enabled_ = value;
    if (!enabled_) {
        stopSounds();
        stopMusic();
    }
}

SoundObject* SoundManager::objectFor(const std::string& soundClass) {
    auto& slot = sounds_[soundClass];
    if (!slot) {
        engine::Application& app = engine::Application::instance();
        engine::Sound* sound = nullptr;
        if (const auto id = app.library().findClass(soundClass)) sound = app.audio().sound(*id);
        slot = std::make_unique<SoundObject>(sound);
        slot->setVolume(masterVolume_);
    }
    return slot.get();
}

void SoundManager::setVolume(const std::string& soundClass, double volume) {
    objectFor(soundClass)->setVolume(masterVolume_ * volume);
}

void SoundManager::setTransform(const std::string& soundClass, const engine::SoundTransform& t) {
    objectFor(soundClass)->setTransform(t);
}

std::shared_ptr<engine::SoundChannel> SoundManager::play(const std::string& soundClass, double startTime, int loops) {
    if (!enabled_) return nullptr;
    return objectFor(soundClass)->play(startTime, loops);
}

std::shared_ptr<engine::SoundChannel> SoundManager::playMusic(const std::string& soundClass) {
    if (!enabled_) return nullptr;
    SoundObject* music = objectFor(soundClass);
    if (music != music_) {
        if (music_ != nullptr) music_->stop();
        music_ = music;
        musicChannel_ = music_->play(0, INT_MAX);
    }
    return musicChannel_;
}

void SoundManager::stopMusic() {
    if (music_ == nullptr) return;
    musicChannel_.reset();
    music_->stop();
    music_ = nullptr;
}

void SoundManager::stopSounds() {
    for (auto& [name, sound] : sounds_)
        if (sound.get() != music_) sound->stop();
}

void SoundManager::customDispose() {
    stopSounds();
    stopMusic();
    sounds_.clear();
    Disposable::customDispose();
}

} // namespace takeover
