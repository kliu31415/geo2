#pragma once

#include <memory>

/** I'm declaring the SDL stuff I need instead of including SDL headers in an
 *  attempt to reduce compile times.
 *  I think this works, although maybe the linker could bug out because
 *  doing this is kind of weird and ISO allows lots of undefined behavior
 */
extern "C" {

/** most of this is copied from SDL2's headers
 *  this might break if SDL2's headers change, but that should be OK because it should
 *  cause a compile time error rather than a silent runtime error
 */
#ifndef SDLCALL
#if (defined(__WIN32__) || defined(__WINRT__)) && !defined(__GNUC__)
#define SDLCALL __cdecl
#elif defined(__OS2__) || defined(__EMX__)
#define SDLCALL _System
# if defined (__GNUC__) && !defined(_System)
#  define _System /* for old EMX/GCC compat.  */
# endif
#else
#define SDLCALL
#endif
#endif

#ifndef DECLSPEC
# if defined(__WIN32__) || defined(__WINRT__)
#  ifdef __BORLANDC__
#   ifdef BUILD_SDL
#    define DECLSPEC
#   else
#    define DECLSPEC    __declspec(dllimport)
#   endif
#  else
#   define DECLSPEC __declspec(dllexport)
#  endif
# elif defined(__OS2__)
#   ifdef BUILD_SDL
#    define DECLSPEC    __declspec(dllexport)
#   else
#    define DECLSPEC
#   endif
# else
#  if defined(__GNUC__) && __GNUC__ >= 4
#   define DECLSPEC __attribute__ ((visibility("default")))
#  else
#   define DECLSPEC
#  endif
# endif
#endif

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Surface;
DECLSPEC SDLCALL void SDL_DestroyWindow(SDL_Window*);
DECLSPEC SDLCALL void SDL_DestroyRenderer(SDL_Renderer*);
DECLSPEC SDLCALL void SDL_DestroyTexture(SDL_Texture*);
DECLSPEC SDLCALL void SDL_FreeSurface(SDL_Surface*);

struct Mix_Chunk;
DECLSPEC SDLCALL void Mix_FreeChunk(Mix_Chunk*);

struct _TTF_Font;
DECLSPEC SDLCALL void TTF_CloseFont(_TTF_Font*);

struct _SDLNet_SocketSet;
struct _TCPsocket;
DECLSPEC SDLCALL void SDLNet_FreeSocketSet(_SDLNet_SocketSet*);
DECLSPEC SDLCALL void SDLNet_TCP_Close(_TCPsocket*);

#undef SDLCALL
#undef DECLSPEC
}

namespace kx {

///note: there are some static_asserts in sdl_deleter.cpp about assumptions we make

/** TODO: Do SDLNet cleanup functions work properly after SDLNet_Quit is called?
 *  We might be passing in a bunch of dangling pointers to SDLNet if SDLNet_Quit
 *  cleans up all data structures. So far this isn't crashing though.
 */

///allows smart pointers to be used with SDL2 data structures
struct SDL_Deleter final
{
    void operator()(SDL_Window *t) const {SDL_DestroyWindow(t);}
    void operator()(SDL_Renderer *t) const {SDL_DestroyRenderer(t);}
    void operator()(SDL_Texture *t) const {SDL_DestroyTexture(t);}
    void operator()(SDL_Surface *t) const {SDL_FreeSurface(t);}

    void operator()(_TTF_Font *t) const {TTF_CloseFont(t);}

    void operator()(Mix_Chunk *t) const {Mix_FreeChunk(t);}

    void operator()(_SDLNet_SocketSet *socket_set) const {SDLNet_FreeSocketSet(socket_set);}
    void operator()(_TCPsocket *tcp_socket) const {SDLNet_TCP_Close(tcp_socket);}
};
template<class T> struct unique_ptr_sdl final: public std::unique_ptr<T, SDL_Deleter>
{
    unique_ptr_sdl(T *obj): std::unique_ptr<T, SDL_Deleter>(obj, SDL_Deleter()) {}
    unique_ptr_sdl(): std::unique_ptr<T, SDL_Deleter>(nullptr) {}
};
template<class T> struct shared_ptr_sdl final: public std::shared_ptr<T>
{
    ///I don't think the move constructor leaks memory, but it might if I'm misunderstanding things
    shared_ptr_sdl(unique_ptr_sdl<T> &&ptr): std::shared_ptr<T>(std::move(ptr)) {}
    shared_ptr_sdl(T *obj): std::shared_ptr<T>(obj, SDL_Deleter()) {}
    shared_ptr_sdl(): std::shared_ptr<T>(nullptr) {}
};

}
