#include "engine/audio/Sound.h"

#include "engine/assets/Library.h"

#include <miniaudio/miniaudio.h>

#include <algorithm>
#include <cstdio>

namespace engine {

    namespace {

        constexpr std::uint32_t kChannels = 2;

    } // namespace

    AudioSystem* AudioSystem::instance_ = nullptr;

    // /----- SOUND CHANNEL -----/

    struct SoundChannel::Voice {
        ma_audio_buffer_ref buffer{};
        ma_sound sound{};
        bool initialized = false;
    };

    SoundChannel::SoundChannel(AudioSystem& audio, std::shared_ptr<const SoundData> data, double startMs, int loops,
                               const SoundTransform& transform)
        : audio_(audio), data_(std::move(data)), voice_(std::make_unique<Voice>()), transform_(transform),
          loopsLeft_(loops) {
        startFrame_ = std::min<std::uint64_t>(static_cast<std::uint64_t>(startMs / 1000.0 * data_->sampleRate),
                                              data_->frameCount);
        if (ma_audio_buffer_ref_init(ma_format_f32, kChannels, data_->frames.data(), data_->frameCount, &voice_->buffer) !=
            MA_SUCCESS)
            return;
        if (ma_sound_init_from_data_source(audio_.engine_, &voice_->buffer, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr,
                                           &voice_->sound) != MA_SUCCESS)
            return;
        voice_->initialized = true;
        ma_data_source_seek_to_pcm_frame(&voice_->buffer, startFrame_);
        setSoundTransform(transform);
        ma_sound_start(&voice_->sound);
    }

    SoundChannel::~SoundChannel() {
        if (voice_ && voice_->initialized) {
            ma_sound_uninit(&voice_->sound);
            ma_audio_buffer_ref_uninit(&voice_->buffer);
        }
    }

    void SoundChannel::stop() {
        if (stopped_)
            return;

        stopped_ = true;

        if (voice_->initialized)
            ma_sound_stop(&voice_->sound);
    }

    void SoundChannel::setSoundTransform(const SoundTransform& transform) {
        transform_ = transform;

        if (!voice_->initialized)
            return;

        ma_sound_set_volume(&voice_->sound, static_cast<float>(std::clamp(transform.volume, 0.0, 1.0)));
        ma_sound_set_pan(&voice_->sound, static_cast<float>(std::clamp(transform.pan, -1.0, 1.0)));
    }

    double SoundChannel::position() const {
        if (!voice_->initialized)
            return 0;

        ma_uint64 cursor = 0;
        ma_data_source_get_cursor_in_pcm_frames(const_cast<ma_audio_buffer_ref*>(&voice_->buffer), &cursor);

        return static_cast<double>(cursor) * 1000.0 / data_->sampleRate;
    }

    bool SoundChannel::poll() {
        if (stopped_ || !voice_->initialized)
            return true;

        if (!ma_sound_at_end(&voice_->sound))
            return false;

        if (loopsLeft_ > 0) {
            // Flash repeats from the start time on every loop.
            --loopsLeft_;
            ma_sound_stop(&voice_->sound);
            ma_data_source_seek_to_pcm_frame(&voice_->buffer, startFrame_);
            ma_sound_seek_to_pcm_frame(&voice_->sound, startFrame_);
            ma_sound_start(&voice_->sound);

            return false;
        }

        return true;
    }

    // /----- SOUND -----/

    std::shared_ptr<SoundChannel> Sound::play(double startTimeMs, int loops, const SoundTransform& transform) {
        if (!audio_.available())
            return nullptr;

        std::shared_ptr<const SoundData> data = audio_.decode(id_);
        if (!data || data->frameCount == 0)
            return nullptr;

        std::shared_ptr<SoundChannel> channel(new SoundChannel(audio_, std::move(data), startTimeMs, loops, transform));
        if (!channel->voice_->initialized)
            return nullptr;

        audio_.playing_.push_back(channel);
        return channel;
    }

    double Sound::length() const {
        const SoundInfo* info = audio_.library_.sound(id_);
        return info ? info->sampleCount * 1000.0 / info->sampleRate : 0;
    }

    // /----- AUDIO SYSTEM -----/

    AudioSystem::AudioSystem(Library& library) : library_(library) {
        instance_ = this;
        auto* engine = new ma_engine;
        ma_engine_config config = ma_engine_config_init();
        config.channels = kChannels;
        config.sampleRate = 44100;

        if (ma_engine_init(&config, engine) != MA_SUCCESS) {
            std::fprintf(stderr, "audio: no output device, the game runs silent\n");
            delete engine;

            return;
        }

        engine_ = engine;
    }

    AudioSystem::~AudioSystem() {
        playing_.clear();
        timelineChannels_.clear();

        if (engine_) {
            ma_engine_uninit(engine_);
            delete engine_;
        }

        if (instance_ == this)
            instance_ = nullptr;
    }

    Sound* AudioSystem::sound(std::uint16_t id) {
        auto& slot = sounds_[id];
        if (!slot) {
            if (!library_.sound(id))
                return nullptr;

            slot.reset(new Sound(*this, id));
        }

        return slot.get();
    }

    std::shared_ptr<const SoundData> AudioSystem::decode(std::uint16_t id) {
        if (auto it = decoded_.find(id); it != decoded_.end())
            return it->second;

        const SoundInfo* info = library_.sound(id);
        if (!info)
            return nullptr;

        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, kChannels, 44100);
        ma_decoder decoder;
        if (ma_decoder_init_file(info->file.string().c_str(), &config, &decoder) != MA_SUCCESS) {
            std::fprintf(stderr, "audio: cannot decode %s\n", info->file.string().c_str());
            decoded_[id] = nullptr;

            return nullptr;
        }

        auto data = std::make_shared<SoundData>();
        std::vector<float> chunk(4096 * kChannels);

        for (;;) {
            ma_uint64 read = 0;
            ma_decoder_read_pcm_frames(&decoder, chunk.data(), 4096, &read);

            if (read == 0)
                break;

            data->frames.insert(data->frames.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(read * kChannels));
        }

        ma_decoder_uninit(&decoder);

        // Skip the encoder delay declared by the SWF, trim to its sample count
        // (both are in the sound's own rate, we decode at 44.1 kHz).
        const double rateRatio = 44100.0 / info->sampleRate;
        const auto skip = static_cast<std::size_t>(std::max(0, info->seekSamples) * rateRatio) * kChannels;
        if (skip < data->frames.size())
            data->frames.erase(data->frames.begin(), data->frames.begin() + static_cast<std::ptrdiff_t>(skip));

        if (info->sampleCount > 0) {
            const auto keep = static_cast<std::size_t>(info->sampleCount * rateRatio) * kChannels;
            if (keep < data->frames.size())
                data->frames.resize(keep);
        }

        data->frameCount = data->frames.size() / kChannels;
        decoded_[id] = data;

        return data;
    }

    void AudioSystem::stopAll() {
        for (auto& channel : playing_)
            channel->stop();
    }

    void AudioSystem::setMasterVolume(double volume) {
        if (engine_)
            ma_engine_set_volume(engine_, static_cast<float>(std::clamp(volume, 0.0, 1.0)));
    }

    void AudioSystem::playTimelineSound(std::uint16_t id, bool stopInstead, bool noMultiple, int loops) {
        auto& channels = timelineChannels_[id];
        std::erase_if(channels, [](const auto& weak) { return weak.expired(); });

        if (stopInstead) {
            for (auto& weak : channels)
                if (auto channel = weak.lock())
                    channel->stop();
            channels.clear();
            return;
        }

        if (noMultiple && !channels.empty())
            return;

        if (Sound* s = sound(id))
            if (auto channel = s->play(0, std::max(0, loops - 1)))
                channels.push_back(channel);
    }

    void AudioSystem::update() {
        // Collect first: SOUND_COMPLETE listeners may start new sounds.
        std::vector<std::shared_ptr<SoundChannel>> finished;

        for (auto& channel : playing_)
            if (channel->poll())
                finished.push_back(channel);
        std::erase_if(playing_, [&](const auto& c) { return std::find(finished.begin(), finished.end(), c) != finished.end(); });

        for (auto& channel : finished) {
            if (channel->stopped_)
                continue; // stop() never sends SOUND_COMPLETE

            Event complete(Event::SOUND_COMPLETE);
            channel->dispatchEvent(complete);
        }
    }

} // namespace engine
