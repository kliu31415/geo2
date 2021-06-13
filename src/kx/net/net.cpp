#include "kx/log.h"
#include "kx/net/kcurl.h"
#include "kx/util.h"

extern "C" {

int SDLNet_Init();
void SDLNet_Quit();
const char *SDLNet_GetError();

}

namespace kx { namespace net {

class InitPkey
{
    InitPkey() {}
    friend int init();
};
class QuitPkey
{
    QuitPkey() {};
    friend void quit();
};

int init()
{
    if(SDLNet_Init() < 0) {
        log_error("SDLNet_Init error: " + to_str(SDLNet_GetError()));
        return -1;
    }
    KCurl::init({});

    return 0;
}
void quit()
{
    SDLNet_Quit();
    KCurl::quit({});
}
}}
