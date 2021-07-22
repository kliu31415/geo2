#pragma once

#include <optional>

namespace kx { namespace sfx {

class AudioTrack;

class SfxLibrary
{
    using channel_t = int;
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
public:
    AudioTrack(SfxLibrary *library_, unique_ptr_sdl<Mix_Chunk> mix_chunk_, Passkey<SfxLibrary>);

    AudioTrack(const AudioTrack&) = delete;
    AudioTrack& operator = (const AudioTrack&) = delete;

    AudioTrack(AudioTrack&&) = delete;
    AudioTrack& operator = (AudioTrack&&) = delete;

    void set_volume(float volume);
    void play(uint32_t num_times = 1);
};

}}
