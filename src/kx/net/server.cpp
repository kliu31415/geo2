#include "kx/net/server.h"
#include "kx/sdl_deleter.h"
#include "kx/debug.h"
#include "kx/log.h"
#include "kx/util.h"
#include "kx/net/net.h"

#include <SDL2/SDL_net.h>
#include <vector>
#include <string>

namespace kx { namespace net {

struct Server::Impl
{
    static constexpr size_t MAX_CONNECTIONS = 20;
    static constexpr size_t RECV_BUFFER_LEN = 10000;

    unique_ptr_sdl<_TCPsocket> server_socket;
    unique_ptr_sdl<_SDLNet_SocketSet> socket_set;
    std::vector<unique_ptr_sdl<_TCPsocket>> clients;
    IPaddress IP;
    std::string recv_buffer;
};

Server::Server():
    impl(std::make_unique<Impl>())
{
    impl->recv_buffer.resize(impl->RECV_BUFFER_LEN);
}
Server::~Server()
{
    stop();
}
void Server::start(uint16_t port)
{
    impl->socket_set = SDLNet_AllocSocketSet(Impl::MAX_CONNECTIONS + 1);
    if(SDLNet_ResolveHost(&impl->IP, nullptr, port))
        log_error("SDLNet_ResolveHost failed to create server");
    impl->server_socket = SDLNet_TCP_Open(&impl->IP);
    SDLNet_TCP_AddSocket(impl->socket_set.get(), impl->server_socket.get());
}
void Server::stop()
{
    //for safety, since I think SDL_net technically wants you to
    //remove all sockets from a socket set before deleting the
    //sockets themselves (this probably wouldn't actually make
    //a difference in practice though)
    SDLNet_TCP_DelSocket(impl->socket_set.get(), impl->server_socket.get());
    for(auto &client: impl->clients)
        SDLNet_TCP_DelSocket(impl->socket_set.get(), client.get());

    impl->server_socket = nullptr;
    impl->socket_set = nullptr;
    impl->clients.clear();
}
void Server::check_for_new_connections()
{
    while(true) {
        unique_ptr_sdl<_TCPsocket> new_client_socket = SDLNet_TCP_Accept(impl->server_socket.get());
        if(new_client_socket != nullptr) {
            int ret = SDLNet_TCP_AddSocket(impl->socket_set.get(), new_client_socket.get());

            //TODO: ret will return an error if socket_set is too small (i.e. we're trying
            //to put too many connections in it), right? We should make sure. Note that
            //the server socket takes one spot.
            if(impl->clients.size()+1 >= Impl::MAX_CONNECTIONS) {
                log_error("net::Server has a new client but is unable to properly accept it because "
                          "the socket set has no more free entries");
                return;
            }
            if(ret == -1)
                log_error("SDLNet_TCP_AddSocket failed while attempting to accept a new connection");
            else
                log_info("server accepted new connection");
            impl->clients.push_back(std::move(new_client_socket));
        } else
            break;
    }
}
void Server::close_dead_connections()
{
    /*if(SDLNet_CheckSockets(impl->socket_set, 0) > 0) {
        for(auto &client: impl->clients) {
            while(SDLNet_SocketReady(client.get())) {
                SDLNet_TCP_Recv(client.get(), impl->recv_buffer.data(), Impl::RECV_BUFFER_LEN);
            }
        }
        for(auto &i: clients)
            i.receive([&](string s)-> void {Server::log(s);}, process_data);
    }
    for(auto i = clients.begin(); i!=clients.end(); )
    {
        if(i->operate([&](string s)-> void {Server::log(s);}, process_data))
        {
            SDLNet_TCP_DelSocket(socket_set, i->socket);
            SDLNet_TCP_Close(i->socket);
            user_names.erase(i->name);
            i = clients.erase(i);
        }
        else i++;
    }*/
}

}}
