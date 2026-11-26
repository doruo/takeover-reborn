// Sound.h - flash.media.Sound, SoundChannel and SoundTransform on miniaudio.
//
//     auto click = audio.sound(library.findClass("sndButtonClick"));
//     std::shared_ptr<SoundChannel> channel = click->play(0, 0, {0.5});
//     channel->addEventListener(Event::SOUND_COMPLETE, this, &Foo::onDone);
//
// Sounds are the SWF's MP3 data (assets/sounds/*.mp3), decoded to PCM the
// first time they are played. The Flash encoder delay ("seek samples") is
// skipped and the length is trimmed to the sample count the SWF declares,
// so loops are as seamless as in Flash.
//
// SOUND_COMPLETE is dispatched from AudioSystem::update(), called once per
// frame on the main thread, never from the audio thread.
#pragma once

#include "engine/events/EventDispatcher.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

struct ma_engine;

namespace engine {

    class AudioSystem;
    class Library;

    /// flash.media.SoundTransform.
    struct SoundTransform {
        double volume = 1; ///< 0..1
        double pan = 0;    ///< -1 (left) .. 1 (right)
    };

    /// Decoded PCM of one sound: interleaved stereo float at the device rate.
    struct SoundData {
        std::vector<float> frames;
        std::uint64_t frameCount = 0;
        std::uint32_t sampleRate = 44100;
    };

    class SoundChannel : public EventDispatcher, public std::enable_shared_from_this<SoundChannel> {
    public:
        ~SoundChannel() override;

        void stop();
        [[nodiscard]] const SoundTransform& soundTransform() const { return transform_; }
        void setSoundTransform(const SoundTransform& transform);

        /// Playback position in milliseconds within the current loop.
        [[nodiscard]] double position() const;

    private:
        friend class Sound;
        friend class AudioSystem;
        struct Voice; ///< miniaudio objects, hidden from the header

        SoundChannel(AudioSystem& audio, std::shared_ptr<const SoundData> data, double startMs, int loops,
                     const SoundTransform& transform);
        /// Called every frame: handles loop counts; true when playback ended.
        bool poll();

        AudioSystem& audio_;
        std::shared_ptr<const SoundData> data_;
        std::unique_ptr<Voice> voice_;
        SoundTransform transform_;
        int loopsLeft_ = 0;
        std::uint64_t startFrame_ = 0;
        bool stopped_ = false;
    };

    class Sound {
    public:
        /// flash.media.Sound.play: `loops` is the number of extra repetitions
        /// (0 = play once). Returns nullptr when audio is unavailable.
        std::shared_ptr<SoundChannel> play(double startTimeMs = 0, int loops = 0, const SoundTransform& transform = {});

        /// Length in milliseconds.
        [[nodiscard]] double length() const;

    private:
        friend class AudioSystem;
        Sound(AudioSystem& audio, std::uint16_t id) : audio_(audio), id_(id) {}

        AudioSystem& audio_;
        std::uint16_t id_;
    };

    /// Owns the audio device, the decoded sounds and the playing channels.
    class AudioSystem {
    public:
        explicit AudioSystem(Library& library);
        ~AudioSystem();
        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;

        /// The audio system of the running game (set by the constructor).
        static AudioSystem* instance() { return instance_; }

        [[nodiscard]] bool available() const { return engine_ != nullptr; }

        /// The sound of a library character, or nullptr if it isn't one.
        Sound* sound(std::uint16_t id);

        /// flash.media.SoundMixer.stopAll.
        void stopAll();

        /// Master volume (flash.media.SoundMixer.soundTransform).
        void setMasterVolume(double volume);

        /// Plays a sound placed on a timeline (StartSound tag).
        void playTimelineSound(std::uint16_t id, bool stopInstead, bool noMultiple, int loops);

        /// Once per frame: ends finished channels and dispatches SOUND_COMPLETE.
        void update();

    private:
        friend class Sound;
        friend class SoundChannel;

        std::shared_ptr<const SoundData> decode(std::uint16_t id);

        static AudioSystem* instance_;
        Library& library_;
        ma_engine* engine_ = nullptr;
        std::unordered_map<std::uint16_t, std::unique_ptr<Sound>> sounds_;
        std::unordered_map<std::uint16_t, std::shared_ptr<const SoundData>> decoded_;

        /// Playing channels, kept alive until they finish (Flash doesn't need a
        /// reference to keep a sound playing either).
        std::vector<std::shared_ptr<SoundChannel>> playing_;
        std::unordered_map<std::uint16_t, std::vector<std::weak_ptr<SoundChannel>>> timelineChannels_;
    };

} // namespace engine
