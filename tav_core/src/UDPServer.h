//
//  UDP_Server.h
//  tav_core
//
//  Created by Sven Hahne on 21/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//
#pragma once

#include <iostream>
#include <stdio.h>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <thread>
#include <mutex>

namespace tav
{
class UDPServer
{
public:
	UDPServer(int _port);
	~UDPServer();
	bool gotNew;
	union
	{
		uint8_t bytes[4];
		float f;
	} floatByte;
	std::string* getCmdStr();
private:
	void start_receive();
	void handle_receive(const boost::system::error_code& error,
			std::size_t /*bytes_transferred*/);
	void handle_send(boost::shared_ptr<std::string> message,
			const boost::system::error_code& error,
			std::size_t /*bytes_transferred*/);
	void processQueue(int portNr);

	boost::asio::io_service io_service;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint remote_endpoint_;
	boost::array<char, 4> recv_buffer_;
	std::string cmdStr;
	int port;

	std::thread m_Thread;
};
}
