//
//  GWindowManager.h
//  tav_core
//
//  Created by Sven Hahne on 19/11/15.
//  Copyright © 2015 Sven Hahne. All rights reserved..
//

#ifndef GWindowManager_h
#define GWindowManager_h
#pragma once

#include <vector>
#include <functional>
#include <map>
#include <iostream>

#include "GWindow.h"
#include "GUI/RootWidget.h"

namespace tav
{
typedef std::function<void(int key, int scancode, int action, int mods)> GKeyCallbackFun;
typedef std::function<void(int button, int action, int mods, double xPos, double yPos)> GMouseButCallbackFun;
typedef std::function<void(double xPos, double yPos)> GCursorCallbackFun;
typedef struct
{
	bool fullScreen;
	bool useGL32p;
	bool decorated = false;
	bool floating = false;
	bool debug = false;
	unsigned int nrSamples;
	int shiftX = 0;
	int monitorNr = 0;
	int width;
	int height;
	int refreshRate;
} gWinPar;

class GWindowManager
{
public:
	~GWindowManager()
	{
	}

	//-------------------------------------------------------------------------------------

	void runMainLoop(void (*f)(double time, double dt, unsigned int ctxNr))
	{
		run = true;

		while (run)
		{
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

			if (showFps && (lastTime - lastPrintFps) > printFpsIntv)
			{
				printf("FPS: %f dt: %f\n", 1.0 / medDt, medDt);
				lastPrintFps = lastTime;
			}

			// update widgets
			for (std::vector<RootWidget*>::iterator it = rootWidgets.begin();
					it != rootWidgets.end(); ++it)
				(*it)->update(lastTime, medDt);

			// update windows
			if (multCtx)
			{
				winInd = 0;
				for (std::vector<GWindow*>::iterator it = windows.begin();
						it != windows.end(); ++it)
				{
					if ((*it)->isOpen
							&& !glfwWindowShouldClose((*it)->getWin()))
					{
						glfwMakeContextCurrent((*it)->getWin());
						f(lastTime, medDt, winInd);
					}
					winInd++;
				}

				for (std::vector<GWindow*>::iterator it = windows.begin();
						it != windows.end(); ++it)
					if ((*it)->isOpen
							&& !glfwWindowShouldClose((*it)->getWin()))
						glfwSwapBuffers((*it)->getWin());

			}
			else
			{
				if (!glfwWindowShouldClose(windows[0]->getWin()))
				{
					f(lastTime, medDt, winInd);
					glfwSwapBuffers(windows[0]->getWin());
				}
			}

			glfwPollEvents(); // Poll for and process events

			// check if new windows were requested
			for (std::vector<gWinPar*>::iterator it = addWindows.begin();
					it != addWindows.end(); ++it)
				addWinGPar((*it));

			// clear the list of added windows
			addWindows.clear();
		}

		for (std::vector<GWindow*>::iterator it = windows.begin();
				it != windows.end(); ++it)
			glfwDestroyWindow((*it)->getWin());

		glfwTerminate();
	}

	//-------------------------------------------------------------------------------------

	void winPushBack(int width, int height, int refreshRate, bool fullScreen,
			bool useGL32p, int shiftX = 0, int monitorNr = 0, bool decorated =
					false, bool floating = false, unsigned int nrSamples = 2,
			bool debug = false)
	{
		addWindows.push_back(new gWinPar());
		addWindows.back()->width = width;
		addWindows.back()->height = height;
		addWindows.back()->refreshRate = refreshRate;
		addWindows.back()->fullScreen = fullScreen;
		addWindows.back()->useGL32p = useGL32p;
		addWindows.back()->shiftX = shiftX;
		addWindows.back()->monitorNr = monitorNr;
		addWindows.back()->decorated = decorated;
		addWindows.back()->floating = floating;
		addWindows.back()->nrSamples = nrSamples;
		addWindows.back()->debug = debug;
	}

	//-------------------------------------------------------------------------------------

	GWindow* addWin(int width, int height, int refreshRate, bool fullScreen,
			bool useGL32p, int shiftX = 0, int shiftY = 0, int monitorNr = 0, bool decorated =
					false, bool floating = false, unsigned int nrSamples = 2,
			bool debug = false)
	{
		GLFWwindow* shareWin = 0;

		printf("GWindowmanager: add width %d height %d refreshRate %d fullScreen %d useGL32p %d, shiftX %d shiftY %d monitorNr %d, decorated %d nrSamples: %d floating: %d\n",
				width, height, refreshRate, fullScreen, useGL32p, shiftX, shiftY,
				monitorNr, decorated, nrSamples, floating);
		// bla
		if (static_cast<unsigned int>(windows.size()) != 0)
			shareWin = shareCtx;

		windows.push_back(new GWindow());
		windows.back()->init(width, height, refreshRate, fullScreen, useGL32p,
				shiftX, shiftY, monitorNr, decorated, floating, nrSamples, debug,
				shareWin);

		if (static_cast<unsigned int>(windows.size()) == 1)
			shareCtx = windows.back()->getWin();

		// add new callbackMaps for the new window
		// new callback will be added to the cb-function-vector of this map
		keyCbMap[windows.back()->getWin()] = new std::vector<GKeyCallbackFun>();
		mouseButCbMap[windows.back()->getWin()] = new std::vector<GMouseButCallbackFun>();
		cursorCbMap[windows.back()->getWin()] = new std::vector<GCursorCallbackFun>();

		glfwSetKeyCallback(windows.back()->getWin(), GWindowManager::gWinKeyCallback);
		glfwSetMouseButtonCallback(windows.back()->getWin(), GWindowManager::gMouseButCallback);
		glfwSetCursorPosCallback(windows.back()->getWin(), GWindowManager::gMouseCursorCallback);

		// glfwSetWindowSizeCallback(windows.back()->getWin(), );

		if (static_cast<int>(windows.size()) > 1)
			multCtx = true;
		else
			multCtx = false;

		return windows.back();
	}

	//-------------------------------------------------------------------------------------

	GWindow* addWinGPar(gWinPar* gp)
	{
		GLFWwindow* shareWin = 0;

		//printf("GWindowmanager: add width %d height %d refreshRate %d fullScreen %d useGL32p %d, shiftX %d monitorNr %d, decorated %d \n", gp->width, gp->height, gp->refreshRate, gp->fullScreen, gp->useGL32p, gp->shiftX, gp->monitorNr, gp->decorated);

		if (static_cast<unsigned int>(windows.size()) != 0)
			shareWin = shareCtx;

		windows.push_back(new GWindow());
		windows.back()->init(gp->width, gp->height, gp->refreshRate,
				gp->fullScreen, gp->useGL32p, gp->shiftX, gp->monitorNr,
				gp->decorated, gp->floating, gp->nrSamples, gp->debug,
				shareWin);

		if (static_cast<unsigned int>(windows.size()) == 1)
			shareCtx = windows.back()->getWin();

		// add new callbackMaps for the new window
		// new callback will be added to the cb-function-vector of this map
		keyCbMap[windows.back()->getWin()] = new std::vector<GKeyCallbackFun>();
		mouseButCbMap[windows.back()->getWin()] = new std::vector<
				GMouseButCallbackFun>();

		glfwSetKeyCallback(windows.back()->getWin(),
				GWindowManager::gWinKeyCallback);
		glfwSetMouseButtonCallback(windows.back()->getWin(),
				GWindowManager::gMouseButCallback);
		glfwSetCursorPosCallback(windows.back()->getWin(),
				GWindowManager::gMouseCursorCallback);

		//printf("GWindowmanager end\n");

		return windows.back();
	}

	//-------------------------------------------------------------------------------------

	void addWidget(unsigned int winInd, Widget* _widg, ShaderCollector* _shCol)
	{
		if (static_cast<unsigned int>(windows.size()) >= (winInd + 1))
		{
			if (static_cast<unsigned int>(rootWidgets.size()) < (winInd + 1))
			{
				// hier müsste eigentlich die gesamtgroesse übergeben werden, so dass auch
				// über das fenster hinaus (etwas für preview) gezeichnet werden kann
				rootWidgets.push_back(
						new RootWidget(_shCol,
								windows[winInd]->getScreenWidth(),
								windows[winInd]->getScreenHeight(), 1));
				rootWidgets.back()->add(_widg);
				rootWidgets.back()->init();    // nrViewports

				addKeyCallback(winInd,
						[this](int key, int scancode, int action, int mods)
						{
							return rootWidgets.back()->onKey(key, scancode, action, mods);
						});
				addMouseButCallback(winInd,
						[this](int button, int action, int mods, double xPos, double yPos)
						{
							return rootWidgets.back()->onMouseButton(button, action, mods, xPos, yPos);
						});
			}
			else
			{
				rootWidgets[winInd]->add(_widg);
			}

		}
		else
		{
			printf(
					"tav::GWindowManager Error: can´t add Widget. Window doesn´t exist. \n");
		}
	}

	//-------------------------------------------------------------------------------------

	void setWidgetCmd(unsigned int winInd, double xpos, double ypos,
			widgetEvent _event)
	{
		if (static_cast<unsigned int>(rootWidgets.size()) >= (winInd + 1))
		{
			rootWidgets[winInd]->setCmd(xpos, ypos, _event);
		}
		else
		{
			printf(
					"tav::GWindowManager Error: can´t set Cmd. RootWidget doesn´t exist. \n");
		}
	}

	//-------------------------------------------------------------------------------------

	void drawWidget(unsigned int winInd)
	{
		if (static_cast<unsigned int>(rootWidgets.size()) >= (winInd + 1))
		{
			rootWidgets[winInd]->draw();
		}
		else
		{
			printf(
					"tav::GWindowManager Error: can´t draw RootWidget. It doesn´t exist. \n");
		}
	}

	//-------------------------------------------------------------------------------------

	void setGlobalKeyCallback(
			void (*f)(GLFWwindow* window, int key, int scancode, int action,
					int mods))
	{
		keyCbFun = f;
	}

	//-------------------------------------------------------------------------------------

	void setGlobalMouseCursorCallback(
			void (*f)(GLFWwindow* window, double xpos, double ypos))
	{
		mouseCursorCbFun = f;
	}

	//-------------------------------------------------------------------------------------

	void setGlobalMouseButtonCallback(
			void (*f)(GLFWwindow* window, int button, int action, int mods))
	{
		mouseButtonCbFun = f;
	}

	//-------------------------------------------------------------------------------------

	void setWinResizeCallback(
			void (*f)(GLFWwindow* window, int width, int height))
	{
		winResizeCbFun = f;
	}

	//-------------------------------------------------------------------------------------

	void addKeyCallback(unsigned int winInd, GKeyCallbackFun _func)
	{
		if (static_cast<unsigned int>(windows.size()) >= (winInd + 1))
		{
			keyCbMap[windows[winInd]->getWin()]->push_back(_func);
		}
		else
		{
			printf(
					"tav::GWindowManager::setKeyCallback Error: window doesn´t exist. \n");
		}
	}

	//-------------------------------------------------------------------------------------

	void addMouseButCallback(unsigned int winInd, GMouseButCallbackFun _func)
	{
		if (static_cast<unsigned int>(windows.size()) >= (winInd + 1))
		{
			mouseButCbMap[windows[winInd]->getWin()]->push_back(_func);
		}
		else
		{
			printf(
					"tav::GWindowManager::addMouseButCallback Error: window doesn´t exist. \n");
		}
	}

	//-------------------------------------------------------------------------------------

	void addCursorCallback(unsigned int winInd, GCursorCallbackFun _func)
	{
		if (static_cast<unsigned int>(windows.size()) >= (winInd + 1))
		{
			cursorCbMap[windows[winInd]->getWin()]->push_back(_func);
		}
		else
		{
			printf(
					"tav::GWindowManager::addCursorCallback Error: window doesn´t exist. \n");
		}
	}

	//-------------------------------------------------------------------------------------

	static void gWinKeyCallback(GLFWwindow* window, int key, int scancode,
			int action, int mods)
	{
		// go through the keyCallback vectors and call the corresponding functions
		for (std::vector<GKeyCallbackFun>::iterator it =
				GWindowManager::shared_instance().keyCbMap[window]->begin();
				it != GWindowManager::shared_instance().keyCbMap[window]->end();
				++it)
		{
			(*it)(key, scancode, action, mods);
		}

		// call the global function which applies for all windows
		GWindowManager::shared_instance().keyCbFun(window, key, scancode,
				action, mods);
	}

	//-------------------------------------------------------------------------------------

	static void gMouseButCallback(GLFWwindow* window, int button, int action,
			int mods)
	{
		// go through the mouseCallback vectors and call the corresponding functions of the actual window
		// that is the rootWidget callback of this window
		for (std::vector<GMouseButCallbackFun>::iterator it =
				GWindowManager::shared_instance().mouseButCbMap[window]->begin();
				it
						!= GWindowManager::shared_instance().mouseButCbMap[window]->end();
				++it)
		{
			(*it)(button, action, mods,
					GWindowManager::shared_instance().lastMouseX,
					GWindowManager::shared_instance().lastMouseY);
		}

		// call the global function which applies for all windows
		GWindowManager::shared_instance().mouseButtonCbFun(window, button,
				action, mods);
	}

	//-------------------------------------------------------------------------------------

	static void gMouseCursorCallback(GLFWwindow* window, double xpos,
			double ypos)
	{
		// go through the keyCallback vectors and call the corresponding functions
		for (std::vector<GCursorCallbackFun>::iterator it =
				GWindowManager::shared_instance().cursorCbMap[window]->begin();
				it
						!= GWindowManager::shared_instance().cursorCbMap[window]->end();
				++it)
		{
			(*it)(xpos, ypos);
		}

		// call the global function which applies for all windows
		GWindowManager::shared_instance().mouseCursorCbFun(window, xpos, ypos);

		// save last mouse Pos
		GWindowManager::shared_instance().lastMouseX = xpos;
		GWindowManager::shared_instance().lastMouseY = ypos;
	}

	//-------------------------------------------------------------------------------------

	void setSwapInterval(unsigned int winNr, bool _swapInterval)
	{
		if (static_cast<unsigned int>(windows.size()) >= winNr)
			windows[winNr]->setSwapInterval(_swapInterval);
	}

	//-------------------------------------------------------------------------------------

	void setPrintFps(bool _val)
	{
		showFps = _val;
	}

	//-------------------------------------------------------------------------------------

	unsigned int getMonitorWidth(unsigned int winNr)
	{
		if (static_cast<unsigned int>(windows.size()) >= winNr - 1)
			return windows[winNr]->getMonitorWidth();

		return 0;
	}

	//-------------------------------------------------------------------------------------

	unsigned int getMonitorHeight(unsigned int winNr)
	{
		if (static_cast<unsigned int>(windows.size()) >= winNr - 1)
			return windows[winNr]->getMonitorHeight();

		return 0;
	}

	//-------------------------------------------------------------------------------------

	GWindow* getFirstWin()
	{
		return windows.front();
	}

	//-------------------------------------------------------------------------------------

	unsigned int getInd(GLFWwindow* win)
	{
		unsigned int ctxInd = 0;
		unsigned int winInd = 0;
		for (std::vector<GWindow*>::iterator it = windows.begin();
				it != windows.end(); ++it)
		{
			if (win == (*it)->getWin())
			{
				ctxInd = winInd;
			}
			winInd++;
		}
		return ctxInd;
	}

	//-------------------------------------------------------------------------------------

	void closeAll()
	{
		run = false;
		for (std::vector<GWindow*>::iterator it = windows.begin();
				it != windows.end(); ++it)
			(*it)->close();
	}

	//-------------------------------------------------------------------------------------

	void stop()
	{
		run = false;
	}

	//-------------------------------------------------------------------------------------

	static GWindowManager &shared_instance()
	{
		static GWindowManager gWinMan;
		return gWinMan;
	}

	//-------------------------------------------------------------------------------------

	GWindow* getBack()
	{
		return windows.back();
	}

	//-------------------------------------------------------------------------------------

	std::vector<GWindow*>* getWindows()
	{
		return &windows;
	}

	//-------------------------------------------------------------------------------------

	void (*keyCbFun)(GLFWwindow* window, int key, int scancode, int action,
			int mods);
	void (*mouseCursorCbFun)(GLFWwindow* window, double xpos, double ypos);
	void (*mouseButtonCbFun)(GLFWwindow* window, int button, int action,
			int mods);
	void (*winResizeCbFun)(GLFWwindow* window, int width, int height);

	double lastMouseX;
	double lastMouseY;

private:
	// make constructor private only share_instance method will create an instance
	GWindowManager()
	{
	}

	std::vector<gWinPar*> addWindows;
	std::vector<GWindow*> windows;
	std::vector<RootWidget*> rootWidgets;
	std::map<GLFWwindow*, std::vector<GKeyCallbackFun>*> keyCbMap;
	std::map<GLFWwindow*, std::vector<GMouseButCallbackFun>*> mouseButCbMap;
	std::map<GLFWwindow*, std::vector<GCursorCallbackFun>*> cursorCbMap;

	bool run = false;
	bool showFps = false;
	bool multCtx = false;

	unsigned int winInd = 0;

	double medDt = 0.066;
	double lastTime = 0;
	double printFpsIntv = 2.0;
	double lastPrintFps = 0.0;
	GLFWwindow* shareCtx;
};
}

#endif /* GWindowManager_h */
