//  GWindow.h
//  Test_Basic_GL4
//
//  Created by Sven Hahne on 04.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef Test_Basic_GL4_GWindow_h
#define Test_Basic_GL4_GWindow_h

#pragma once

#include "headers/gl_header.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class GWindow
{
public:
	bool debug = true;

	GWindow(){}

	//------------------------------------------------------------------------

	~GWindow(){}

	//------------------------------------------------------------------------

	int init(int width, int height, int refreshRate, bool fullScreen,
			bool useGL32p, int shiftX = 0, int shiftY = 0, int monitorNr = 0, bool decorated =
					false, bool floating = false, unsigned int nrSamples = 2,
					bool debug = false, GLFWwindow* _shareCont = 0)
	{
		printf("glfw version %s \n", glfwGetVersionString());

		screenWidth = width;
		screenHeight = height;
		monWidth = 0;
		monHeight = 0;

		glfwSetErrorCallback(error_callback);

		// Initialize the library
		if (!glfwInit())
			exit(EXIT_FAILURE);

		// get all the information about the monitors
		monitors = glfwGetMonitors(&count);

		if (fullScreen)
		{
			if (debug)
				for (int i = 0; i < count; i++)
					printf("monitor %i: %s current video mode: width: %i height: %i refreshRate: %i\n",
							i, glfwGetMonitorName(monitors[i]),
							glfwGetVideoMode(monitors[i])->width,
							glfwGetVideoMode(monitors[i])->height,
							glfwGetVideoMode(monitors[i])->refreshRate);

			if (count > monitorNr)
				useMonitor = monitorNr;

			// get Video Modes
			int countVm;
			bool found = false;
			const GLFWvidmode* modes = glfwGetVideoModes(monitors[useMonitor], &countVm);
			const GLFWvidmode* useThisMode = 0;

			if (debug)
				for (int j = 0; j < countVm; j++)
					printf("%i: current video mode: width: %i height: %i refreshRate: %i\n",
							j, modes[j].width, modes[j].height,
							modes[j].refreshRate);

			for (auto i = 0; i < countVm; i++)
			{
				if (modes[i].width == width && modes[i].height == height
						&& modes[i].refreshRate == refreshRate
						&& modes[i].redBits == 8)
				{
					found = true;
					useThisMode = &modes[i];
				}
			}

			if (!found)
			{
				const GLFWvidmode * mode = glfwGetVideoMode(
						monitors[useMonitor]);
				printf(
						"current video mode: width: %i height: %i refreshRate: %i\n",
						mode->width, mode->height, mode->refreshRate);
				printf("using unsupported videomode! \n");
			}
			else
			{
				printf("set video mode: width: %i height: %i refreshRate: %i\n",
						useThisMode->width, useThisMode->height,
						useThisMode->refreshRate);

				screenWidth = useThisMode->width;
				screenHeight = useThisMode->height;
				glfwWindowHint(GLFW_RED_BITS, useThisMode->redBits);
				glfwWindowHint(GLFW_GREEN_BITS, useThisMode->greenBits);
				glfwWindowHint(GLFW_BLUE_BITS, useThisMode->blueBits);
				glfwWindowHint(GLFW_REFRESH_RATE, useThisMode->refreshRate);
				monitorRefreshRate = useThisMode->refreshRate;
			}

			mon = monitors[useMonitor];

			// get the video mode of the current selected monitor
			const GLFWvidmode* mode = glfwGetVideoMode(mon);

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			glfwWindowHint(GLFW_SAMPLES, nrSamples);

			monitorRefreshRate = mode->refreshRate;

			screenWidth = mode->width;
			screenHeight = mode->height;
			monWidth = mode->width;
			monHeight = mode->height;

		}
		else
		{
			for (int i = 0; i < count; i++)
				printf("monitor %i: %s current video mode: width: %i height: %i refreshRate: %i\n",
						i, glfwGetMonitorName(monitors[i]),
						glfwGetVideoMode(monitors[i])->width,
						glfwGetVideoMode(monitors[i])->height,
						glfwGetVideoMode(monitors[i])->refreshRate);

			monitorRefreshRate = 60;

			// for non-fullscreen to always stay on top
			glfwWindowHint(GLFW_DECORATED, decorated ? GL_TRUE : GL_FALSE);
			glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

			//glfwWindowHint(GLFW_FOCUSED, GL_TRUE);
			//glfwWindowHint(GLFW_REFRESH_RATE, monitorRefreshRate);
			glfwWindowHint(GLFW_SAMPLES, nrSamples);

#ifndef __EMSCRIPTEN__
			glfwWindowHint(GLFW_FLOATING, floating); // ignored for fullscreen, necessary
#endif
			const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
			monWidth = mode->width;
			monHeight = mode->height;
		}

		 printf("set gl version 4.5 \n");
		 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		if (debug) glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

		//glfwWindowHint(GLFW_AUTO_ICONIFY, GL_TRUE);

		if (mon) printf("creating fullscreen window\n");

		// printf("screenWidth %d screenHeight %d \n", screenWidth, screenHeight);

		window = glfwCreateWindow(screenWidth, screenHeight, "", mon, _shareCont);

		if (!window)
		{
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		// kommt immer 0 raus....
//        int left, top, right, bottom;
//        glfwGetWindowFrameSize(window, &left, &top, &right, &bottom);
//        if(debug) printf("window framebuffer limits: left: %d top:%d right:%d bottom:%d \n", left, top, right, bottom);

		glfwMakeContextCurrent(window);

		if (!fullScreen)
		{
			printf("shift x window: %d y: %d\n", shiftX, shiftY);
			glfwSetWindowPos(window, shiftX, shiftY);
			glfwShowWindow(window);
		}

		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n", glGetString(GL_VERSION));
		printf("GLSL:     %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

		if (nrSamples > 2)
		{
			glEnable(GL_MULTISAMPLE_ARB);
			printf("enabling Multisampling \n");
		}
		inited = true;

		return 0;
	}

	//------------------------------------------------------------------------

	unsigned int getMonitorWidth()
	{
		return monWidth;
	}

	//------------------------------------------------------------------------

	unsigned int getMonitorHeight()
	{
		return monHeight;
	}

	//------------------------------------------------------------------------

	unsigned int getScreenWidth()
	{
		return screenWidth;
	}

	//------------------------------------------------------------------------

	unsigned int getScreenHeight()
	{
		return screenHeight;
	}

	//------------------------------------------------------------------------

	glm::ivec2 getSize()
	{
		glm::ivec2 size;
		glfwGetWindowSize(window, &size.x, &size.y);
		return size;
	}

	//------------------------------------------------------------------------

	glm::ivec2 getPosition()
	{
		glm::ivec2 pos;
		glfwGetWindowPos(window, &pos.x, &pos.y);
		return pos;
	}

	//------------------------------------------------------------------------

	int getMonitorId()
	{
		return useMonitor;
	}

	//------------------------------------------------------------------------

	int getFocus()
	{
		int foc = glfwGetWindowAttrib(window, GLFW_FOCUSED);
		return foc;
	}

	//------------------------------------------------------------------------

	void makeCurrent()
	{
		glfwMakeContextCurrent(window);
	}

	//------------------------------------------------------------------------

	void swap()
	{
		glfwSwapBuffers(window);
	}

	//------------------------------------------------------------------------

	void pollEvents()
	{
		glfwPollEvents();
	}

	//------------------------------------------------------------------------

	void setId(int _id){
		id = _id;
	}

	//------------------------------------------------------------------------

	void setSwapInterval(bool _set)
	{
		glfwSwapInterval(_set);
	}

	//------------------------------------------------------------------------

	void setKeyCallback(
			void (*f)(GLFWwindow* window, int key, int scancode, int action,
					int mods))
	{
		glfwSetKeyCallback(window, f);
	}

	//------------------------------------------------------------------------

	void setWindowSizeCallback(
			void (*f)(GLFWwindow* window, int width, int height))
	{
		glfwSetWindowSizeCallback(window, f);
	}

	//------------------------------------------------------------------------

	void setMouseCursorCallback(
			void (*f)(GLFWwindow* window, double xpos, double ypos))
	{
		glfwSetCursorPosCallback(window, f);
	}

	//------------------------------------------------------------------------

	void setMouseButtonCallback(
			void (*f)(GLFWwindow* window, int button, int action, int mods))
	{
		glfwSetMouseButtonCallback(window, f);
	}

	//------------------------------------------------------------------------

	void setCloseCallback(void (*f)(GLFWwindow* window))
	{
		glfwSetWindowCloseCallback(window, f);
	}

	//------------------------------------------------------------------------

	static void error_callback(int error, const char* description)
	{
		fputs(description, stderr);
	}

	//------------------------------------------------------------------------

	void runLoop(void (*f)())
	{
		// Loop until the user closes the window
		while (!glfwWindowShouldClose(window))
		{
			f();

			// Swap front and back buffers
			glfwSwapBuffers(window);

			// Poll for and process events
			glfwPollEvents();
		}

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	//------------------------------------------------------------------------

	void open()
	{
		glfwShowWindow(window);
		isOpen = true;
	}

	//------------------------------------------------------------------------

	void close()
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		isOpen = false;
	}

	//------------------------------------------------------------------------

	GLFWwindow* getWin()
	{
		return window;
	}

	//------------------------------------------------------------------------

	void getFps()
	{
#ifdef USE_WEBGL
		// update time counter, get dt and smooth it
		if (medDt == 0.066)
		{
			if (lastTime != 0.0) medDt = emscripten_get_now() - lastTime;
		}
		else
		{
			medDt = ((emscripten_get_now() - lastTime) + (medDt * 30.0)) / 31.0;
		}

		lastTime = emscripten_get_now();

		if ((lastTime - lastPrintFps) > 1000.0)
		{
			printf("FPS: %f dt: %fms\n", 1000.0 / medDt, medDt);
			lastPrintFps = lastTime;
		}
#else
		// update time counter, get dt and smooth it
		if (medDt == 0.066)
		{
			if (lastTime != 0.0)
				medDt = glfwGetTime() - lastTime;
		}
		else
		{
			medDt = ((glfwGetTime() - lastTime) + (medDt * 30.0)) / 31.0;
		}

		lastTime = glfwGetTime();

		if ((lastTime - lastPrintFps) > printFpsIntv)
		{
			printf("FPS: %f dt: %f\n", 1.0 / medDt, medDt);
			lastPrintFps = lastTime;
		}
#endif
	}

	//------------------------------------------------------------------------

	GLFWmonitor** getMonitors()
	{
		return monitors;
	}

	//------------------------------------------------------------------------

	int getNrMonitors()
	{
		return count;
	}

	//------------------------------------------------------------------------

	int monitorRefreshRate;
	int useMonitor = 0;
	bool isOpen = false;
	bool inited = false;
	int id = 0;

private:
	double medDt = 0.066;
	double lastTime = 0;
	double printFpsIntv = 2.0;
	double lastPrintFps = 0.0;
	int count;
	int monWidth;
	int monHeight;
	int screenWidth;
	int screenHeight;
	GLFWwindow* window;
	GLFWmonitor** monitors = 0;
	GLFWmonitor* mon = 0;
};

#endif
