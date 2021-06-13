#pragma once

#include "kx/util.h"
#include "kx/time.h"

#include <cstdint>
#include <string>
#include <SDL2/SDL_events.h>

namespace kx { namespace sh {

struct KTermInputComm
{
    KTermInputComm(Passkey<class KTerm>) {};

    SDL_Event *input;
    KTerm *terminal;
    SDL_Keymod keymod;
    double *font_size;
    double min_font_size;
    double max_font_size;
    Time *last_keydown_time;
    int *scroll_line;
    int min_scroll_line;
    int max_scroll_line;
    int num_lines_drawn;
};

}}
