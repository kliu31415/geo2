#include "geo2/test.h"
#include "geo2/master_instance.h"

#include "kx/gfx/gfx.h"
#include "kx/time.h"

#ifndef SDL_main
#undef main
#endif

int main()
{
    using namespace kx;

    gfx::init();

    std::ios::sync_with_stdio(false); //if we're not using scanf/printf and not using multithreaded
                                      //non-mutexed printing, then we can disable sync

    //When moving timezones, change the value here.
    //When DST occurs, change the value here and also change Time::ET_offset.
    //The next line makes now() return seconds since epoch in our local timezone
    Time::set_now_utc_offset(Time::Delta(-5, Time::Length::hour));

    geo2::run();

    gfx::quit();
    return 0;
}
