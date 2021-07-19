#include "geo2/test.h"
#include "geo2/master_instance.h"

#include "kx/gfx/gfx.h"
#include "kx/time.h"

#ifndef SDL_main
#undef main
#endif

static_assert(sizeof(int) == 4);

int main()
{
    using namespace kx;

    gfx::init();


    //don't use cout; use io::print instead (which uses cout internally)
    std::ios::sync_with_stdio(false);

    //When moving timezones, change the value here.
    //When DST occurs, change the value here and also change Time::ET_offset.
    //The next line makes now() return seconds since epoch in our local timezone
    Time::set_now_utc_offset(Time::Delta(-5, Time::Length::hour));

    geo2::run();

    gfx::quit();
    return 0;
}
