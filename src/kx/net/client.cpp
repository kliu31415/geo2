#include "kx/net/client.h"
#include "kx/util.h"
#include "kx/debug.h"
#include "kx/net/net.h"

#include <SDL2/SDL_net.h>
#include <string>
#include <cstring>
#include <cstdint>
#include <future>
#include <atomic>
#include <climits>

namespace kx { namespace net {

struct Client::Impl
{
    static constexpr int RECV_BUFFER_LEN = 25000;

    std::string buffer;
    std::atomic<bool> is_async_connecting;
    std::future<int> async_connect;
    std::mutex modify_mutex;
    void (*async_callback)();
    //note that even though it's a "socket set", it will only contain 1 socket (it's possible
    //to put more in but we can just create multiple socket sets and put one socket in each).
    unique_ptr_sdl<_SDLNet_SocketSet> socket_set;

    shared_ptr_sdl<_TCPsocket> tcp_socket; //makes get_status() thread safe
    IPaddress ip_address;

    Status get_status_already_locked_mutex()
    {
        //Status::connecting should NEVER happen because we locked modify_mutex
        return (tcp_socket!=nullptr? Status::Connected: Status::Disconnected);
    }

    Impl():
        is_async_connecting(false)
    {
        buffer.resize(RECV_BUFFER_LEN);
    }
};

static std::atomic<bool> verbose(true);
void Client::set_verbose(bool verbose_)
{
    verbose.store(verbose_, std::memory_order_relaxed);
}

Client::Client():
    impl(std::make_unique<Impl>())
{

}
Client::~Client()
{
    if(get_status()==Status::Connected)
        disconnect();
}
int Client::connect(const std::string &domain, uint16_t port)
{
    std::lock_guard<std::mutex> lg(impl->modify_mutex);

    Status status = impl->get_status_already_locked_mutex();

    if(status == Status::Connected) {
        if(verbose.load(std::memory_order_relaxed))
            log_error("Attempting to call Client.connect, but a connection is already active");
        return -1;
    }

    if(SDLNet_ResolveHost(&impl->ip_address, domain.c_str(), port)) {
        if(verbose.load(std::memory_order_relaxed))
            log_error("SDLNet_ResolveHost failed");
        return -2;
    }
    impl->tcp_socket = SDLNet_TCP_Open(&impl->ip_address);
    if(impl->tcp_socket == nullptr) {
        if(verbose.load(std::memory_order_relaxed))
            log_error("SDLNet_TCP_Open failed");
        return -3;
    }
    if(verbose.load(std::memory_order_relaxed))
        log_info("successfully connected to server");
    impl->socket_set = SDLNet_AllocSocketSet(1); //we will only put at most 1 socket into the socket set
    SDLNet_TCP_AddSocket(impl->socket_set.get(), impl->tcp_socket.get());
    return 0;
}
void Client::disconnect()
{
    std::lock_guard<std::mutex> lg(impl->modify_mutex);

    Status status = impl->get_status_already_locked_mutex();

    if(status == Status::Disconnected) {
        if(verbose.load(std::memory_order_relaxed))
            log_error("attempting to disconnect, but already disconnected");
        return;
    }

    k_expects(impl->socket_set != nullptr); //if tcp_socket exists, it must be in
                                            //socket_set, so socket_set can't be nullptr

    SDLNet_TCP_DelSocket(impl->socket_set.get(), impl->tcp_socket.get()); //is this necessary?
    impl->tcp_socket = nullptr; //necessary because get_status checks if this is nullptr
    impl->socket_set = nullptr;
}
Client::Status Client::get_status()
{
    if(impl->is_async_connecting.load(std::memory_order_relaxed)) {
        return Status::Connecting;
    }

    std::lock_guard<std::mutex> lg(impl->modify_mutex);

    if(impl->async_connect.valid()) {
        impl->async_connect.get();
        if(impl->async_callback != nullptr)
            impl->async_callback();
        return Status::Connected;
    } else if(impl->tcp_socket == nullptr) {
        return Status::Disconnected;
    } else
        return Status::Connected;
}
bool Client::recv_ready()
{
    //if we suddenly disconnect, then recv_ready returns true for some reason, so for safety,
    //make it explicitly return false if we aren't connected
    if(get_status() != Status::Connected)
        return false;

    return SDLNet_CheckSockets(impl->socket_set.get(), 0) > 0;
}
int Client::send(const std::string &data)
{
    if(get_status() != Status::Connected)
        return -1;

    //SDLNet_TCP_Send uses an int for length, but string uses a size_t. The chance that this
    //check fails is very small (you'd need a 2GB+ packet), which is why it's only enabled
    //in KX_DEBUG mode.
    k_expects(data.size() <= INT_MAX);

    if(data.size() > 0) {
        int len_sent = SDLNet_TCP_Send(impl->tcp_socket.get(), data.data(), data.size());
        if(len_sent < (int)data.size()) {
            log_error("SDLNet_TCP_Send failed (len_sent < 0). Closing connection.");
            disconnect();
            return -2;
        }
    }
    return 0;
}
std::string Client::recv()
{
    if(get_status() != Status::Connected)
        return "";

    //It appears that this enters an infinite loop with recv_len=0 if we're suddenly disconnected
    std::string data;
    do {
        int recv_len = SDLNet_TCP_Recv(impl->tcp_socket.get(), impl->buffer.data(), Impl::RECV_BUFFER_LEN);
        if(recv_len <= 0) { //for some reason recv_len is sometimes 0 even if SDLNet_SocketReady returns >0
            //if(verbose.load(std::memory_order_relaxed))
            log_error("SDLNet_TCP_Recv failed (recv_len <= 0). Closing connection.");
            disconnect();
            break;
        }
        data.append(impl->buffer.c_str(), recv_len);
    }
    while(SDLNet_CheckSockets(impl->socket_set.get(), 0) > 0); //we're the only socket in the set so we can do this

    return data;
}

}}
