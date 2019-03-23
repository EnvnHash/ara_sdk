/*
 * UdpClient.h
 *
 *  Created on: Jul 14, 2018
 *      Author: sven
 */

#ifndef SRC_COMMUNICATION_UDPCLIENT_H_
#define SRC_COMMUNICATION_UDPCLIENT_H_

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <thread>

using boost::asio::ip::udp;
using boost::asio::ip::address;

namespace tav
{

class UdpClient
{
public:

    boost::asio::io_service io_service;
    udp::socket socket;
    boost::array<char, 1024> recv_buffer;
    udp::endpoint remote_endpoint;
    std::function<void(boost::array<char, 1024>*)> cb;

    // start receiving
    UdpClient(int _portNr) : socket(io_service, udp::endpoint(udp::v4(), _portNr))
    {}

    void startThread (){
		std::cout << "start thread" << std::endl;

		std::thread r([&] {
        	start_receive();

        	/*
            socket.open(udp::v4());
            remote_endpoint = udp::endpoint(udp::v4(), _portNr);
            socket.bind(remote_endpoint); // listen to port 6024
            */

            std::cout << "io_service.run\n";
            io_service.run();
            std::cout << "Receiver exit\n";
    	});

    	r.detach();
    }

    void start_receive()
    {
        socket.async_receive_from(boost::asio::buffer(recv_buffer),
                                   remote_endpoint,
                                   boost::bind(&UdpClient::handle_receive,
                                               this,
                                               boost::asio::placeholders::error,
                                               boost::asio::placeholders::bytes_transferred));
    }

    void handle_receive(const boost::system::error_code& error,
    		size_t bytes_transferred)
    {
        if (error) {
            std::cout << "Receive failed: " << error.message() << "\n";
            return;
        }

        cb(&recv_buffer);
        start_receive();
    }

    void setCb(std::function<void(boost::array<char, 1024>*)> _cb){
    	cb = _cb;
    }
};


}


#endif /* SRC_COMMUNICATION_UDPCLIENT_H_ */
