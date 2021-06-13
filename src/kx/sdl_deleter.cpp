#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <type_traits>

static_assert(std::is_same<_SDLNet_SocketSet*, SDLNet_SocketSet>::value);
static_assert(std::is_same<_TCPsocket*, TCPsocket>::value);

static_assert(std::is_same<_TTF_Font, TTF_Font>::value);

namespace kx {

}
