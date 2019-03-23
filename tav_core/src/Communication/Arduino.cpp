//
//  Arduino.cpp
//  tav_core
//
//  Created by Sven Hahne on 22/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "Arduino.h"

using namespace boost::asio;

namespace tav
{
Arduino::Arduino(std::string _port, int _baudRate, parity _par,
		stopBits _stopBits, flowContr _flow) :
		arduinoPort(io, _port), port(_port), baudRate(_baudRate), timeout(io), data_available(
				false), actPosition(0)
{
	read_msg = new char[max_read_length];

	serial_port_base::baud_rate BAUD(_baudRate);
	serial_port_base::parity PARITY;
	serial_port_base::stop_bits STOP;
	serial_port_base::flow_control FLOW;

	switch (_par)
	{
	case PAR_NONE:
		serial_port_base::parity PARITY(serial_port_base::parity::none);
		break;
	}

	switch (_stopBits)
	{
	case STOP_1:
		serial_port_base::stop_bits STOP(serial_port_base::stop_bits::one);
		break;
	}

	switch (_flow)
	{
	case FLOW_OFF:
		serial_port_base::flow_control FLOW(
				serial_port_base::flow_control::none);
		break;
	}

	// Setup port - base settings
	arduinoPort.set_option(BAUD);
	arduinoPort.set_option(FLOW);
	arduinoPort.set_option(PARITY);
	arduinoPort.set_option(STOP);
	printf("Arduino setup finished\n");

	// vermutlich hier thread starten
	m_Thread = std::thread(&Arduino::startReadQ, this);
}

Arduino::~Arduino()
{
}

void Arduino::startReadQ()
{
	read_start();
	io.run();
}

// Start an asynchronous read and call read_complete when it completes or fails
void Arduino::read_start(void)
{
	/*
	 boost::asio::async_read_until(arduinoPort,
	 buf,
	 '\n',
	 boost::bind(&Arduino::read_complete,
	 this,
	 boost::asio::placeholders::error,
	 boost::asio::placeholders::bytes_transferred));
	 */

	boost::asio::async_read(arduinoPort,
			boost::asio::buffer(read_msg, max_read_length),
			boost::asio::transfer_exactly(5),
			boost::bind(&Arduino::read_complete, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	/*
	 timeout.expires_from_now(boost::posix_time::milliseconds(100));
	 timeout.async_wait(boost::bind(&Arduino::wait_callback,
	 this,
	 boost::ref(arduinoPort),
	 boost::asio::placeholders::error()));
	 */
}

// the asynchronous read operation has now completed or failed and returned an error
void Arduino::read_complete(const boost::system::error_code& error,
		size_t bytes_transferred)
{
	// read completed, so process the data
	if (!error)
	{
		for (int i = 0; i < 4; i++)
			intByte.b[i] = read_msg[i + 1];

		if (read_msg[0] == 's')
			actSpeed = intByte.i;

		if (read_msg[0] == 'p')
			actPosition = intByte.i;

		//printf("msg: %s \n", read_msg); // echo to standard output
//            std::cout << "got: " << &os << std::endl;

//            delete read_msg;
//            read_msg = new char[max_read_length];
	}

	read_start(); // start waiting for another asynchronous read again
}

void Arduino::wait_callback(serial_port& ser_port,
		const boost::system::error_code& error)
{
	// Data was read and this timeout was canceled
	if (error)
		return;

	ser_port.cancel();  // will cause read_callback to fire with an error
}

void Arduino::send(const char _char, float _val)
{
	//        unsigned char command[1] = { 0 };
	unsigned char command[1] =
	{ static_cast<unsigned char>(_char) };

	// send command for moveToFloat
	boost::asio::write(arduinoPort, boost::asio::buffer(command, 1));

	//printf("send char: %d  value: %f\n", _char, _val);

	// Convert and float
	unsigned char* floatAsByte = reinterpret_cast<unsigned char*>(&_val);
	write(arduinoPort, buffer(floatAsByte, 4));
}

void Arduino::send(const char _char, int _val)
{
	//        unsigned char command[1] = { 0 };
	unsigned char command[1] =
	{ static_cast<unsigned char>(_char) };

	// send command for moveToFloat
	boost::asio::write(arduinoPort, boost::asio::buffer(command, 1));

	//printf("send integer char: %d  value: %d\n", _char, _val);

	unsigned char* intAsByte = reinterpret_cast<unsigned char*>(&_val);
	write(arduinoPort, buffer(intAsByte, 4));
}

void Arduino::send(const char _char)
{
	//        unsigned char command[1] = { 0 };
	unsigned char command[1] =
	{ static_cast<unsigned char>(_char) };

	// send command for moveToFloat
	boost::asio::write(arduinoPort, boost::asio::buffer(command, 1));
}

int Arduino::getActPos()
{
	return actPosition;
}

int Arduino::getActSpeed()
{
	return actSpeed;
}
}
