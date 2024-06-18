#ifndef SocketEndpoint_W2RP_H
#define SocketEndpoint_W2RP_H

#include <string>
#include <chrono>

namespace w2rp {

/**
 * @brief Describes the parameters of an boost asio endpoint. Used for passing endpoints to functions.
 * 
 */
struct socket_endpoint
{
    std::string ip_addr;
    int port;

    socket_endpoint(){};
    socket_endpoint(    
        std::string ip, 
        int p
    ):
        ip_addr(ip),
        port(p)
    {};
};

}; // end namespace

#endif //SocketEndpoint_W2RP_H