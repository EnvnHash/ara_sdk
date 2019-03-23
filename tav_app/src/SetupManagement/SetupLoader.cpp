/*
 *  SetupLoader.cpp
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 30.08.12.
 *  Copyright 2012 Sven Hahne. All rights reserved.
 *
 */

#include "SetupLoader.h"

namespace tav
{

SetupLoader::SetupLoader(char* filename, char* dataPath, bool _debug) :
		winMan(GWindowManager::shared_instance()), debug(_debug)
{
	std::locale::global(std::locale(""));
	setlocale(LC_NUMERIC, "C");

	GLFWwindow* curCtx;
	sceneData* camScd = new sceneData();
	dpStr = std::string(dataPath);
	sceneTree = new SceneNode();
	sceneTree->setName("sceneTree");
	sequencer = new Sequencer(&dpStr);
	//theOscData.seq = sequencer;

#ifdef HAVE_GSTREAMER
	// Initialize GStreamer
//	gst_init (NULL, NULL);
#endif


#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
	try
	{
#endif
	// combine filename and datapath
	std::string dp(dataPath);
	std::string fn(filename);

	xmlpp::DomParser parser(dp + fn);
	if (parser)
	{
		xmlpp::Node* root = parser.get_document()->get_root_node(); //deleted by DomParser.

		if (root)
		{
			initOpenGlCbs();
			//initFreenect(root);
			initGlfw(root);
			initGlew();
			// ab hier sollten opengl befehle gehen
			initStdQuadAndShader();
			initBoundingBoxer();
			initRoomDim(root);
			initMarks(root);
			initFboView(root);
			initMasks(root);
			initKinect(root);
			initOsc(root);
			initPaudio(root);
			initSoundFilePlayer(root);
			initTextures(root);
			initVideoTextures(root);
			initBackTex(root);
			initBackVideo(root);
			initColors(root);
			initFonts(root);
			initCamSetup(root, camScd);
			initSceneName(root);
			initSceneBlending(root);
			//initSequencer(root);

			// creat a sceneData Object
			fillSceneDataObj(&sceneD, camScd);

			// make sceneNodes
			switch (scnStructMode)
			{
			case FLAT:
				loadScenesFlat(root, camScd);
				break;
			case BLEND:
				loadScenesFlat(root, camScd);
				break;
			case NODE:
				loadScenesHierachic(root, camScd);
				break;
			}

			// handover the associative sceneNode Map to the oscHandler
			theOscData.sceneMap = &sceneMap;

			if (debug)
			{
				std::cout << "--- setup load sceneMap: " << &sceneMap << std::endl;
				getGlError();
				std::cout << "--- Scene Loading ready " << std::endl;
				getGlError();
			}

			initSequencer(root);
		}

		//	parser.get_document()->write_to_file_formatted(dp+"/output.xml");
	}

	curCtx = glfwGetCurrentContext();

#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
}
catch(const std::exception& ex)
{
	std::cout << "Exception caught: " << ex.what() << std::endl;
}
#endif
}

//---------------------------------------------------------------

void SetupLoader::initBoundingBoxer()
{
	boundBoxer = new BoundingBoxer(shaderCollector);
}

//---------------------------------------------------------------

void SetupLoader::initFreenect(xmlpp::Node* root)
{
#ifdef HAVE_FREENECT2
	if (debug)
	{
		getGlError();
		std::cout << "--- start init Freenect2" << std::endl;
	}

	xmlpp::NodeSet fncInfs = root->find("freenect2");

	if (static_cast<int>(fncInfs.size()) != 0)
	{
		fnc = new Freenect2In*[static_cast<int>(fncInfs.size())];

		std::cout << "start init freenect" << std::endl;
		std::cout << "requesting " << static_cast<int>(fncInfs.size())
				<< " freenects" << std::endl;

		short ind = 0;
		for (xmlpp::NodeSet::iterator fi = fncInfs.begin(); fi != fncInfs.end();
				++fi)
		{
			nodeElement = dynamic_cast<xmlpp::Element*>((*fi));

			if (nodeElement->get_attribute("serial") != 0)
			{
				std::cout << "" << std::endl;
				std::cout << "initing freenect instance with serial "
						<< nodeElement->get_attribute("serial")->get_value().c_str()
						<< std::endl;

				fnc[ind] =
						new Freenect2In(
								nodeElement->get_attribute("serial")->get_value().c_str());
				myNanoSleep(200000000);
				ind++;
			}
		}
		nrFnc = ind;
	}
#endif
}

//---------------------------------------------------------------

void SetupLoader::initGlfw(xmlpp::Node* root)
{
	xmlpp::NodeSet glfws = root->find("glfw");

	if (static_cast<int>(glfws.size()) == 0)
	{
		printf("Error in XML File. No <glfw> Definition!!!\n");
	}
	else
	{
		nrWins = (int) glfws.size();
		int winCnt = 0;

		for (xmlpp::NodeSet::iterator gi = glfws.begin(); gi != glfws.end(); ++gi)
		{
			int id;
			int decorated;
			int floating;
			bool bDecorated;

			nodeElement = dynamic_cast<xmlpp::Element*>((*gi));
			std::vector<std::pair<std::string, int*> > glfwAttribs;

			glfwAttribs.push_back(std::make_pair("id", &id));
			glfwAttribs.push_back(std::make_pair("width", &scrWidth));
			glfwAttribs.push_back(std::make_pair("height", &scrHeight));
			glfwAttribs.push_back(std::make_pair("offsetX", &scrXOffset));
			glfwAttribs.push_back(std::make_pair("offsetY", &scrYOffset) );
			glfwAttribs.push_back(std::make_pair("monitor", &monitor));
			glfwAttribs.push_back(std::make_pair("refreshRate", &monRefreshRate));
			glfwAttribs.push_back(std::make_pair("fullScreen", &fullScreen));
			glfwAttribs.push_back(std::make_pair("printFps", &printFps));
			glfwAttribs.push_back(std::make_pair("swapInterval", &swapInterval));
			glfwAttribs.push_back(std::make_pair("nrSamples", &scrNrSamples));
			glfwAttribs.push_back(std::make_pair("decorated", &decorated));
			glfwAttribs.push_back(std::make_pair("floating", &floating));

			for (std::vector<std::pair<std::string, int*> >::iterator it =
					glfwAttribs.begin(); it != glfwAttribs.end(); ++it)
			{
				if (nodeElement->get_attribute((*it).first) != 0)
					*((*it).second) = std::atoi(nodeElement->get_attribute((*it).first)->get_value().c_str());
				else
					printf("Error in XML File. <glfw> Definition is missing a %s attribute!!!\n", (*it).first.c_str());
			}

			bDecorated = static_cast<bool>(decorated);
			GWindow* w = winMan.addWin(scrWidth, scrHeight, monRefreshRate,
					fullScreen, true, scrXOffset, scrYOffset, monitor, bDecorated, floating,
					scrNrSamples, debug);
			w->setId(id);
			winMan.setSwapInterval(winCnt, static_cast<bool>(swapInterval));
			winMan.setPrintFps((bool) printFps);
			w->open();

			// save parameters for the first window (for the moment) for the gui
			if (gi == glfws.begin())
			{
				theOscData.ctxWidth = scrWidth;
				theOscData.ctxHeight = scrHeight;
				theOscData.ctxXpos = scrXOffset;
				theOscData.ctxYpos = scrYOffset;
			}
		}

		winCnt++;
	}

	// make first window current since it will contain all relevant data
	// to be shares with other contexts
	if (static_cast<int>(glfws.size()) != 0)
	{
		winMan.getFirstWin()->makeCurrent();

		// get a pointer to the monitor information
		monitors = winMan.getFirstWin()->getMonitors();
		nrMonitors = winMan.getFirstWin()->getNrMonitors();
		theOscData.monitors = monitors;
		theOscData.nrMonitors = nrMonitors;
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init glfw" << std::endl;
	}
}

//---------------------------------------------------------------

void SetupLoader::initGlew()
{
	// init glew
	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
		exit(EXIT_FAILURE);
	glGetError();    // delete glew standard error (bug in glew)
	printf("using GLEW %s\n", glewGetString(GLEW_VERSION));

	if (debug)
	{
		getGlError();
		std::cout << "end init glew" << std::endl;
	}
}

//---------------------------------------------------------------

void SetupLoader::initStdQuadAndShader()
{

	// standardQuads
	stdQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);

	stdHFlipQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f, nullptr, 1, true);

	// build ShaderCollector
	shaderCollector = new ShaderCollector();
}

//---------------------------------------------------------------

void SetupLoader::initRoomDim(xmlpp::Node* root)
{
	xmlpp::NodeSet roomD = root->find("roomDim");

	if (static_cast<int>(roomD.size()) != 0)
	{
		for (xmlpp::NodeSet::iterator mi = roomD.begin(); mi != roomD.end();
				++mi)
		{
			nodeElement = dynamic_cast<xmlpp::Element*>((*mi));
			std::vector<std::pair<std::string, float*> > roomDimAttribs;

			roomDimAttribs.push_back(std::make_pair("x", &roomDim.x));
			roomDimAttribs.push_back(std::make_pair("y", &roomDim.y));
			roomDimAttribs.push_back(std::make_pair("z", &roomDim.z));

			for (std::vector<std::pair<std::string, float*> >::iterator it =
					roomDimAttribs.begin(); it != roomDimAttribs.end(); ++it)
			{
				if (nodeElement->get_attribute((*it).first) != 0)
				{
					*((*it).second) =
							atof(
									nodeElement->get_attribute((*it).first)->get_value().c_str());
				}
				else
				{
					printf(
							"Error in XML File. <roomDim> Definition is missing a %s attribute!!!\n",
							(*it).first.c_str());
				}
			}
		}
	}
}

//---------------------------------------------------------------

void SetupLoader::initMarks(xmlpp::Node* root)
{
	xmlpp::NodeSet markD = root->find("mark");

	if (static_cast<int>(markD.size()) != 0)
	{
		for (xmlpp::NodeSet::iterator mi = markD.begin(); mi != markD.end();
				++mi)
		{
			nodeElement = dynamic_cast<xmlpp::Element*>((*mi));
			std::vector<std::pair<std::string, float*> > markAttribs;

			glm::vec3 readPos;
			std::string markName;

			//markAttribs.push_back( std::make_pair("name", &markName) );
			markAttribs.push_back(std::make_pair("x", &readPos.x));
			markAttribs.push_back(std::make_pair("y", &readPos.y));
			markAttribs.push_back(std::make_pair("z", &readPos.z));

			for (std::vector<std::pair<std::string, float*> >::iterator it =
					markAttribs.begin(); it != markAttribs.end(); ++it)
			{
				if (nodeElement->get_attribute((*it).first) != 0)
				{
					*((*it).second) =
							atof(
									nodeElement->get_attribute((*it).first)->get_value().c_str());
				}
				else
				{
					printf(
							"Error in XML File. <roomDim> Definition is missing a %s attribute!!!\n",
							(*it).first.c_str());
				}
			}

			// read name
			if (nodeElement->get_attribute("name") != 0)
			{
				markName =
						std::string(
								nodeElement->get_attribute("name")->get_value().c_str());

			}
			else
			{
				printf(
						"Error in XML File. <mark> Definition is missing a name attribute!!!\n");
			}

			mark[markName] = readPos;
		}
	}
}

//---- masks
void SetupLoader::initMasks(xmlpp::Node* root)
{
	xmlpp::NodeSet masks = root->find("mask");
	nrMaskQuads = (int) masks.size();

	if (static_cast<int>(masks.size()) != 0)
	{
		maskQuads = new Quad*[(int) masks.size()];

		int maskInd = 0;

		for (xmlpp::NodeSet::iterator mi = masks.begin(); mi != masks.end();
				++mi)
		{
			maskQuads[maskInd] = new Quad(-1.f, -1.f, 2.f, 2.f,
					glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 1.f);

			nodeElement = dynamic_cast<xmlpp::Element*>((*mi));
			std::vector<std::pair<std::string, float*> > maskAttribs;

			maskAttribs.push_back(std::make_pair("scaleX", &mscaleX));
			maskAttribs.push_back(std::make_pair("scaleY", &mscaleY));
			maskAttribs.push_back(std::make_pair("scaleZ", &mscaleZ));
			maskAttribs.push_back(std::make_pair("transX", &mtransX));
			maskAttribs.push_back(std::make_pair("transY", &mtransY));
			maskAttribs.push_back(std::make_pair("transZ", &mtransZ));

			for (std::vector<std::pair<std::string, float*> >::iterator it =
					maskAttribs.begin(); it != maskAttribs.end(); ++it)
			{
				if (nodeElement->get_attribute((*it).first) != 0)
				{
					*((*it).second) =
							atof(
									nodeElement->get_attribute((*it).first)->get_value().c_str());
				}
				else
				{
					printf(
							"Error in XML File. <osc> Definition is missing a %s attribute!!!\n",
							(*it).first.c_str());
				}
			}

			maskQuads[maskInd]->scale(mscaleX, mscaleY, mscaleZ);
			maskQuads[maskInd]->translate(mtransX, mtransY, mtransZ);

			maskInd++;
		}
	}
}

//---- screens or fboViews
void SetupLoader::initFboView(xmlpp::Node* root)
{
	fboViews = new std::vector<fboView*>();
	xmlpp::NodeSet screenNodes = root->find("fboView");
	for (xmlpp::NodeSet::iterator i = screenNodes.begin(); i != screenNodes.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		fboViews->push_back(new fboView());

		if (nodeElement->get_attribute("lowLeftX") != 0)
			fboViews->back()->lowLeft.x = atoi(nodeElement->get_attribute("lowLeftX")->get_value().c_str());

		if (nodeElement->get_attribute("lowLeftY") != 0)
			fboViews->back()->lowLeft.y = atoi(nodeElement->get_attribute("lowLeftY")->get_value().c_str());

		if (nodeElement->get_attribute("lowRightX") != 0)
			fboViews->back()->lowRight.x = atoi(nodeElement->get_attribute("lowRightX")->get_value().c_str());

		if (nodeElement->get_attribute("lowRightY") != 0)
			fboViews->back()->lowRight.y = atoi(nodeElement->get_attribute("lowRightY")->get_value().c_str());

		if (nodeElement->get_attribute("upRightX") != 0)
			fboViews->back()->upRight.x = atoi(nodeElement->get_attribute("upRightX")->get_value().c_str());

		if (nodeElement->get_attribute("upRightY") != 0)
			fboViews->back()->upRight.y = atoi(nodeElement->get_attribute("upRightY")->get_value().c_str());

		if (nodeElement->get_attribute("upLeftX") != 0)
			fboViews->back()->upLeft.x = atoi(nodeElement->get_attribute("upLeftX")->get_value().c_str());

		if (nodeElement->get_attribute("upLeftY") != 0)
			fboViews->back()->upLeft.y = atoi( nodeElement->get_attribute("upLeftY")->get_value().c_str());

		if (nodeElement->get_attribute("texOffsX") != 0)
			fboViews->back()->texOffs.x = atoi(nodeElement->get_attribute("texOffsX")->get_value().c_str());

		if (nodeElement->get_attribute("texOffsY") != 0)
			fboViews->back()->texOffs.y = atoi(nodeElement->get_attribute("texOffsY")->get_value().c_str());

		if (nodeElement->get_attribute("texSizeX") != 0)
			fboViews->back()->texSize.x = atoi(nodeElement->get_attribute("texSizeX")->get_value().c_str());

		if (nodeElement->get_attribute("texSizeY") != 0)
			fboViews->back()->texSize.y = atoi(nodeElement->get_attribute("texSizeY")->get_value().c_str());

		if (nodeElement->get_attribute("srcCamId") != 0)
			fboViews->back()->srcCamId = atoi(nodeElement->get_attribute("srcCamId")->get_value().c_str());

		if (nodeElement->get_attribute("dstGlfw") != 0)
			fboViews->back()->ctxId = atoi(nodeElement->get_attribute("dstGlfw")->get_value().c_str());

		fboViews->back()->update = true;
		fboViews->back()->id = static_cast<unsigned int>(fboViews->size() - 1);

		if (debug)
		{
			std::cout << "add fboview: srcCam: " << fboViews->back()->srcCamId;
			std::cout << " lowleft " << glm::to_string(fboViews->back()->lowLeft);
			std::cout << " lowRight " << glm::to_string(fboViews->back()->lowRight);
			std::cout << " upLeft " << glm::to_string(fboViews->back()->upLeft);
			std::cout << " upRight " << glm::to_string(fboViews->back()->upRight);
			std::cout << " texOffs " << glm::to_string(fboViews->back()->texOffs);
			std::cout << " texSize " << glm::to_string(fboViews->back()->texSize) << std::endl;
		}
	}

	theOscData.fboViews = fboViews;

	if (debug)
	{
		getGlError();
		std::cout << "end init viewfbo" << std::endl;
	}

}

void SetupLoader::initKinect(xmlpp::Node* root)
{
#ifdef HAVE_OPENNI2
	if (debug)
	{
		getGlError();
		std::cout << "end init ShaderCollector" << std::endl;
		std::cout << "start Kinect init" << std::endl;
	}

	//---- start kinect ---

	// init in der KinectInput Klasse
	kinMap.offset = new glm::vec2(0.f);
	kinMap.scale = new glm::vec2(1.f);
	kinMap.distScale = new glm::vec2(1.f);

	std::string kinOniFilePath;
	xmlpp::NodeSet kinInfs = root->find("kinect");

	if (static_cast<int>(kinInfs.size()) == 0)
	{
		printf("Error in XML File. No <kinect> Definition!!!\n");

	}
	else
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(kinInfs.front());
		std::vector<std::pair<std::string, bool*> > kinAttribs;

		// get booleans
		kinAttribs.push_back(std::make_pair("useKin", &kPar.activate));
		kinAttribs.push_back(std::make_pair("useNiteSkel", &kPar.useNiteSkel));
		kinAttribs.push_back(std::make_pair("useFile", &kPar.useFile));
		kinAttribs.push_back(std::make_pair("useDepth", &kPar.useDepth));
		kinAttribs.push_back(std::make_pair("useColor", &kPar.useColor));
		kinAttribs.push_back(std::make_pair("useIr", &kPar.useIr));
		kinAttribs.push_back(std::make_pair("kinAutoExp", &kPar.autoExp));
		kinAttribs.push_back(std::make_pair("kinAutoWB", &kPar.autoWB));
		kinAttribs.push_back(std::make_pair("kinEmitter", &kPar.emitter));
		kinAttribs.push_back(std::make_pair("kinCloseRange", &kPar.closeRange));
		kinAttribs.push_back(std::make_pair("kinMirror", &kPar.mirror));
		kinAttribs.push_back(
				std::make_pair("registration", &kPar.registration));

		for (std::vector<std::pair<std::string, bool*> >::iterator it =
				kinAttribs.begin(); it != kinAttribs.end(); ++it)
		{
			if (nodeElement->get_attribute((*it).first) != 0)
			{
				*((*it).second) =
						static_cast<bool>(atoi(
								nodeElement->get_attribute((*it).first)->get_value().c_str()));
			}
			else
			{
				printf(
						"Error in XML File. <kinect> Definition is missing a %s attribute!!!\n",
						(*it).first.c_str());
			}
		}

		// get ints
		std::vector<std::pair<std::string, unsigned int*> > kinIAttribs;
		kinIAttribs.push_back(std::make_pair("kinDepthW", &kPar.depthW));
		kinIAttribs.push_back(std::make_pair("kinDepthH", &kPar.depthH));
		kinIAttribs.push_back(std::make_pair("kinColorW", &kPar.colorW));
		kinIAttribs.push_back(std::make_pair("kinColorH", &kPar.colorH));
		kinIAttribs.push_back(std::make_pair("kinIrW", &kPar.irW));
		kinIAttribs.push_back(std::make_pair("kinIrH", &kPar.irH));

		for (std::vector<std::pair<std::string, unsigned int*> >::iterator it =
				kinIAttribs.begin(); it != kinIAttribs.end(); ++it)
		{
			if (nodeElement->get_attribute((*it).first) != 0)
			{
				*((*it).second) =
						static_cast<unsigned int>(atoi(
								nodeElement->get_attribute((*it).first)->get_value().c_str()));
			}
			else
			{
				printf(
						"Error in XML File. <kinect> Definition is missing a %s attribute!!!\n",
						(*it).first.c_str());
			}
		}

		// get floats
		std::vector<std::pair<std::string, float*> > kinFAttribs;
		kinFAttribs.push_back(std::make_pair("kinIrAmp", &kPar.irAmp));

		for (std::vector<std::pair<std::string, float*> >::iterator it =
				kinFAttribs.begin(); it != kinFAttribs.end(); ++it)
		{
			if (nodeElement->get_attribute((*it).first) != 0)
			{
				*((*it).second) =
						atof(
								nodeElement->get_attribute((*it).first)->get_value().c_str());
			}
			else
			{
				printf(
						"Error in XML File. <kinect> Definition is missing a %s attribute!!!\n",
						(*it).first.c_str());
			}
		}

		if (kPar.useFile)
			kinOniFilePath = dpStr.c_str()
					+ nodeElement->get_child_text()->get_content();
	}

	// hier noch optimieren, nur color oder nur depth stream
	// depth 100_UM geht nicht...
	// event based reading
	if (kPar.activate)
	{
		if (!kPar.useFile)
		{
			kin = new KinectInput(&kPar, &kinMap);
		}
		else
		{
			std::cout << "KinectInput loading: " << kinOniFilePath << std::endl;
			kin = new KinectInput(kinOniFilePath, &kPar, &kinMap);
		}

		if (kPar.useNiteSkel && kin) kin->setUpdateNis(true);

		// create repro tools
		kinRepro = new KinectReproTools(&winMan, kin, shaderCollector, scrWidth,
				scrHeight, dpStr.c_str(), 0);
		kinRepro->noUnwarp();
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init Kinect" << std::endl;
	}
#endif
}

//---------------------------------------------------------------

void SetupLoader::initOpenGlCbs()
{
	openGlCbs = new std::vector< std::function<void()> >();
}

//---- start osc ----
void SetupLoader::initOsc(xmlpp::Node* root)
{
	xmlpp::NodeSet oscInfs = root->find("osc");

	if (static_cast<int>(oscInfs.size()) == 0)
	{
		printf("Error in XML File. No <osc> Definition!!!\n");
	}
	else
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(oscInfs.front());
		std::vector<std::pair<std::string, std::string*> > oscAttribs;

		oscAttribs.push_back(std::make_pair("portNr", &oscPortNr));

		for (std::vector<std::pair<std::string, std::string*> >::iterator it =
				oscAttribs.begin(); it != oscAttribs.end(); ++it)
		{
			if (nodeElement->get_attribute((*it).first) != 0)
			{
				*((*it).second) =
						nodeElement->get_attribute((*it).first)->get_value();
			}
			else
			{
				printf(
						"Error in XML File. <osc> Definition is missing a %s attribute!!!\n",
						(*it).first.c_str());
			}
		}
	}

	theOscData.initData();
	osc_handler = new OSCHandler(oscPortNr);	// initialise osc

	if (debug)
	{
		getGlError();
		std::cout << "end init OSC" << std::endl;
	}
}

//---------------------------------------------------------------

void SetupLoader::initPaudio(xmlpp::Node* root)
{
#ifdef WITH_AUDIO
	//---- start paudio

	xmlpp::NodeSet paudios = root->find("paudio");

	if (static_cast<int>(paudios.size()) != 0)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(paudios.front());
		std::vector<std::pair<std::string, int*> > paudioAttribs;

		paudioAttribs.push_back(std::make_pair("samplerate", &sampleRate));
		paudioAttribs.push_back(std::make_pair("maxNrChans", &maxNrChans));

		for (std::vector<std::pair<std::string, int*> >::iterator it =
				paudioAttribs.begin(); it != paudioAttribs.end(); ++it)
		{
			if (nodeElement->get_attribute((*it).first) != 0)
			{
				*((*it).second) = atoi(nodeElement->get_attribute((*it).first)->get_value().c_str());
			}
			else
			{
				printf("Error in XML File. <paudio> Definition is missing a %s attribute!!!\n",
						(*it).first.c_str());
			}
		}

		// start paudio
		frameSize = 256; // muss dasselbe wie in supercollider sein, sonst dropouts!!!!!
		int fftSize = 1024;
		int rec_buf_size = fftSize / frameSize;

		paudioIsReady = false;
		pa = new PAudio(&paMutex, &paudioIsReady, frameSize, rec_buf_size,
				fftSize, sampleRate, monRefreshRate, maxNrChans);
		pa->start();

		// wait until paudio is set
		while (!paudioIsReady)
			myNanoSleep(1000000);

		std::cout << "nrChannels: " << pa->getMaxNrInChannels()
				<< " sampleRate:" << sampleRate << " frameSize:" << frameSize
				<< " rec_buf_size:" << rec_buf_size << " fftSize:"
				<< frameSize * rec_buf_size << " monRefreshRate: "
				<< monRefreshRate << std::endl;
		std::cout
				<< "frameSize must be the same as all other Applications using the same interface at the same time"
				<< std::endl;
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init paudio" << std::endl;
	}

	// -- Init Audio Textures for all GPU stuff -

	if (paudioIsReady)
		audioTex = new AudioTexture(shaderCollector, pa->getMaxNrInChannels(), frameSize);
#endif
}

//---------------------------------------------------------------

void SetupLoader::initSoundFilePlayer(xmlpp::Node* root)
{
#ifdef WITH_AUDIO

	xmlpp::NodeSet sndfileTag = root->find("sndfile");

	nrSoundFileLayers=0;

	if (static_cast<int>(sndfileTag.size()) != 0)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(sndfileTag.front());
			std::vector<std::pair<std::string, int*> > sndfileAttribs;

		sndfileAttribs.push_back(std::make_pair("nrLayers", &nrSoundFileLayers));

		for (std::vector<std::pair<std::string, int*> >::iterator it =
				sndfileAttribs.begin(); it != sndfileAttribs.end(); ++it)
		{
			if (nodeElement->get_attribute((*it).first) != 0)
				*((*it).second) = atoi(nodeElement->get_attribute((*it).first)->get_value().c_str());
			else
				printf("Error in XML File. <paudio> Definition is missing a %s attribute!!!\n",
						(*it).first.c_str());
		}
	}

	if(nrSoundFileLayers > 0)
	{
		soundFilePlayer = new SoundFilePlayer*[nrSoundFileLayers];

		for (int i=0; i<nrSoundFileLayers; i++)
		{
			soundFilePlayer[i] = new SoundFilePlayer(frameSize, pa->getOutNrChannels());

			pa->addSndFilePlayerCb(std::bind(&SoundFilePlayer::pa_play_cb, soundFilePlayer[i],
					std::placeholders::_1, std::placeholders::_2,  std::placeholders::_3));

			pa->addSndFilePlayerSampCb(std::bind(&SoundFilePlayer::pa_play_samp_cb, soundFilePlayer[i],
					std::placeholders::_1, std::placeholders::_2));

		}
	}
#endif
}

//---------------------------------------------------------------

void SetupLoader::initTextures(xmlpp::Node* root)
{
	// load textures
	xmlpp::NodeSet texts = root->find("texture");
	int texNr = 0;
	textures = new GLuint[static_cast<int>(texts.size())];
	texObjs = new TextureManager*[static_cast<int>(texts.size())];

	// each texture
	for (xmlpp::NodeSet::iterator i = texts.begin(); i != texts.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		texObjs[texNr] = new TextureManager();
		textures[texNr] = texObjs[texNr]->loadTexture2D(dpStr + nodeElement->get_child_text()->get_content());
		//                        textures[texNr] = tav::TextureManager::Inst()->loadTexture2D(dataPath+nodeElement->get_child_text()->get_content());
		texPaths.push_back(	std::string(nodeElement->get_child_text()->get_content().c_str()));
		texNr++;
	}

	nrTextures = texNr;
}

//---------------------------------------------------------------

void SetupLoader::initVideoTextures(xmlpp::Node* root)
{
#ifdef HAVE_OPENCV
	// load videoTextures
	xmlpp::NodeSet vtexts = root->find("videotexture");
	nrVTexts = static_cast<int>(vtexts.size());
	videoTextures = new VideoTextureCv*[nrVTexts];
	if(debug) std::cout << "nr video textures: " << nrVTexts << std::endl;
#ifdef USE_ACTRANGE_TRACKING
	videoTextsRange = new VideoTextureCvActRange*[nrVTexts];
#endif
	// each videotexture
	for (xmlpp::NodeSet::iterator i = vtexts.begin(); i != vtexts.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		std::cout << "load video texture: "
				<< (char*) (dpStr + nodeElement->get_child_text()->get_content()).c_str()
				<< std::endl;
		videoTextures[i - vtexts.begin()] =	new VideoTextureCv((char*) (dpStr	+ nodeElement->get_child_text()->get_content()).c_str());
#ifdef USE_ACTRANGE_TRACKING
		videoTextsRange[i - vtexts.begin()] = new VideoTextureCvActRange(shaderCollector, videoTextures[i - vtexts.begin()]);
#endif
		videoTexPaths.push_back(std::string((dpStr + nodeElement->get_child_text()->get_content()).c_str()));
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init textures" << std::endl;
	}
#endif
}

//---------------------------------------------------------------

void SetupLoader::initBackTex(xmlpp::Node* root)
{
	xmlpp::NodeSet backgr = root->find("background");
	for (xmlpp::NodeSet::iterator i = backgr.begin(); i != backgr.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		backTex = new TextureManager();
		backTex->loadTexture2D(
				dpStr + nodeElement->get_child_text()->get_content());
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init background" << std::endl;
	}

}

//---------------------------------------------------------------

void SetupLoader::initBackVideo(xmlpp::Node* root)
{
	xmlpp::NodeSet backgrV = root->find("backvideo");
	for (xmlpp::NodeSet::iterator i = backgrV.begin(); i != backgrV.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		std::cout << "found background video: "
				<< nodeElement->get_child_text()->get_content() << std::endl;

		backVidPath = dpStr + nodeElement->get_child_text()->get_content();
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init background video" << std::endl;
	}
}

//---------------------------------------------------------------

void SetupLoader::initColors(xmlpp::Node* root)
{
	// load colors
	xmlpp::NodeSet cols = root->find("color");
	int colNr = 0;
	colors = new glm::vec4[static_cast<int>(cols.size())];

	// each color
	for (xmlpp::NodeSet::iterator i = cols.begin(); i != cols.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		for (auto j = 0; j < 4; j++)
			if (nodeElement->get_attribute("c" + std::to_string(j)) != 0)
				colors[colNr][j] = atof( nodeElement->get_attribute("c" + std::to_string(j))->get_value().c_str()) / 255.f;

		colNr++;
	}
}

//---------------------------------------------------------------

void SetupLoader::initFonts(xmlpp::Node* root)
{
	// load fonts
	xmlpp::NodeSet font = root->find("font");
	nrFonts = static_cast<unsigned int>(font.size());

	// if there is a font init freetype
	if (nrFonts != 0)
	{
		ft_library = new FT_Library();
		FT_Error err = FT_Init_FreeType(ft_library);
		assert(!err);
		ft_fonts = new FreeTypeFont*[nrFonts];
	}

	for (xmlpp::NodeSet::iterator i = font.begin(); i != font.end(); ++i)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*i);
		ft_fonts[i - font.begin()] = new FreeTypeFont(ft_library,
				(dpStr + nodeElement->get_child_text()->get_content()).c_str(),
				font.begin() - i);
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init fonts" << std::endl;
	}
}

//---------------------------------------------------------------

void SetupLoader::initCamSetup(xmlpp::Node* root, sceneData* camScd)
{
	// load camera
	fillSceneDataObj(camScd, nullptr);

	xmlpp::NodeSet camNode = root->find("camera");
	if ((int) camNode.size() == 0)
		printf("SetupManagement Error: No Camera set in XML-Config File!!!\n");

	for (xmlpp::NodeSet::iterator it = camNode.begin(); it != camNode.end(); ++it)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(*it);

		if (nodeElement->get_attribute("id") != 0)
			camScd->id = atoi( nodeElement->get_attribute("id")->get_value().c_str());

		camScd->camPos = glm::vec3(0.f, 0.f, 1.f);
		if (nodeElement->get_attribute("posX") != 0)
			camScd->camPos.x = atof( nodeElement->get_attribute("posX")->get_value().c_str());
		if (nodeElement->get_attribute("posY") != 0)
			camScd->camPos.y = atof(nodeElement->get_attribute("posY")->get_value().c_str());
		if (nodeElement->get_attribute("posZ") != 0)
			camScd->camPos.z = atof(nodeElement->get_attribute("posZ")->get_value().c_str());

		camScd->camLookAt = glm::vec3(0.f);
		if (nodeElement->get_attribute("lookAtX") != 0)
			camScd->camLookAt.x = atof(nodeElement->get_attribute("lookAtX")->get_value().c_str());
		if (nodeElement->get_attribute("lookAtY") != 0)
			camScd->camLookAt.y = atof(nodeElement->get_attribute("lookAtY")->get_value().c_str());
		if (nodeElement->get_attribute("lookAtZ") != 0)
			camScd->camLookAt.z = atof(nodeElement->get_attribute("lookAtZ")->get_value().c_str());

		camScd->camUpVec = glm::vec3(0.f);
		if (nodeElement->get_attribute("upVecX") != 0)
			camScd->camUpVec.x = atof(nodeElement->get_attribute("upVecX")->get_value().c_str());
		if (nodeElement->get_attribute("upVecY") != 0)
			camScd->camUpVec.y = atof(nodeElement->get_attribute("upVecY")->get_value().c_str());
		if (nodeElement->get_attribute("upVecZ") != 0)
			camScd->camUpVec.z = atof(nodeElement->get_attribute("upVecZ")->get_value().c_str());

		if (nodeElement->get_attribute("fboWidth") != 0)
			camScd->fboWidth = atoi(nodeElement->get_attribute("fboWidth")->get_value().c_str());
		else camScd->fboWidth = scrWidth;

		if (nodeElement->get_attribute("fboHeight") != 0)
			camScd->fboHeight = atoi(nodeElement->get_attribute("fboHeight")->get_value().c_str());
		else camScd->fboHeight = scrHeight;

		theOscData.fboWidth = camScd->fboWidth;
		theOscData.fboHeight = camScd->fboHeight;

		CameraFact* csf = new CameraFact(nodeElement->get_child_text()->get_content().c_str());
		cam.push_back(csf->Create(camScd, &theOscData, osc_handler, &winMan, fboViews));
	}

	if (debug)
	{
		getGlError();
		std::cout << "end init camera setup" << std::endl;
	}
}

//---- scene name
void SetupLoader::initSceneName(xmlpp::Node* root)
{
	xmlpp::NodeSet scname = root->find("name");
	if (static_cast<int>(scname.size()) == 0)
	{
		printf("Error in XML File. No <name> Definition!!!\n");
	}
	else
	{
		for (xmlpp::NodeSet::iterator gi = scname.begin(); gi != scname.end();
				++gi)
		{
			nodeElement = dynamic_cast<xmlpp::Element*>((*gi));
			setupName = std::string(
					nodeElement->get_child_text()->get_content());
		}
	}
}

//---------------------------------------------------------------

void SetupLoader::initSceneBlending(xmlpp::Node* root)
{
	//---- blending name

	xmlpp::NodeSet scBlend = root->find("sceneBlending");
	if (static_cast<int>(scBlend.size()) == 0)
	{
		printf("Error in XML File. No <sceneBlending> Definition!!!\n");
	}
	else
	{
		for (xmlpp::NodeSet::iterator gi = scBlend.begin(); gi != scBlend.end();
				++gi)
		{
			nodeElement = dynamic_cast<xmlpp::Element*>((*gi));
			switch (atoi(nodeElement->get_child_text()->get_content().c_str()))
			{
			case 0:
				scnStructMode = FLAT;
				break;
			case 1:
				scnStructMode = BLEND;
				break;
			case 2:
				scnStructMode = NODE;
				break;
			}
		}
	}
}

//---------------------------------------------------------------

void SetupLoader::initSequencer(xmlpp::Node* root)
{
	// check for sequencer node
	xmlpp::NodeSet seqNode = root->find("sequencer");
	if (static_cast<int>(seqNode.size()) != 0)
	{
		nodeElement = dynamic_cast<xmlpp::Element*>(seqNode[0]);
		if (debug)
			std::cout << "Sequencer loading file: "
					<< nodeElement->get_child_text()->get_content().c_str()
					<< std::endl;
		sequencer->loadXml(
				nodeElement->get_child_text()->get_content().c_str());

		/*
		 xmlpp::NodeSet events = seqNode[0]->find("event");
		 for(xmlpp::NodeSet::iterator evt = events.begin(); evt != events.end(); ++evt)
		 {
		 SceneNode* _scene=0;
		 double _startTime=0;
		 double _dur=0;
		 nodeElement = dynamic_cast<xmlpp::Element*>((*evt));

		 // get SceneNode
		 if ( nodeElement->get_attribute("scene") != 0 )
		 {
		 // search for scene Argument
		 const char* getScene = nodeElement->get_attribute("scene")->get_value().c_str();
		 for ( std::vector<SceneNode*>::iterator it = sceneNodes.begin(); it != sceneNodes.end(); ++it )
		 if ( std::strcmp( (*it)->getName().c_str(), getScene ) == 0 )
		 _scene = (*it);
		 } else {
		 std::cerr << "SetupLoader Error: event is missing a scene argument" << std::endl;
		 }

		 if ( nodeElement->get_attribute("startTime") != 0 )
		 _startTime = atof( nodeElement->get_attribute("startTime")->get_value().c_str() );

		 if ( nodeElement->get_attribute("startTime") != 0 )
		 _dur = atof( nodeElement->get_attribute("duration")->get_value().c_str() );

		 std::cout << "loading event scene " << _scene << " start " << _startTime << " dur " << _dur << std::endl;

		 seqEvents.push_back( new SeqEvent(_startTime, _dur, _scene) );
		 sequencer->addEvent(_startTime, seqEvents.back());
		 }
		 */
	}
}

//---------------------------------------------------------------

void SetupLoader::fillSceneDataObj(sceneData* scd, sceneData* camScd)
{
	// prepare taMod for scene
	//	scd.push_back(new sceneData());
	scd->screenWidth = scrWidth;
	scd->screenHeight = scrHeight;

	if (camScd)
	{
		scd->fboWidth = camScd->fboWidth;
		scd->fboHeight = camScd->fboHeight;
	}

	scd->frameSize = frameSize;
	scd->dataPath = &dpStr;
	scd->nrTextures = nrTextures;
	scd->nrSoundFileLayers = nrSoundFileLayers;
	scd->shaderCollector = shaderCollector;
	scd->textures = textures;
	scd->mark = &mark;
	scd->monRefDt = 1.0 / static_cast<double>(monRefreshRate);
	scd->monRefRate = monRefreshRate;
	scd->nrChannels = maxNrChans;
	scd->nrSamples = scrNrSamples;
	scd->scnStructMode = scnStructMode;
	scd->tex = textures;
	//scd->texNrs = new int[MAX_NUM_SIM_TEXS];
	scd->roomDim = &roomDim;
	scd->setupName = setupName;
	scd->backTex = backTex;
	scd->backVidPath = &backVidPath;
	scd->stdQuad = stdQuad;
	scd->stdHFlipQuad = stdHFlipQuad;

//	for (auto i=0;i<MAX_NUM_SIM_TEXS;i++) {
//		scd->tex[i] = 0;
//		scd->texNrs[i] = 0;
//	}

	// manuell, später per xml
	scd->colors = colors;
//	chanCols = new glm::vec4[MAX_NUM_COL_SCENE];

	// camera parameters
	// braucht man die wirklich?
	if (camScd)
	{
		scd->camPos = camScd->camPos;
		scd->camLookAt = camScd->camLookAt;
		scd->camUpVec = camScd->camUpVec;
	}

	scd->boundBoxer = static_cast<void*>(boundBoxer);
	scd->fboViews = static_cast<void*>(fboViews);
	scd->ft_lib = static_cast<void*>(ft_library);
	scd->ft_fonts = static_cast<void*>(ft_fonts);
#ifdef HAVE_FREENECT2
	scd->fnc = static_cast<void*>(fnc);
	scd->nrFnc = nrFnc;
#endif
#ifdef HAVE_OPENNI2
	scd->kin = static_cast<void*>(kin);
	scd->kinMapping = static_cast<void*>(&kinMap);
	scd->kinRepro = static_cast<void*>(kinRepro);
#endif
	scd->nrVideoTex = nrVTexts;
	scd->openGlCbs = static_cast<void*>(openGlCbs);
	scd->osc = static_cast<void*>(&theOscData);
	scd->oscHandler = static_cast<void*>(osc_handler);
#ifdef WITH_AUDIO
	scd->audioTex = audioTex;
	scd->pa = static_cast<void*>(pa);
	scd->soundFilePlayer = static_cast<void*>(soundFilePlayer);
#endif
	scd->texObjs = static_cast<void*>(texObjs);
#ifdef HAVE_OPENCV
	scd->videoTextures = static_cast<void*>(videoTextures);
#ifdef USE_ACTRANGE_TRACKING
	scd->videoTextsActRange = static_cast<void*>(videoTextsRange);
#endif
#endif
	scd->winMan = static_cast<void*>(&winMan);
}

//---------------------------------------------------------------

void SetupLoader::loadScenesFlat(xmlpp::Node* root, sceneData* camScd)
{
	// search for the beginning of the tree
	xmlpp::NodeSet xmlSceneNodes = root->find("//scene");

	// each scene
	for (xmlpp::NodeSet::iterator itr = xmlSceneNodes.begin();
			itr != xmlSceneNodes.end(); ++itr)
	{
		if (const xmlpp::Element* el = dynamic_cast<const xmlpp::Element*>(*itr))
		{
			const char* nodeAttrName;
			const xmlpp::Element::AttributeList& attributes = el->get_attributes();
			sceneArgs.push_back(std::map<std::string, float>());

			if (attributes.size() != 0)
			{
				nodeAttrName = static_cast<const char*>(el->get_attribute_value("name").c_str());
				//std::cout << "name: " << nodeAttrName << std::endl;

				for (std::list<xmlpp::Attribute*>::const_iterator argName = attributes.begin();
						argName != attributes.end(); ++argName)
				{
				//	std::cout << "got argument " << static_cast<std::string>((*argName)->get_name()) << " with val: " << std::atof( (*argName)->get_value().c_str() ) << std::endl;
					sceneArgs.back()[static_cast<std::string>((*argName)->get_name())] = std::atof((*argName)->get_value().c_str());
				}

			}
			else
			{
				std::cerr << "SetupLoader Error, <scene> needs an unique name attribute!!!" << std::endl;
			}

			if (static_cast<unsigned int>(el->get_children().size()) == 1)
			{
				SceneNodeFact* scF = new SceneNodeFact(el->get_child_text()->get_content().c_str());
				sceneNodes.push_back( scF->Create(&sceneD, &sceneArgs.at(int(sceneArgs.size() - 1))));
				sceneNodes.back()->name = nodeAttrName;
				sceneMap[nodeAttrName] = sceneNodes.back();
			}
		}
	}

	nrSceneNodes = static_cast<int>(sceneNodes.size());
}

//---------------------------------------------------------------

void SetupLoader::loadScenesHierachic(xmlpp::Node* root, sceneData* camScd)
{
	xmlpp::NodeSet sroot = root->find("sceneroot");

	if (static_cast<unsigned int>(sroot.size()) != 0)
	{
		iterateNode(sroot[0], sceneTree, camScd); // start iteration
	}
	else
	{
		printf("SetupLoader::loadScenesHierachic Error: No <sceneroot> found \n");
	}

	nrSceneNodes = 0;
}

//---------------------------------------------------------------

void SetupLoader::iterateNode(xmlpp::Node* node, SceneNode* scRoot, sceneData* camScd)
{
	const char* nodeAttrName;
	const char* nodeName = static_cast<const char*>(node->get_name().c_str());

	if (debug) std::cout << std::endl;
	if (debug) std::cout << "nodeName: " << nodeName << std::endl;
	if (debug) std::cout << "scRoot name: " << scRoot->getName() << std::endl;

	xmlpp::Node::NodeList nodeList = node->get_children();
	for (xmlpp::Node::NodeList::iterator iter = nodeList.begin();
			iter != nodeList.end(); ++iter)
	{
		const char* thisNodeName = static_cast<const char*>((*iter)->get_name().c_str());

		// omit text nodes
		if (std::strcmp(thisNodeName, "scene") == 0)
		{
			if (debug)
				std::cout << "thisNodeName: " << thisNodeName << std::endl;

			if (const xmlpp::Element* scn = static_cast<const xmlpp::Element*>((*iter)))
			{
				if (scn->get_attribute("name") != 0)
				{
					const char* an = scn->get_attribute("name")->get_value().c_str();

					// make a copy that it doens't get overwritten
					std::string nodeAttrName = static_cast<std::string>(an);
					if (debug) std::cout << "nodeAttrName: " << nodeAttrName << std::endl;

					// if has children iterate
					if ((*iter)->get_children().size() != 1)
					{
						sceneNodes.push_back(new SceneNode());
						sceneNodes.back()->setName(nodeAttrName);

						if (debug) std::cout << "sceneNodes.back()->name: " << sceneNodes.back()->name << std::endl;
						if (debug) std::cout << "add to scene: " << scRoot->getName() << std::endl;

						sceneNodes.back()->_active = getSingleXmlFloat(scn,	(char*) "active");
						sceneMap[nodeAttrName] = sceneNodes.back();

						iterateNode((*iter), scRoot->addChild(sceneNodes.back()), camScd); //calls this method for all the children which is Element
					}
					else
					{
						const xmlpp::Element::AttributeList& attributes = scn->get_attributes();
						sceneArgs.push_back(std::map<std::string, float>());

						for (std::list<xmlpp::Attribute*>::const_iterator argName = attributes.begin();
								argName != attributes.end(); ++argName)
						{
							if (debug) std::cout << "got argument "
										<< static_cast<std::string>((*argName)->get_name())
										<< " with val: "
										<< std::atof((*argName)->get_value().c_str())
										<< std::endl;

							sceneArgs.back()[static_cast<std::string>((*argName)->get_name())] =
									std::atof((*argName)->get_value().c_str());

							//if (debug){
							//	std::cout << "sceneArgs.back().size() " << sceneArgs.back().size();
							//	std::cout << ", sceneArgs.back()[" << static_cast<std::string>((*argName)->get_name()) << "] = ";
							//	std::cout << std::atof((*argName)->get_value().c_str()) << std::endl;
							//}
						}
						if (debug) std::cout << "alle argumente gemappt!! " << std::endl;

						SceneNodeFact* scF = new SceneNodeFact( scn->get_child_text()->get_content().c_str());
						sceneNodes.push_back( scF->Create(&sceneD, &sceneArgs.back()));
						sceneNodes.back()->_active = getSingleXmlFloat(scn, (char*) "active");
						sceneNodes.back()->setName(nodeAttrName);

						if (debug)
							std::cout << "sceneNodes.back()->name: " << sceneNodes.back()->name << std::endl;
						if (debug)
							std::cout << "add to Scene with name: " << nodeAttrName << std::endl;

						sceneMap[nodeAttrName] = sceneNodes.back();	// add group

						scRoot->addChild(sceneNodes.back());	// add group
					}
				}
				else
				{
					std::cerr
							<< "SetupLoader Error, <scene> not loaded, needs an unique name attribute!!!"
							<< std::endl;
				}
			}
		}
	}
}

//---------------------------------------------------------------

inline float SetupLoader::getSingleXmlFloat(const xmlpp::Element* el,
		std::string name)
{
	float out = 0;
	if (el->get_attribute(name.c_str()) != 0)
		out = atof(el->get_attribute(name.c_str())->get_value().c_str());
	return out;
}

//---------------------------------------------------------------

SetupLoader::~SetupLoader()
{
	/*
	FT_Done_FreeType(*ft_library);

	delete stdQuad;
	delete stdHFlipQuad;
#ifdef WITH_AUDIO
	delete pa;
#endif
#ifdef HAVE_OPENCV
	delete[] videoTextures;
#endif
#ifdef HAVE_OPENNI2
	delete kin;
#endif
#ifdef HAVE_FREENECT2
	delete fnc;
#endif
	delete[] textures;
	delete[] texObjs;
	delete openGlCbs;
	*/
}

//---------------------------------------------------------------

void SetupLoader::close()
{
#ifdef HAVE_FREENECT2
	if (fnc)
		for (int i = 0; i < nrFnc; i++)
			fnc[i]->stop();
#endif

#ifdef HAVE_OPENNI2
	if (kPar.activate)
		kin->shutDown();
#endif

	osc_handler->stop();

	//        for(auto i=0;i<nrSceneNodes;i++)
	//            sceneNodes[i]->cleanUp();

	for (auto i = 0; i < nrTextures; i++)
		texObjs[i]->releaseTexture();

	// destroy opengl context
	winMan.closeAll();
}

//---------------------------------------------------------------

OSCData* SetupLoader::getOscData()
{
	return &theOscData;
}

//---------------------------------------------------------------

int SetupLoader::getNrSceneNodes()
{
	return nrSceneNodes;
}

//---------------------------------------------------------------

sceneData* SetupLoader::getSceneData(int ind)
{
	return &sceneD;
}

//---------------------------------------------------------------

std::vector<CameraSet*>* SetupLoader::getCamera()
{
	return &cam;
}

//---------------------------------------------------------------

GWindowManager* SetupLoader::getWinMan()
{
	return &winMan;
}

//---------------------------------------------------------------

unsigned int SetupLoader::getNrGlfwWin()
{
	return nrWins;
}

//---------------------------------------------------------------

bool SetupLoader::getPrintFps()
{
	return (bool) printFps;
}

//---------------------------------------------------------------

bool SetupLoader::useKin()
{
#ifdef HAVE_OPENNI2
	return kPar.activate;
#else
	return false;
#endif
}

//---------------------------------------------------------------

bool SetupLoader::to_bool(std::string const& s)
{
	return s != "0";
}

//---------------------------------------------------------------

void SetupLoader::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long) ns;
	nanosleep(&tim, NULL);
}

}
