#pragma once

/** TODO: SDL_net isn't thread-safe. In particular, ThreadSanitizer detects a data race in
 *  SDLNet_SetError(), because if multiple connections fail simultaneously, then each of
 *  them will write to SDL_net's internal error string simultaneously. This shouldn't be
 *  that big of a deal in practice, because connecting usually happens at the beginning of
 *  the program, so if we get a fatal error we fail fast. In additional, we only get a
 *  data race if connecting fails, in which case we don't care (unless we try to reconnect
 *  in the same instance of the program).
 */
namespace kx { namespace net {

int init();
/** Our SDL_Deleter will attempt to delete static-storage SDL_Net data structures on
 *  program exit (i.e. after SDLNet_Quit() is called in quit()), but the deleting functions
 *  don't crash, even though freeing a TTF_Font after TTF_Quit() is called crashes. This is
 *  a weird discrepancy (some SDL freeing functions crash if they're used after a quit
 *  function is called, and others don't). I don't know if this is intentional or not, (i.e.
 *  maybe you're not supposed to use SDLNet_Free...(...) after SDLNet_Quit(); it just happens
 *  that it works).
 *
 *  This makes lifetime management a lot easier. As long as the main thread is the only
 *  running thread at the end of the main() function, then quit() should work without crashing.
 *
 *  If a connection attempt is in progress (i.e. an async connection attempt) when quit()
 *  is called, this could lead to crashes. This can only be possible if there's another
 *  running thread when the main() function ends.
 */
void quit();

enum class Result {Success, Failure};

}}
