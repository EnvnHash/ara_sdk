//
//  OSC.h
//  Tav_App
//
//  Created by Sven Hahne on 25/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__OSC__
#define __Tav_App__OSC__

#pragma once

#include <stdio.h>
#include <iostream>
#include <atomic>

#include <lo/lo.h>
#include <lo/lo_cpp.h>

namespace tav
{
class OSC
{
public:
	OSC()
	{
		osc_in_vals.resize(numOscScenePar);
		osc_vals.resize(numOscScenePar);

		for (int i = 0; i < numOscScenePar; i++)
		{
			osc_in_vals[i] = 0.0f;
			osc_vals[i] = 0.0;
		}

		// Create a server on a background thread.  Note, all message
		//handlers will run on the background thread!

		lo::ServerThread st(9000);
		if (!st.is_valid())
		{
			std::cout << "Nope." << std::endl;
		}

		std::cout << "URL: " << st.url() << std::endl;

		// Counter for number of messages received -- we use an atomic
		// because it will be updated in a background thread.
		std::atomic<int> received(0);

		/*
		 * Add a method handler for "/example,i" using a C++11 lambda to
		 * keep it succinct.  We capture a reference to the `received'
		 * count and modify it atomatically.
		 *
		 * You can also pass in a normal function, or a callable function
		 * object.
		 *
		 * Note: If the lambda doesn't specify a return value, the default
		 *       is `return 0', meaning "this message has been handled,
		 *       don't continue calling the method chain."  If this is not
		 *       the desired behaviour, add `return 1' to your method
		 *       handlers.
		 */
		st.add_method("example", "i", [&received](lo_arg **argv, int)
		{	std::cout << "example (" << (++received) << "): "
			<< argv[0]->i << std::endl;});

		// Start the server.
		st.start();

		// Send some messages to the server we just created on localhost.
		lo::Address a("localhost", "9000");

		// An individual message
		a.send("example", "i", 7890987);

		// Initalizer lists and message constructors are supported, so
		//that bundles can be created easily:
		a.send(lo::Bundle(
		{
		{ "example", lo::Message("i", 1234321) },
		{ "example", lo::Message("i", 4321234) } }));

		// Polymorphic overloads on lo::Message::add() mean you don't need
		// to specify the type explicitly.  This is intended to be useful
		// for templates.
		lo::Message m;
		m.add(7654321);
		a.send("example", m);

		// Resources are freed automatically, RAII-style, including closing the background server.
		std::cout << "Success!" << std::endl;
	}
	;
	~OSC();

	float seqParMed = 0.01f;
	float parMed = 0.1f;
	std::vector<float> osc_vals;
	std::vector<float> osc_in_vals;
	float scBlend = 0.f;
	int sceneNum1 = 0;
	int sceneNum2 = 0;
	float totalBrightness = 1.f;
	float feedback = 0.f;
	float alpha = 1.f;
	float rotYAxis = 0.f;
	float speed = 1.f;
	float backColor = 0.f;
	float startStopVideo = 1.f;
	float videoSpeed = 1.f;
	float zoom = 1.f;

	bool handlerFirstRun = true;
	bool firstRun = true;
	double actTime = 0.0;
	double lastTime = 0.0;

	short numOscScenePar = 12;
};
}

#endif /* defined(__Tav_App__OSC__) */
