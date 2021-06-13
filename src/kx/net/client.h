#pragma once

#include "kx/sdl_deleter.h"

#include <string>
#include <cstdint>
#include <memory>

///idk a ton about network programming so this very well might be buggy/leak memory
namespace kx { namespace net {

/** connect(), connect_async(), disconnect(), and get_status() are thread safe in the
 *  sense that even if all of them are simultaneously called multiple times (across
 *  multiple threads), it'll work (although it could take quite a long time because
 *  connect() and connect_async() can take up to a few seconds per call).
 *
 *  This has data races when using multiple threads due to SDLNet writing to a shared
 *  SDL2 error string, but that probably won't be a major issue.
 *
 *  connect_async() is not well tested, so it might be buggy.
 *
 *  If the network connection suddenly disconnects, then there'll be an error logged
 *  if verbose=true (and maybe also even if verbose=false depending on how I decide
 *  to implement this). Afterwards, the Status will be Disconnected, all send calls
 *  will fail and return -1, all recv calls will fail and return "", and recv_ready
 *  will return false. I believe you can reconnect if desired.
 */
class Client final
{
    struct Impl;
    ///const to prevent bugs
    const std::unique_ptr<Impl> impl;
public:
    enum class Status{Disconnected, Connecting, Connected};

    static void set_verbose(bool verbose_); ///thread-safe

    Client();
    ~Client();

    ///you can't copy network connections
    Client(const Client&) = delete;
    Client &operator = (const Client&) = delete;

    ///moving isn't implemented yet (we'll need to add many impl==nullptr checks if it's movable)
    Client(Client&&) = delete;
    Client &operator = (Client&&) = delete;

    int connect(const std::string &domain, uint16_t port);

    /** This is thread-safe. Also, you don't have to check if a connection is active or not
     *  before calling this (it performs checks for you).
     */
    void disconnect();

    Status get_status();
    bool recv_ready(); ///not thread safe

    ///Precondition: data.size()<=INT_MAX
    int send(const std::string &data); ///not thread safe. Returns 0 on success.

    std::string recv(); ///not thread safe
};

}}
