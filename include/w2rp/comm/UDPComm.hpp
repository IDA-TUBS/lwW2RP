#ifndef UDPComm_h
#define UDPComm_h

#include <w2rp/messages/messages.hpp>
#include <w2rp/messages/message_net.hpp>
#include <w2rp/comm/socketEndpoint.hpp>

#include <w2rp/log.hpp>

#include <boost/asio.hpp>
#include <string>
#include <chrono>
#include <mutex>

using boost::asio::ip::udp;

namespace w2rp {

class UDPComm 
{
    public:

    /**
     * @brief Construct a new UDPComm object
     * 
     * @param endpoint_rx socket endpoint struct for rx
     * @param endpoint_rx socket endpoint struct for tx
     */
    UDPComm(
        struct socket_endpoint endpoint_rx,
        struct socket_endpoint endpoint_tx
    );

    /**
     * @brief Destroy the UDPComm object
     * 
     */
    ~UDPComm();

    /**
     * @brief Sends a UDP datagram via tx_endpoint
     * 
     * @param msg serialized message_net object to transmit
     */
    void sendMsg(
        MessageNet_t& msg
    );

    /**
     * @brief receive UDP datagrams targeted at this object instance (IP).
     * 
     * @param msg message_net object, stores received udp datagram
     * @return udp::endpoint sender endpoint
     */
    udp::endpoint receiveMsg(
        MessageNet_t& msg
    );

    /**
     * @brief Get the Endpoint object
     * 
     * @return struct socket_endpoint 
     */
    struct socket_endpoint getRxEndpoint();

    /**
     * @brief set the Endpoint object
     * 
     * @param struct containing updated socket_endpoint 
     */
    void setTxEndpoint(struct socket_endpoint endpoint_tx);


    /**
     * @brief Get the Endpoint object
     * 
     * @return struct socket_endpoint 
     */
    struct socket_endpoint getTxEndpoint();

    private:
    
    /*-------------------------- Methods ---------------------------------*/
    /**
     * @brief generate a boost::asio::udp::endpoint
     * 
     * @param endpoint socket_endpoint struct
     * @return udp::endpoint 
     */
    udp::endpoint generate_endpoint(struct socket_endpoint endpoint);
    
    /**
     * @brief  generate a boost::asio::udp::endpoint
     * 
     * @param ip_addr endpoint ip address
     * @param port endpoint port 
     * @return udp::endpoint 
     */
    udp::endpoint generate_endpoint(
        std::string ip_addr,
        int port
    );
    
    /**
     * @brief generate a boost::asio::udp::endpoint for all interfaces (0.0.0.0)
     * 
     * @param port endpoint port 
     * @return udp::endpoint 
     */
    udp::endpoint generate_endpoint(int port);
    
    /*-------------------------- Attributes ---------------------------------*/
    boost::asio::io_context comm_context_;

    udp::socket socket_;
    udp::endpoint rx_endpoint_;
    udp::endpoint tx_endpoint_;
    
    std::mutex send_lock;
};


}; // End namespace w2rp

#endif