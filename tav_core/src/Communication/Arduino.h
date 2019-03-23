//
//  Arduino.h
//  tav_core
//
//  Created by Sven Hahne on 22/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <thread>
#include <mutex>

namespace tav
{
class Arduino
{
public:
	enum parity
	{
		PAR_NONE = 0
	};
	enum stopBits
	{
		STOP_1 = 0
	};
	enum flowContr
	{
		FLOW_OFF = 0
	};
	Arduino(std::string _port, int _baudRate, parity _par, stopBits _stopBits,
			flowContr _flow);
	~Arduino();
	void join();
	virtual void startReadQ();

	void read_start(void);
	void read_complete(const boost::system::error_code& error,
			size_t bytes_transferred);
	void send(const char _char, float _val);
	void send(const char _char, int _val);
	void send(const char _char);
	void wait_callback(boost::asio::serial_port& ser_port,
			const boost::system::error_code& error);

	int getActPos();
	int getActSpeed();
private:
	std::string port;
	int baudRate;
	boost::asio::io_service io;
	boost::asio::serial_port arduinoPort;
	boost::asio::deadline_timer timeout;
	bool data_available;
	static const int max_read_length = 32;
	char* read_msg; // data read from the socket

	std::thread m_Thread;

	int actPosition;
	int actSpeed;

	union
	{
		char b[4];
		int i;
	} intByte;
};
}
