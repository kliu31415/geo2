#pragma once

#include <cstdint>
#include <memory>

namespace kx { namespace net {

///This is a WIP. It's not functional.
class Server
{
    struct Impl;
    ///const to prevent bugs
    const std::unique_ptr<Impl> impl;
public:
    Server();
    ~Server();

    ///you can't copy network connections
    Server(const Server&) = delete;
    Server &operator = (const Server&) = delete;

    ///moving isn't implemented yet (we'll need to add many impl==nullptr checks if it's movable)
    Server(Server&&) = delete;
    Server &operator = (Server&&) = delete;

    void start(uint16_t port);
    void stop();
    void check_for_new_connections();
    void close_dead_connections();
};

}}
