/*
 *  OSCHandler.cpp
 *  ta_visualizer
 *
 *  Created by Sven Hahne on 06.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#include "pch.h"

#include <iostream>

#include "OSCHandler.h"
#include "OSCData.h"

using std::vector;

namespace tav
{

OSCHandler::OSCHandler(std::string& portNr) :
		osc_thread_(NULL), osc_server_(NULL)
{
	start(portNr);
}

void OSCHandler::start(std::string& portNr)
{
	m_Thread = std::thread(&OSCHandler::processQueue, this, &portNr);
}

void OSCHandler::join()
{
	m_Thread.join();
}

void OSCHandler::processQueue(std::string* portNr)
{
	osc_thread_ = lo_server_thread_new(portNr->c_str(), oscError);

	lo_server_thread_add_method(osc_thread_, "/clear", NULL, osc_enel_clear, NULL);
	lo_server_thread_add_method(osc_thread_, "/trig", NULL, osc_enel_trig, NULL);

	lo_server_thread_add_method(osc_thread_, "/data", NULL, osc_data, NULL);
	lo_server_thread_add_method(osc_thread_, "/seq", NULL, osc_seq, NULL);
	lo_server_thread_add_method(osc_thread_, "/slider", NULL, osc_slider, NULL);
	lo_server_thread_add_method(osc_thread_, "/startstop", NULL, osc_startstop, NULL);
	lo_server_thread_add_method(osc_thread_, "/getContrSpec", NULL, osc_sendContrSpec, NULL);
	lo_server_thread_add_method(osc_thread_, "/nodePar", NULL, osc_nodePar, NULL);
	lo_server_thread_add_method(osc_thread_, "/getMonitors", NULL, osc_getMonitors, NULL);
	lo_server_thread_add_method(osc_thread_, "/updtFboView", NULL, osc_addFboView, NULL);
	lo_server_thread_start(osc_thread_);
}

OSCHandler::~OSCHandler()
{
	//	lo_server_thread_stop(osc_thread_);
	//	lo_server_thread_free(osc_thread_);
}

void OSCHandler::oscError(int num, const char *msg, const char *path)
{
	std::cerr << "liblo server error " << num << "in path " << path << " "
			<< msg << std::endl;
	fflush(stdout);
}

int OSCHandler::osc_enel_clear(const char *path, const char *types, lo_arg **argv,
		int argc, void *data, void *user_data)
{
	printf(" OSCHandler clear \n");

	std::string sceneNames[3] = { "bodyWrite", "gestalt", "mural" };
	std::string com = "clear";

	for (unsigned int i=0; i<3;i++)
	{
		if (theOscData.sceneMap && theOscData.sceneMap->find(sceneNames[i]) != theOscData.sceneMap->end())
		{

			theOscData.sceneMap->at(sceneNames[i])->setPar(&com, 1.f);
		}
	}
    return 0;
}

int OSCHandler::osc_enel_trig(const char *path, const char *types, lo_arg **argv,
		int argc, void *data, void *user_data)
{
	printf(" OSCHandler trig \n");

	std::string sceneNames[3] = { "bodyWrite", "gestalt", "mural" };
	std::string com = "trig";

	for (unsigned int i=0; i<3;i++)
	{
		if (theOscData.sceneMap && theOscData.sceneMap->find(sceneNames[i]) != theOscData.sceneMap->end())
		{

			theOscData.sceneMap->at(sceneNames[i])->setPar(&com, 1.f);
		}
	}
    return 0;
}


int OSCHandler::osc_data(const char *path, const char *types, lo_arg **argv,
		int argc, void *data, void *user_data)
{
	if (argc >= NUMOSCSCENEPAR)
	{
		for (int i = 0; i < NUMOSCSCENEPAR; i++)
			theOscData.osc_in_vals[i] = argv[i]->f;

	}
	else
	{
		std::cerr
				<< "OSCHandler error: Number of SceneNode Parameter differs, Please increase in OSC-App!!"
				<< std::endl;
	}

	if (theOscData.handlerFirstRun)
		theOscData.handlerFirstRun = false;

	return 0;
}

// steuerung des internen sequencers
int OSCHandler::osc_seq(const char *path, const char *types, lo_arg **argv,
		int argc, lo_message msg, void *user_data)
{
	lo_message returnMsg;
	lo_address senderAddr = lo_message_get_source(msg);
	float startOffs = 0.f;

	if (argc >= 2)
	{
		int bundleId = argv[0]->i;

		// send confirmation
		if (bundleId != -1)
		{
			returnMsg = lo_message_new();
			std::string returnPath = "/n_" + std::to_string(bundleId);
			lo_send_message(senderAddr, returnPath.c_str(), returnMsg);
		}

		int cmd = argv[1]->i; // 0 = stop, 1 = start
		if (argc >= 4)
			startOffs = argv[2]->f;

		switch (cmd)
		{
		// stop
		case 0:
			std::cout << "stopping sequencer" << std::endl;
			break;
			// start
		case 1:
			std::cout << "starting sequencer" << std::endl;
			break;
		default:
			break;
		}
	}
	return 0;
}

int OSCHandler::osc_slider(const char *path, const char *types, lo_arg **argv,
		int argc, void *data, void *user_data)
{
	theOscData.sliderX = argv[0]->f;
	theOscData.sliderY = argv[1]->f;
	theOscData.sliderHasNewVal = true;

	return 0;
}

int OSCHandler::osc_startstop(const char *path, const char *types,
		lo_arg **argv, int argc, void *data, void *user_data)
{
	theOscData.extStart = argv[0]->i;
	theOscData.extStartHasNewVal = true;

	return 0;
}

int OSCHandler::osc_nodePar(const char *path, const char *types, lo_arg **argv,
		int argc, lo_message msg, void *user_data)
{
	std::string sceneName;
	std::string commandName;
	lo_message returnMsg;
	lo_address senderAddr = lo_message_get_source(msg);
	int nrItems;

	// struktur sollte sein "/nodePar", bundleId, nrOfCmds, nrOfCmdItems, "nodeName", "cmdName", val0, val1, ..., nrOfCmsItems, "nodeName", "cmdName2", val0, val1,
	// von supercollider als sendBundle(nil, ["/nodePar", ...l])
	// jeder rawvalue befehl sollte die form type, "nodename", "cmdName", val0, val1, etc haben
	// jeder env befehl sollte die form type, "nodename", "cmdName", nrEnvPoints, times0, times1, ..., val0, val1 etc haben

	if (argc >= 3)
	{
		int cmdOffs = 2;
		int bundleId = argv[0]->i;

		// send confirmation
		if (bundleId != -1)
		{
			returnMsg = lo_message_new();
			std::string returnPath = "/n_" + std::to_string(bundleId);
			lo_send_message(senderAddr, returnPath.c_str(), returnMsg);
		}


		// argv[1] = Nr Commands in the Osc String
		for (int cmdNr = 0; cmdNr < argv[1]->i; cmdNr++)
		{
			// nrItems in the Command
			nrItems = argv[2]->i;

			//std::cout << " parsing commmand nr: " << cmdNr << " nrItems: " << nrItems << std::endl;

			// error protection
			if (nrItems < 10000)
			{
				sceneName = (const char*) argv[++cmdOffs];
				commandName = (const char*) argv[++cmdOffs];

				//std::cout << "nrItems: " << nrItems << " sceneName: " << sceneName << " commanName: " << commandName << std::endl;

				if (theOscData.sceneMap && theOscData.sceneMap->find(sceneName) != theOscData.sceneMap->end())
				{
					cmdOffs++;

					if ( std::strcmp(commandName.c_str(), "active") != 0)
					{

						if (types[cmdOffs] == 'f')
						{
							//std::cout << "val " << argv[cmdOffs]->f << std::endl;
							theOscData.sceneMap->at(sceneName)->setPar(&commandName, argv[cmdOffs++]->f);
						}

						if (types[cmdOffs] == 's')
						{
							//std::cout << "val " << argv[cmdOffs]->s << std::endl;
							theOscData.sceneMap->at(sceneName)->setOscStrPar(&commandName, &argv[cmdOffs++]->s);
						}
					} else
					{
						//std::cout << "set active" << std::endl;
						if (argv[cmdOffs++]->f > 0.f)
						{
							printf("set active: %s %d \n", sceneName.c_str(), 1);
							theOscData.sceneMap->at(sceneName)->setActive(true);
						} else
						{
							printf("set active: %s %d \n", sceneName.c_str(), 0);
							theOscData.sceneMap->at(sceneName)->setActive(false);
						}
						//theOscData.sceneMap->at(sceneName)->setActive(bool(int(argv[cmdOffs++]->f)));
					}

				} else {

					cmdOffs += 2;
				}
			}
		}
	}
	else if (argc == 1)
	{
		// roundtrip time measurement
		returnMsg = lo_message_new();
		lo_message_add_int32(returnMsg, argv[0]->i);  // add width
		lo_send_message(senderAddr, "/nodeParAns", returnMsg);

	}
	else
	{
		std::cerr << "OSCHandler::osc_nodePar error: Number of arguments must be 3!" << std::endl;
	}

	return 0;
}

int OSCHandler::osc_getMonitors(const char *path, const char *types,
		lo_arg **argv, int argc, lo_message msg, void *user_data)
{
	GLFWmonitor* thisMonitor = 0;
	int xpos, ypos;

	// gui was requesting monitor information

	if (theOscData.monitors != 0)
	{
		// send a osc Message with all the monitor information
		lo_address senderAddr = lo_message_get_source(msg);
		lo_message returnMsg = lo_message_new();

		// add the width, height, x znd y Offset of the first glfw window
		lo_message_add_int32(returnMsg, theOscData.ctxWidth);  // add width
		lo_message_add_int32(returnMsg, theOscData.ctxHeight);  // add height
		lo_message_add_int32(returnMsg, theOscData.ctxXpos);  // add x pos
		lo_message_add_int32(returnMsg, theOscData.ctxYpos);  // add y pos

		// add fbo width and height
		lo_message_add_int32(returnMsg, theOscData.fboWidth);  // add x pos
		lo_message_add_int32(returnMsg, theOscData.fboHeight);  // add y pos

		lo_message_add_int32(returnMsg, theOscData.nrMonitors); // add nr monitors

		// collect all node Parameter and send them to the sender who requested them
		for (int i = 0; i < theOscData.nrMonitors; i++)
		{
			thisMonitor = theOscData.monitors[i];
			const GLFWvidmode* thisMode = glfwGetVideoMode(thisMonitor);
			glfwGetMonitorPos(thisMonitor, &xpos, &ypos);
			const char* monName = glfwGetMonitorName(thisMonitor);

			lo_message_add_int32(returnMsg, thisMode->width);  // add width
			lo_message_add_int32(returnMsg, thisMode->height);  // add height
			lo_message_add_int32(returnMsg, xpos);  // add x pos
			lo_message_add_int32(returnMsg, ypos);  // add y pos
			lo_message_add_string(returnMsg, monName);  // add name
		}

		// add views configured through setup.xml
		std::cout << theOscData.fboViews->size() << std::endl;
		lo_message_add_int32(returnMsg, (int) theOscData.fboViews->size()); // add nr monitors

		for (int i = 0; i < int(theOscData.fboViews->size()); i++)
		{
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->lowLeft.x); //0
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->lowLeft.y); //1

			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->lowRight.x); //2
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->lowRight.y); //3

			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->upRight.x); // 4
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->upRight.y); // 5

			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->upLeft.x); // 6
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->upLeft.y); // 7

			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->texOffs.x); // 8
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->texOffs.y); // 9

			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->texSize.x); // 11
			lo_message_add_int32(returnMsg,
					theOscData.fboViews->at(i)->texSize.y); // 12
		}

		lo_send_message(senderAddr, "/monitors", returnMsg);

	}
	else
	{
		printf("OSCHandler::osc_getMonitors error, no monitor information! \n");
	}

	return 0;
}

int OSCHandler::osc_addFboView(const char *path, const char *types,
		lo_arg **argv, int argc, lo_message msg, void *user_data)
{
	// gui was requesting monitor information
	// format: name, Ecke links unten, Ecke rechts unten, Ecke rechts oben, Ecke links oben, texOffs, texSize
	// alles in Pixeln
	if (argc == 19)
	{
		int fboViewIndex = argv[0]->i;

		if (int(theOscData.fboViews->size()) <= fboViewIndex)
		{
			printf(" add new fboView \n");
			theOscData.fboViews->push_back(new fboView());
		}

		theOscData.fboViews->at(fboViewIndex)->update = true;

		theOscData.fboViews->at(fboViewIndex)->id = fboViewIndex;
		theOscData.fboViews->at(fboViewIndex)->srcCamId = argv[1]->i;
		theOscData.fboViews->at(fboViewIndex)->ctxId = argv[2]->i;

		theOscData.fboViews->at(fboViewIndex)->lowLeft.x = argv[3]->f;
		theOscData.fboViews->at(fboViewIndex)->lowLeft.y = argv[4]->f;

		theOscData.fboViews->at(fboViewIndex)->lowRight.x = argv[5]->f;
		theOscData.fboViews->at(fboViewIndex)->lowRight.y = argv[6]->f;

		theOscData.fboViews->at(fboViewIndex)->upRight.x = argv[7]->f;
		theOscData.fboViews->at(fboViewIndex)->upRight.y = argv[8]->f;

		theOscData.fboViews->at(fboViewIndex)->upLeft.x = argv[9]->f;
		theOscData.fboViews->at(fboViewIndex)->upLeft.y = argv[10]->f;

		theOscData.fboViews->at(fboViewIndex)->texOffs.x = argv[11]->f;
		theOscData.fboViews->at(fboViewIndex)->texOffs.y = argv[12]->f;

		theOscData.fboViews->at(fboViewIndex)->texSize.x = argv[13]->f;
		theOscData.fboViews->at(fboViewIndex)->texSize.y = argv[14]->f;

		theOscData.fboViews->at(fboViewIndex)->lowBlend = argv[15]->f;
		theOscData.fboViews->at(fboViewIndex)->rightBlend = argv[16]->f;
		theOscData.fboViews->at(fboViewIndex)->upBlend = argv[17]->f;
		theOscData.fboViews->at(fboViewIndex)->leftBlend = argv[18]->f;

	}
	else
	{
		printf(
				"OSCHandler::osc_addFboView Error: Could not add View wrong number of arguments !!!\n");
	}

	return 0;
}

int OSCHandler::osc_sendContrSpec(const char *path, const char *types,
		lo_arg **argv, int argc, lo_message msg, void *user_data)
{
	// send a osc Message with all control specs
	lo_address senderAddr = lo_message_get_source(msg);
	lo_message returnMsg = lo_message_new();

	// collect all node Parameter and send them to the sender who requested them
	for (std::map<std::string, OSCData::contrSpec>::iterator it =
			theOscData.nodePar.begin(); it != theOscData.nodePar.end(); ++it)
	{
		lo_message_add_string(returnMsg, (*it).first.c_str());  // add name
		lo_message_add_float(returnMsg, (*it).second.min);  // add min
		lo_message_add_float(returnMsg, (*it).second.max);  // add max
		lo_message_add_float(returnMsg, (*it).second.step);  // add step
		lo_message_add_float(returnMsg, (*it).second.initVal);  // initVal
		lo_message_add_int32(returnMsg, (int) (*it).second.shape);  // initVal
	}

	lo_send_message(senderAddr, "/sendContrSpec", returnMsg);

	return 0;
}

void OSCHandler::sendFFT(const char* addr, const char* port,
		const char* msgName, float* data, int fftSize)
{
	lo_address t = lo_address_new(addr, port);
	lo_message msg = lo_message_new();
	lo_message_add_int32(msg, fftSize);

	for (int i = 0; i < fftSize; i++)
		lo_message_add_float(msg, data[i]);

	lo_send_message(t, msgName, msg);
	lo_message_free(msg);
}

void OSCHandler::stop()
{
	if (osc_thread_)
	{
		lo_server_thread_stop(osc_thread_);
		lo_server_thread_free(osc_thread_);
	}
}

}
