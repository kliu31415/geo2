#pragma once

#include <optional>

namespace kx { namespace sfx {

class AudioTrack;

using channel_t = int;

class SfxLibrary
{
    static constexpr channel_t NUM_CHANNELS = 256;
public:
    SfxLibrary();
    ~SfxLibrary();

    SfxLibrary(const SfxLibrary&) = delete;
    SfxLibrary& operator = (const SfxLibrary&) = delete;

    SfxLibrary(SfxLibrary&&) = delete;
    SfxLibrary& operator = (SfxLibrary&&) = delete;

    void set_global_volume(float volume);
    std::unique_ptr<AudioTrack> load_audio_from_file(std::string_view file_name);

    void play(AudioTrack *track, uint32_t num_times, Passkey<AudioTrack>);
};

class AudioTrack
{
    friend SfxLibrary;

    SfxLibrary *library;
    unique_ptr_sdl<Mix_Chunk> mix_chunk;
    channel_t playing_on_channel;
    float left_panning;
    float right_panning;
public:
    AudioTrack(SfxLibrary *library_, unique_ptr_sdl<Mix_Chunk> mix_chunk_, Passkey<SfxLibrary>);

    AudioTrack(const AudioTrack&) = delete;
    AudioTrack& operator = (const AudioTrack&) = delete;

    AudioTrack(AudioTrack&&) = delete;
    AudioTrack& operator = (AudioTrack&&) = delete;

    void set_volume(float volume);
    void set_panning(float left_panning_, float right_panning_);
    void play(uint32_t num_times = 1);
};

}}
