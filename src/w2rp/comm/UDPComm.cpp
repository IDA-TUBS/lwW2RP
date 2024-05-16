#include <w2rp/comm/UDPComm.hpp>

#include <iostream>
#include <bitset>


using namespace w2rp;

/*--------------------------------------- Public -----------------------------------------*/

UDPComm::UDPComm(
    struct socket_endpoint endpoint
):
    comm_context_(),
    socket_(comm_context_),
    endpoint_(generate_endpoint(endpoint)),
    send_lock()
{
    socket_.open(endpoint_.protocol());
    socket_.bind(endpoint_);
};

UDPComm::~UDPComm()
{
    socket_.close();
};


void UDPComm::sendMsg(
    MessageNet_t& msg,
    udp::endpoint target 
)
{
    std::lock_guard<std::mutex> lock(send_lock);
    socket_.send_to(boost::asio::buffer(msg.buffer, msg.length), target);
}

udp::endpoint UDPComm::receiveMsg(
    MessageNet_t& msg
)
{
    size_t ret_val = 0;

    // Store endpoint for request reply
    udp::endpoint sender_endpoint;

    ret_val = socket_.receive_from(boost::asio::buffer(msg.buffer, msg.max_size), sender_endpoint);

    msg.length = ret_val;

    return sender_endpoint;
}

struct socket_endpoint UDPComm::getEndpoint()
{
    return socket_endpoint(endpoint_.address().to_string(), endpoint_.port());
}

/*--------------------------------------- Private -----------------------------------------*/
udp::endpoint generate_endpoint(
    struct socket_endpoint endpoint
)
{
    return udp::endpoint(boost::asio::ip::address::from_string(endpoint.ip_addr), endpoint.port);
}

udp::endpoint generate_endpoint(
    std::string ip_addr,
    int port
)
{
    return udp::endpoint(boost::asio::ip::address::from_string(ip_addr), port); 
}

udp::endpoint generate_endpoint(int port)
{
    return udp::endpoint(boost::asio::ip::udp::v4(), port); 
}
