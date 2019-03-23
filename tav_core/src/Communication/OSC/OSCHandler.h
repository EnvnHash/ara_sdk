/*
 *  OSCHandler.h
 *  ta_visualizer
 *
 *  Created by Sven Hahne on 06.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#include <lo/lo.h>

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>

#include "headers/global_vars.h"
#include "../../SceneNode.h"
#include "../../Envelope.h"

namespace tav
{

class OSCHandler
{
public:
	OSCHandler(std::string& portNr);
	~OSCHandler();
	void stop();

	void start(std::string& portNr);
	void join();
	virtual void processQueue(std::string* portNr);
	void sendFFT(const char* addr, const char* port, const char* msgName,
			float* data, int fftSize);

private:
	static void oscError(int num, const char *m, const char *path);

	static int osc_enel_trig(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data);
	static int osc_enel_clear(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data);


	static int osc_data(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data);
	static int osc_slider(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data);
	static int osc_seq(const char *path, const char *types, lo_arg **argv,
			int argc, lo_message msg, void *user_data);
	static int osc_startstop(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data);
	static int osc_sendContrSpec(const char *path, const char *types,
			lo_arg **argv, int argc, lo_message msg, void *user_data);
	static int osc_nodePar(const char *path, const char *types, lo_arg **argv,
			int argc, lo_message msg, void *user_data);
	static int osc_getMonitors(const char *path, const char *types,
			lo_arg **argv, int argc, lo_message msg, void *user_data);
	static int osc_addFboView(const char *path, const char *types,
			lo_arg **argv, int argc, lo_message msg, void *user_data);

	lo_server_thread osc_thread_ = nullptr;
	lo_server osc_server_ = nullptr;

	lo_server_thread osc_thread_2 = nullptr;
	lo_server osc_server_2 = nullptr;

	std::thread m_Thread;
};
}
