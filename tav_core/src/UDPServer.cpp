//
//  UDP_Server.cpp
//  tav_core
//
//  Created by Sven Hahne on 21/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "UDPServer.h"

using boost::asio::ip::udp;

namespace tav
{
UDPServer::UDPServer(int _port) :
		socket_(io_service, udp::endpoint(udp::v4(), _port)), port(_port), gotNew(
				false)
{
	m_Thread = std::thread(&UDPServer::processQueue, this, port);
	cmdStr = "0000";
}

//------------------------------------------------------------------------

void UDPServer::processQueue(int portNr)
{
	try
	{
		start_receive();
		io_service.run();
	} catch (std::exception& e)
	{
		std::cerr << "tav::UDPServer Error: couldn´t start Server!!! "
				<< e.what() << std::endl;
	}
}

//------------------------------------------------------------------------

void UDPServer::start_receive()
{
	//std::cout << "udp_server::start_receive on port " << port << std::endl;
	socket_.async_receive_from(boost::asio::buffer(recv_buffer_),
			remote_endpoint_,
			boost::bind(&UDPServer::handle_receive, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	for (int i = 0; i < 4; i++)
		cmdStr[i] = recv_buffer_.at(i);

	for (int i = 0; i < 4; i++)
		floatByte.bytes[i] = recv_buffer_.at(i);

	gotNew = true;
//        std::cout << "received " << floatByte.f << std::endl;
}

//------------------------------------------------------------------------

void UDPServer::handle_receive(const boost::system::error_code& error,
		std::size_t /*bytes_transferred*/)
{
	// std::cout << "call handle_receive" << std::endl;

	if (!error || error == boost::asio::error::message_size)
	{
		/*
		 float fooFloat = 1.0022334f;
		 std::cout << "fooFloat: " << fooFloat << std::endl;
		 const char * px = reinterpret_cast<const char*>(&fooFloat);  // no type punning, cast-to-char is allowed
		 
		 //            boost::shared_ptr<std::string> message(new std::string(make_daytime_string()));
		 boost::shared_ptr<std::string> message(new std::string(px));
		 
		 socket_.async_send_to(boost::asio::buffer(px, sizeof(float)),
		 remote_endpoint_,
		 boost::bind(&UDPServer::handle_send,
		 this,
		 message,
		 boost::asio::placeholders::error,
		 boost::asio::placeholders::bytes_transferred));
		 */

		/*
		 boost::shared_ptr<std::string> message(new std::string(make_daytime_string()));
		 socket_.async_send_to(
		 boost::asio::buffer(fooFloat),
		 remote_endpoint_,
		 boost::bind(&udp_server::handle_send,
		 this,
		 message,
		 boost::asio::placeholders::error,
		 boost::asio::placeholders::bytes_transferred));
		 */

		start_receive();
	}
}

//------------------------------------------------------------------------

void UDPServer::handle_send(boost::shared_ptr<std::string> message,
		const boost::system::error_code& error,
		std::size_t /*bytes_transferred*/)
{
	std::cout << "sending: " << *message << std::endl;
	std::cout << "error code: " << error << std::endl;
}

//------------------------------------------------------------------------

std::string* UDPServer::getCmdStr()
{
	return &cmdStr;
}

//----------------------------------------------------

UDPServer::~UDPServer()
{
}

}
