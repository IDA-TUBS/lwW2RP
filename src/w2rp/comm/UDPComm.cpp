#include <w2rp/comm/UDPComm.hpp>

#include <iostream>
#include <bitset>


namespace w2rp {

/*--------------------------------------- Public -----------------------------------------*/

UDPComm::UDPComm(
    struct socket_endpoint endpoint_rx,
    struct socket_endpoint endpoint_tx
):
    comm_context_(),
    socket_(comm_context_),
    rx_endpoint_(generate_endpoint(endpoint_rx)),
    tx_endpoint_(generate_endpoint(endpoint_tx)),
    send_lock()
{
    socket_.open(rx_endpoint_.protocol());
    socket_.bind(rx_endpoint_);
};

UDPComm::~UDPComm()
{
    socket_.close();
};

void UDPComm::sendMsg(
    MessageNet_t& msg 
)
{
    std::lock_guard<std::mutex> lock(send_lock);
    socket_.send_to(boost::asio::buffer(msg.buffer, msg.length), tx_endpoint_);
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

struct socket_endpoint UDPComm::getRxEndpoint()
{
    return socket_endpoint(rx_endpoint_.address().to_string(), rx_endpoint_.port());
}

struct socket_endpoint UDPComm::getTxEndpoint()
{
    return socket_endpoint(tx_endpoint_.address().to_string(), tx_endpoint_.port());
}

/*--------------------------------------- Private -----------------------------------------*/
udp::endpoint UDPComm::generate_endpoint(
    struct socket_endpoint endpoint
)
{
    return udp::endpoint(boost::asio::ip::address::from_string(endpoint.ip_addr), endpoint.port);
}

udp::endpoint UDPComm::generate_endpoint(
    std::string ip_addr,
    int port
)
{
    return udp::endpoint(boost::asio::ip::address::from_string(ip_addr), port); 
}

udp::endpoint UDPComm::generate_endpoint(int port)
{
    return udp::endpoint(boost::asio::ip::udp::v4(), port); 
}

} //end namespace