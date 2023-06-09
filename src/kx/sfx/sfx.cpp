#include "kx/sfx/sfx.h"
#include "kx/debug.h"

#include <SDL2/SDL_mixer.h>

#include <atomic>
#include <memory>

namespace kx { namespace sfx {

std::atomic<int> init_count(0);
SfxLibrary::SfxLibrary()
{
    //SfxLibrary should only be created once
    k_assert(init_count.fetch_add(1) == 0);

    if(Mix_Init(MIX_INIT_MP3 | MIX_INIT_OPUS | MIX_INIT_FLAC | MIX_INIT_OGG) != 0) {
        log_error((std::string)"Mix_GetError(): " + Mix_GetError());
    }
    if(Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 512)) {
        log_error((std::string)"Mix_GetError(): " + Mix_GetError());
    }
    Mix_Volume(-1, MIX_MAX_VOLUME);
    Mix_AllocateChannels(NUM_CHANNELS);
}
SfxLibrary::~SfxLibrary()
{
    Mix_CloseAudio();
    Mix_Quit();
}
void SfxLibrary::set_global_volume(float volume)
{
    Mix_Volume(-1, volume * MIX_MAX_VOLUME);
}
std::unique_ptr<AudioTrack> SfxLibrary::load_audio_from_file(std::string_view file_name)
{
    unique_ptr_sdl<Mix_Chunk> chunk = Mix_LoadWAV(file_name.data());
    if(chunk == nullptr)
        return nullptr;
    return std::unique_ptr<AudioTrack>(new AudioTrack(this, std::move(chunk), {}));
}
constexpr float PANNING_MULT = 254;
void SfxLibrary::play(AudioTrack *track, uint32_t num_times, Passkey<AudioTrack>)
{
    if(num_times != 0) {
        auto channel = Mix_PlayChannel(-1, track->mix_chunk.get(), num_times - 1);
        if(channel == -1) {
            log_error("Mix_PlayChannel failed: ", Mix_GetError());
        } else {
            auto lpan = track->left_panning * PANNING_MULT;
            auto rpan = track->right_panning * PANNING_MULT;
            if(Mix_SetPanning(channel, lpan, rpan) == 0) {
                log_error("Mix_SetPanning failed: ", Mix_GetError());
            }
        }
        track->playing_on_channel = channel;
    }
}

AudioTrack::AudioTrack(SfxLibrary *library_, unique_ptr_sdl<Mix_Chunk> mix_chunk_, Passkey<SfxLibrary>):
    library(library_),
    mix_chunk(std::move(mix_chunk_)),
    playing_on_channel(-1),
    left_panning(1.0),
    right_panning(1.0)
{
    Mix_VolumeChunk(mix_chunk.get(), MIX_MAX_VOLUME);
}
void AudioTrack::set_volume(float volume)
{
    Mix_VolumeChunk(mix_chunk.get(), volume * MIX_MAX_VOLUME);
}
void AudioTrack::set_panning(float left_panning_, float right_panning_)
{
    left_panning = left_panning_;
    right_panning = right_panning_;

    k_expects(left_panning>=0 && left_panning<=1);
    k_expects(right_panning>=0 && right_panning<=1);

    if(playing_on_channel != -1) {
        auto lpan = left_panning * PANNING_MULT;
        auto rpan = right_panning * PANNING_MULT;
        if(Mix_SetPanning(playing_on_channel, lpan, rpan) == 0) {
            log_error("Mix_SetPanning failed: ", Mix_GetError());
        }
    }
}
void AudioTrack::play(uint32_t num_times)
{
    library->play(this, num_times, {});
}

}}
