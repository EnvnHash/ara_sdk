//
// SNGam_Libro.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNGam_Libro.h"

#define STRINGIFY(A) #A

using namespace std::chrono_literals;

namespace tav
{

SNGam_Libro::SNGam_Libro(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), tx(0.f), ty(0.f), tz(0.f), rx(0.f), ry(0.f), rz(0.f),
		fadeInTime(2.0), pageBuildUpTime(2.0), pageFadeTime(1.0), turnPageTime(3.0),
		actDrawMode(IDLE_BLACK), blockInteraction(false), nrLevels(3), nrPages(17),
		actPage(12), fadeInThread(NULL), requestFadeIn(false), dstWidth(1920), dstHeight(1080),
		nrDecodeThreads(5), decodeYuv420OnGpu(false), request(TR_NONE)
{
	shCol = (ShaderCollector*)_scd->shaderCollector;
	winMan = static_cast<GWindowManager*>(scd->winMan);
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });
	winMan->addCursorCallback(0, [this](double xPos, double yPos){
		return this->onCursor(xPos, yPos); });
	winMan->addMouseButCallback(0, [this](int button, int action, int mods, double xPos, double yPos){
		return this->onMouseButton( button, action, mods); });

#ifndef SNGAMLIBRO_USE_FFMPEG
	vts = static_cast<VideoTextureCv**>(scd->videoTextures);
#endif

    scAddr = lo_address_new("127.0.0.1", "57120");

	initCurveData();
	initPageData();

	mouseX = new Median<float>(3.f);

	// Textures
	gui_tex = new TextureManager();
	gui_tex->loadTexture2D((*_scd->dataPath)+"/textures/gam_mustakis/libro_gui.png");

	// load the model of the book to map
	modelRoot = new SceneNode();
	modelRoot->setName("SNGam_Libro_model_root");
	modelRoot->setActive(false);

	// mapping parameters
	aspectRatio = static_cast<float>(_scd->screenWidth) / static_cast<float>(_scd->screenHeight);
	near = 1.f;
	far = 1000.f;
	glm::vec3 eye = glm::vec3(0.f, 0.f, 1.f);
	glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

	projMat = glm::mat4(1.f);
	viewMat = glm::lookAt(eye, center, up);

	// Epson Home Cinema 1060
	// f 1.58 - 1.70
	// focal length 18.2 - 29.2
	// zoom ratio, 1.0 - 1.2

	// zoom ratio = d / b,
	// d = zoom ratio * b
	// fov = std::atan2(b/2, d);
	// fov = std::atan2(b/2, zoom ratio * b)
	// bei b=1 fov = std::atan2(0.5, zoom ratio);
	zoomRatio = 1.0;


	// AssimpImport creates a attaches the imported meshes as new nodes to the node given as
	// the second argument, libro.obj should have only one subnode, extract this after importing
	aImport = new AssimpImport(_scd, true);
	aImport->load(((*scd->dataPath)+"models/gam_mustakis/libro.obj").c_str(), modelRoot, [this](){

		std::vector<SceneNode*>* children = modelRoot->getChildren();
		if(children->size() > 0) modelNode = children->at(0);

		// calibration i/o
		calibFileName = (*scd->dataPath)+"calib_cam/gam_libro.yml";

		// check if calibration file exists, if this is the case load it
		if (access(calibFileName.c_str(), F_OK) != -1)
			loadCalib();

		requestFadeIn = true;
	});

	// Geometry
	rawquad = _scd->stdQuad;

	// osc parameters
	addPar("tx", &tx);
	addPar("ty", &ty);
	addPar("tz", &tz);
	addPar("rx", &rx);
	addPar("ry", &ry);
	addPar("rz", &rz);
	addPar("sx", &sx);
	addPar("sy", &sy);
	addPar("sz", &sz);
	addPar("borderSizeX", &borderSizeX);
	addPar("borderSizeY", &borderSizeY);
	addPar("yDistAmt", &yDistAmt);
	addPar("zoomRatio", &zoomRatio);

	addPar("nrQuadsX", &nrQuadsX);
	addPar("nrQuadsY", &nrQuadsY);

	// animation values
	opacity = new AnimVal<float>*[nrLevels];
	for (int i=0; i<nrLevels; i++){
		opacity[i] = new AnimVal<float>(RAMP_LIN_UP, [this](){});
		opacity[i]->setInitVal(0.f);
	}

	// init shaders
	initShader();
	stdTex = shCol->getStdTex();
}

//----------------------------------------------------

void SNGam_Libro::initCurveData()
{
	// curve sicht von vorne, linke seite, ursprung oben links
	unsigned int nrCurvePoints = 11;
	glm::ivec2 curveLeft[nrCurvePoints] = {
			glm::ivec2(935,998),
			glm::ivec2(1022,981),
			glm::ivec2(1108,966),
			glm::ivec2(1195,955),
			glm::ivec2(1283,945),
			glm::ivec2(1367,935),
			glm::ivec2(1452,930),
			glm::ivec2(1542,930),
			glm::ivec2(1628,935),
			glm::ivec2(1716,948),
			glm::ivec2(1804,964)
	};

	//normalize curve and flip y axis
	glm::vec2 curveLeftFloat[nrCurvePoints];

	for (unsigned int i=0; i<nrCurvePoints; i++)
		curveLeftFloat[i] = glm::vec2(
				((float)curveLeft[i].x - (float)curveLeft[0].x) / ((float)curveLeft[nrCurvePoints-1].x - (float)curveLeft[0].x),
				1.f - (((float)curveLeft[i].y - (float)curveLeft[6].y) / ((float)curveLeft[0].y - (float)curveLeft[6].y)));

	// copy mirrored curve to the right and multiply x * 0.5
	nrCurveSymPoints = nrCurvePoints * 2 -1;
	curveSymetric = new glm::vec2[nrCurveSymPoints];

	for (unsigned int i=0; i<nrCurvePoints; i++)
		curveSymetric[i] = curveLeftFloat[i];

	for (unsigned int i=0; i<nrCurvePoints-1; i++)
		curveSymetric[i +nrCurvePoints] = curveLeftFloat[nrCurvePoints -i -2];

	for (unsigned int i=0; i<nrCurveSymPoints; i++)
		curveSymetric[i].x = static_cast<float>(i) / static_cast<float>(nrCurveSymPoints-1);


	quadArray = new QuadArray(nrCurveSymPoints-1, 1);
}

//----------------------------------------------------

void SNGam_Libro::initPageData()
{
	buttPos = new glm::vec2*[2];
	for (unsigned int i=0; i<2; i++)
		buttPos[i] = new glm::vec2[2];

	// butt[0] = left butt[1] = right
	// [0] lowleft [1] top right

	buttPos[0][0] = glm::vec2(58.f, 138.f);
	buttPos[0][1] = glm::vec2(378.f, 460.f);
	buttPos[1][0] = glm::vec2(642.f, 138.f);
	buttPos[1][1] = glm::vec2(960.f, 454.f);

	glm::vec2 smallScreenDim = glm::vec2(1024.f, 600.f);

	for (unsigned int i=0; i<2; i++)
		for (unsigned int j=0; j<2; j++)
			buttPos[i][j] = glm::vec2(
					buttPos[i][j].x / smallScreenDim.x,
					buttPos[i][j].y / smallScreenDim.y);


	pages = new pageElement[nrPages];

	pages[0].intro = "01portadaintro.mp4";
	pages[0].intro_audio = "01_Portada_Intro.wav";
	pages[0].dur_intro = 9.07;
	pages[0].loop = "01PORTADALOOP.m4v";
	pages[0].loop_audio = "01_Portada_Loop.wav";

	pages[1].intro = "02BiografiaINTRO2.mp4";
	pages[1].intro_audio = "02_Biografia_Intro.wav";
	pages[1].dur_intro = 6.00;
	pages[1].loop = "02BiografiaLOOP.m4v";
	pages[1].loop_audio = "02_Biografia_Loop.wav";

	pages[2].intro = "03CAPITULO1INTRO2.m4v";
	pages[2].intro_audio = "03_Capitulo_1_Intro.wav";
	pages[2].dur_intro = 6.50;
	pages[2].loop = "03CAPITULO1LOOP.m4v";
	pages[2].loop_audio = "03_Capitulo_1_Loop.wav";

	pages[3].intro = "04_puntarenas_intro.mp4";
	pages[3].intro_audio = "04_Pta_Arenas_Intro.wav";
	pages[3].dur_intro = 4.84;
	pages[3].loop = "04puntarenasLOOP.m4v";
	pages[3].loop_audio = "04_Pta_Arenas_Loop.wav";

	pages[4].intro = "05Santiago.mp4";
	pages[4].intro_audio = "05_Santiago_Intro.wav";
	pages[4].dur_intro = 37.33;
	pages[4].loop = "05Santiago_loop.mp4";
	pages[4].loop_audio = "05_Santiago_Loop.wav";

	pages[5].intro = "06intro_new_york.m4v";
	pages[5].intro_audio = "06_New_Jork_Intro.wav";
	pages[5].dur_intro = 10.07;
	pages[5].loop = "06loop_new_york.mp4";
	pages[5].loop_audio = "06_New_Jork_Loop.wav";

	pages[6].intro = "07_Madrid_intro.mp4";
	pages[6].intro_audio = "07_Madrid_Intro.wav";
	pages[6].dur_intro = 09.00;
	pages[6].loop = "07_Madrid_loop.mp4";
	pages[6].loop_audio = "07_Madrid_Loop.wav";

	pages[7].intro = "08BuenosAiresINTRO.mp4";
	pages[7].intro_audio = "08_Buenos_Aires_Intro.wav";
	pages[7].dur_intro = 40.30;
	pages[7].loop = "08BuenosAiresLOOP.mp4";
	pages[7].loop_audio = "08_Buenos_Aires_Loop.wav";

	pages[8].intro = "09_LAGAR_INTRO.mp4";
	pages[8].intro_audio = "09_Lagar_Intro.wav";
	pages[8].dur_intro = 8.09;
	pages[8].loop = "09LAGARLOOP.mp4";
	pages[8].loop_audio = "09_Lagar_Loop.wav";

	pages[9].intro = "10CAPITULO2INTRO.m4v";
	pages[9].intro_audio = "10_Capitulo_2_Intro.wav";
	pages[9].dur_intro = 6.47;
	pages[9].loop = "10CAPITULO2LOOP.m4v";
	pages[9].loop_audio = "10_Capitulo_2_Intro.wav";

	pages[10].intro = "11_Premios_INTRO.mp4";
	pages[10].intro_audio = "11_Premios_Intro.wav";
	pages[10].dur_intro = 18.73;
	pages[10].loop = "11_Premios_LOOP.mp4";
	pages[10].loop_audio = "11_Premios_Loop.wav";

	pages[11].intro = "11_Premios_INTRO.m4v";
	pages[11].intro_audio = "10_Capitulo_2_Intro.wav";
	pages[11].dur_intro = 49.87;
	pages[11].loop = "11_Premios_LOOP.m4v";
	pages[11].loop_audio = "10_Capitulo_2_Loop.wav";

	pages[12].intro = "13CAPITULO3INTRO.m4v";
	pages[12].intro_audio = "13_Capitulo_2_Intro.wav";
	pages[12].dur_intro = 6.47;
	pages[12].loop = "13CAPITULO3LOOP.m4v";
	pages[12].loop_audio = "13_Capitulo_2_Loop.wav";

	pages[13].intro = "14_MEXICO.m4v";
	pages[13].intro_audio = "14_Reforma_Educativa_Intro.wav";
	pages[13].dur_intro = 12.07;
	pages[13].loop = "14_MEXICO_loop.m4v";
	pages[13].loop_audio = "14_Reforma_Educativa_Loop.wav";

	pages[14].intro = "15_ESCUELAS_INTRO2.mp4";
	pages[14].intro_audio = "15_EscuelasAireLibre_Intro.wav";
	pages[14].dur_intro = 24.83;
	pages[14].loop = "15_ESCUELAS_LOOP2.m4v";
	pages[14].loop_audio = "15_EscuelasAireLibre_Loop.wav";

	pages[15].intro = "16_POESIA_INTRO.mp4";
	pages[15].intro_audio = "16_Poesia_intro.wav";
	pages[15].dur_intro = 21.50;
	pages[15].loop = "16_POESIA_LOOP.mp4";
	pages[15].loop_audio = "16_Poesia_loop.wav";

	pages[16].intro = "17CIERREintro03.mp4";
	pages[16].intro_audio = "17_Cierre_Intro.wav";
	pages[16].dur_intro = 36.71;
	pages[16].loop = "17CIERREloop3.m4v";
	pages[16].loop_audio = "17_Cierre_Loop.wav";


	basePath = (*scd->dataPath)+"movies/gam_mustakis/";

#ifdef SNGAMLIBRO_USE_FFMPEG

	intro_page_vt = new FFMpegDecode();
	intro_page_vt->OpenFile(shCol, (char*)(basePath+pages[actPage].intro).c_str(),
			nrDecodeThreads, dstWidth, dstHeight, decodeYuv420OnGpu);

	loop_page_vt = new FFMpegDecode();
	loop_page_vt->OpenFile(shCol, (char*)(basePath+pages[actPage].loop).c_str(),
			nrDecodeThreads, dstWidth, dstHeight, decodeYuv420OnGpu);
	loop_page_vt->start();

	turn_page_plus_vt = new FFMpegDecode();
	turn_page_plus_vt->OpenFile(shCol, (char*)(basePath+"00HojaVuelta.mp4").c_str(),
			nrDecodeThreads, dstWidth, dstHeight, decodeYuv420OnGpu);

	turn_page_minus_vt = new FFMpegDecode();
	turn_page_minus_vt->OpenFile(shCol, (char*)(basePath+"00HojaVuelta_rev.mp4").c_str(),
			nrDecodeThreads, dstWidth, dstHeight, decodeYuv420OnGpu);

#else
	vts[ static_cast<GLuint>(sceneArgs->at("vtex1")) ];

	turn_page_plus_vt = vts[ static_cast<GLuint>( sceneArgs->at( "vtex0" ) ) ];
	turn_page_minus_vt = vts[ static_cast<GLuint>( sceneArgs->at( "vtex1" ) ) ];

	intro_page_vt = new VideoTextureCv((char*)(basePath+pages[actPage].intro).c_str());
	intro_page_vt->setLoop(false);

	loop_page_vt = new VideoTextureCv((char*)(basePath+pages[actPage].loop).c_str());
	loop_page_vt->setLoop(false);

#endif

	audioBasePath = (*scd->dataPath)+"audio/";

	turn_page_audio = "/00_HojaPasa.wav";
}

//----------------------------------------------------

void SNGam_Libro::initShader()
{
	std::string shdr_Header = shCol->getShaderHeader();

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	\n
	out block {
		vec3 normal;\n
		vec2 tex_coord;\n
	} vertex_out;

	uniform float yDistAmt;
	uniform mat4 m_pvm;);

	vert += "uniform vec2 yOffsPoints["+std::to_string(nrCurveSymPoints)+"];";

	vert += "uniform float yOffs;\n"
	"\n"
	"void main() {"
		"int xOffs = int(texCoord.x * "+std::to_string(nrCurveSymPoints)+");\n"
		"vec4 modPos = position + vec4(0.0, 0.0, yOffsPoints[xOffs].y * yDistAmt, 0.0);\n"
		"vertex_out.normal = normal.xyz;\n"
		"vertex_out.tex_coord = texCoord;\n"
		"gl_Position = m_pvm * modPos;\n"
	"}";

	vert = shdr_Header +vert;

	//----------------------------------------------------------------------------

	shdr_Header = "#version 430\n";

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;\n
	\n
	in block {
		vec3 normal;\n
		vec2 tex_coord;\n
	} vertex_in;

	uniform sampler2D tex;\n
	uniform float alpha;\n
	uniform float nrQuadsX;\n
	uniform float nrQuadsY;\n
	uniform float borderSizeX;\n
	uniform float borderSizeY;\n
	\n
	float pi = 3.1415926535897932384626433832795;
	\n
	void main() {\n
		float lightCol = min( max( abs( dot( vec3(0.0, 0.0, 1.0), vertex_in.normal ) * 0.8 ), 0.0), 1.0);
		fragColor = texture(tex, vec2( vertex_in.tex_coord.x, 1.0 - vertex_in.tex_coord.y) ) * lightCol;\n
		float border = min(vertex_in.tex_coord.x * borderSizeX, 1.0)
			* min((1.0 - vertex_in.tex_coord.x) * borderSizeX, 1.0)
			* min((1.0 - vertex_in.tex_coord.y) * borderSizeY, 1.0)
			* min(vertex_in.tex_coord.y * borderSizeY, 1.0);

		//if (alpha == 0.0) discard;
		fragColor.a = alpha;
		//fragColor.a = alpha * border;

		/*
		float colX = sin( vertex_in.tex_coord.x * pi * nrQuadsX ) > 0.0 ? 1.0 : 0.0;
		float colY = sin( vertex_in.tex_coord.y * pi * nrQuadsY ) > 0.0 ? 0.0 : 1.0;
		fragColor = vec4( colX == 0.0 ? colY : 1.0 - colY );
		*/
	});

	frag = "// SNGam_Libro fade shader frag\n"+shdr_Header+frag;

	drawShdr = shCol->addCheckShaderText("SNGam_Libro_fadeShdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Libro::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
/*
	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, vt_test.texIDs[0]);
	if(loop_page_vt->isReady())  glBindTexture(GL_TEXTURE_2D, loop_page_vt->getTex());

	rawquad->draw();
*/

	if (cp->camId == 0)
	{
		glm::mat4 pvm = projMat * viewMat * modelNode->_modelMat;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		drawShdr->begin();
		drawShdr->setUniformMatrix4fv("m_pvm", &pvm[0][0]);
		drawShdr->setUniform1i("tex", 0);
		drawShdr->setUniform1f("nrQuadsX", std::floor(nrQuadsX));
		drawShdr->setUniform1f("nrQuadsY", std::floor(nrQuadsY));
		drawShdr->setUniform2fv("yOffsPoints", &curveSymetric[0][0], nrCurveSymPoints);
		drawShdr->setUniform1f("yDistAmt", yDistAmt);
		drawShdr->setUniform1f("borderSizeX", borderSizeX);
		drawShdr->setUniform1f("borderSizeY", borderSizeY);

		glActiveTexture(GL_TEXTURE0);

		// test
		//drawShdr->setUniform1f("alpha", 1.f);
		//glBindTexture(GL_TEXTURE_2D, texIDs[0]);
		//quadArray->draw();

		// level 0  - turn page
		if ( opacity[1]->getVal() < 0.1f){
			if (actDrawMode == TURN_PAGE_PLUS){
				drawShdr->setUniform1f("alpha", 1.f);
				glBindTexture(GL_TEXTURE_2D, turn_page_plus_vt->getTex());
				quadArray->draw();
			}

			if (actDrawMode == TURN_PAGE_MINUS){
				drawShdr->setUniform1f("alpha", 1.f);
				glBindTexture(GL_TEXTURE_2D, turn_page_minus_vt->getTex());
				quadArray->draw();
			}
		}

		// level 1 - fade in page
		drawShdr->setUniform1f("alpha", opacity[1]->getVal());
		if(intro_page_vt->isReady()) glBindTexture(GL_TEXTURE_2D, intro_page_vt->getTex());
		quadArray->draw();

		// level 2 - page loop
		drawShdr->setUniform1f("alpha", opacity[2]->getVal());
		if(loop_page_vt->isReady())  glBindTexture(GL_TEXTURE_2D, loop_page_vt->getTex());
		quadArray->draw();
	}


	if (cp->camId == 1)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		stdTex->begin();
		stdTex->setIdentMatrix4fv("m_pvm");
		stdTex->setUniform1i("tex", 0);

		gui_tex->bind(0);
		rawquad->draw();
	}
}

//----------------------------------------------------

void SNGam_Libro::update(double time, double dt)
{
	actTime = time;

	if ( requestFadeIn){
        lo_send(scAddr, "/chat", "ssii", "stopall", (audioBasePath+pages[actPage].loop_audio).c_str(), (int)SND_LAYER_LOOP, 1);

        myNanoSleep(100000);

        lo_send(scAddr, "/chat", "ssii", "start", (audioBasePath+pages[actPage].loop_audio).c_str(), (int)SND_LAYER_LOOP, 1);
		startPages();
		requestFadeIn = false;
	}

	// animation values
	for (int i=0; i<nrLevels; i++)
		opacity[i]->update(time);


	// update video textures
	if (turn_page_plus_vt->isReady() && actDrawMode == TURN_PAGE_PLUS)
		turn_page_plus_vt->loadFrameToTexture(time);

	if (turn_page_minus_vt->isReady() && actDrawMode == TURN_PAGE_MINUS)
		turn_page_minus_vt->loadFrameToTexture(time);

	if (intro_page_vt->isReady()
		&& actDrawMode != PAGE_LOOPING && opacity[1]->getVal() > 0.f){
		intro_page_vt->loadFrameToTexture(time);
	}

	if (loop_page_vt->isReady() && opacity[2]->getVal() > 0.f){
		loop_page_vt->loadFrameToTexture(time);
	}

    lo_send(scAddr, "/chat", "sff", "vol", opacity[1]->getVal(), opacity[2]->getVal());

	// ------------- proc requests ----------------------------

	switch (request){
	case TR_TURN_PAGE_CHANGE_INTRO_AND_LOOP : tr_start_intro(); request = TR_NONE;
		break;
	case TR_FADEIN_INTRO : tr_fade_in_intro(); request = TR_NONE;
		break;
	case TR_START_LOOP : tr_start_loop(); request = TR_NONE;
		break;
	default :
		break;

	}

	// osc values
	if (_hasNewOscValues)
	{
		_hasNewOscValues = false;

		// custom model matrix;
		glm::vec3 tVec = glm::vec3(tx, ty, tz);
		modelNode->translate(tVec);

		glm::quat rQuat = glm::quat(glm::vec3(rx, ry, rz));
		glm::vec3 rAxis = glm::axis(rQuat);
		modelNode->rotate(glm::angle(rQuat), rAxis);

		glm::vec3 sVec = glm::vec3(sx, sy, sz);

		modelNode->scale(sVec);
		modelNode->rebuildModelMat();

		fov =	 std::atan2(0.5, zoomRatio);
		projMat = glm::perspective(fov, aspectRatio, near, far);

		saveCalib();
	}

	if (requestPushButton != -1){

		if (requestPushButton == 0)
		{
			if (mouseX->get() < 910.0)
			{
				pageMinus();
			} else {
				pagePlus();
			}
		}
		requestPushButton--;
	}
}

//----------------------------------------------------

void SNGam_Libro::onKey(int key, int scancode, int action, int mods)
{
	if (!blockInteraction && action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_UP :
				fadeInFromIdle();
				break;

			case GLFW_KEY_LEFT :
				pageMinus();
				break;

			case GLFW_KEY_RIGHT :
				pagePlus();
				break;

		default:
			break;
		}
	}
}

//----------------------------------------------------

void SNGam_Libro::onCursor(double xpos, double ypos)
{
	mousePos = glm::vec2(xpos, ypos);
	mouseX->update(xpos);
}

//----------------------------------------------------

void SNGam_Libro::onMouseButton(int button, int action, int mods)
{
	if (!blockInteraction && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// coordinate kommen rein im bezug auf beiden bildschirme...
		glm::vec2 totalScreenSize = glm::vec2(1920.f, 1080.f);
		glm::vec2 smallScreenSize = glm::vec2(1024.f, 600.f);


		// 0.0 ist oben links
		float normPos = mouseX->get()  / totalScreenSize.x;
		requestPushButton = 6;

	}
}

//----------------------------------------------------

void SNGam_Libro::startPages()
{
    opacity[2]->setInitVal(1.f);
}

//----------------------------------------------------

void SNGam_Libro::fadeInFromIdle()
{
	blockInteraction = true;

	opacity[1]->setEndFunc([this]()
	{
		actDrawMode = PAGE_BUILDS_UP;
		std::cout << "PAGE_BUILDS_UP" << std::endl;

		opacity[2]->setDelay(pageBuildUpTime);
		opacity[2]->setEndFunc([this]()
		{
			actDrawMode = PAGE_LOOPING;
			blockInteraction = false;
			std::cout << "PAGE_LOOPING" << std::endl;
		});
		opacity[2]->start(0.f, 1.f, pageFadeTime, actTime, false);
	});

	opacity[1]->start(0.f, 1.f, fadeInTime, actTime, false);
}

//----------------------------------------------------

void SNGam_Libro::pageMinus()
{
	std::cout << "pageMinus" << std::endl;

	if (actPage -1 >= 0) turnPage(actPage -1, false);
}

//----------------------------------------------------

void SNGam_Libro::pagePlus()
{
	std::cout << "pagePlus" << std::endl;
	turnPage((actPage +1) % nrPages, true);
}

//----------------------------------------------------

void SNGam_Libro::turnPage(int _destPage, bool forw)
{
	blockInteraction = true;
	destPage = _destPage;

	// set turn page mode
	actDrawMode = forw ? TURN_PAGE_PLUS : TURN_PAGE_MINUS;
	std::cout << "TURN_PAGE" << std::endl;

	// set turn page videos to beginning
	if (forw){
		turn_page_plus_vt->resetToStart();
	} else {
		turn_page_minus_vt->resetToStart();
	}

    lo_send(scAddr, "/chat", "ssii", "start", (audioBasePath+turn_page_audio).c_str(), (int)SND_LAYER_TURN, 0);

	opacity[1]->setVal(0.f); // set intro level to 0

	// fade out loop level
	opacity[2]->setDelay(0.0);
	opacity[2]->setEndFunc([this, _destPage](){
		request = TR_TURN_PAGE_CHANGE_INTRO_AND_LOOP;
	});

	// fade out loop level with pageFadeTime duration
	opacity[2]->start(1.f, 0.f, pageFadeTime, actTime, false);
}

//--------------------------------------------------------------------------------

void SNGam_Libro::tr_start_intro()
{
	std::cout << "PAGE FADED OUT, TURNING PAGE ANIMATION STILL RUNS" << std::endl;
	std::cout << "wait until turning page animation is over, " << turnPageTime - pageFadeTime * 2.0 << " secs left" << std::endl;

	std::cout << " changing videos" << std::endl;


	// load new videos
	// do it as a thread to not block
	// change loop vt
#ifdef SNGAMLIBRO_USE_FFMPEG
	intro_page_vt->open((basePath+pages[destPage].intro).c_str());
	loop_page_vt->open((basePath+pages[destPage].loop).c_str());
#else

	std::cout << "opening " << (basePath+pages[destPage].intro).c_str() << " and " << (basePath+pages[destPage].intro).c_str() << std::endl;
	intro_page_vt->open((basePath+pages[destPage].intro).c_str());
	loop_page_vt->open((basePath+pages[destPage].loop).c_str());
#endif


	// fade in new intro animation after turn page animation ended
	actPage = destPage;

	if (fadeInThread) delete fadeInThread;
	fadeInThread = new std::thread(&SNGam_Libro::fadeInFromTurnPage, this,
			std::max(turnPageTime - pageFadeTime * 2.0, 0.0));
	fadeInThread->detach();
}

//--------------------------------------------------------------------------------

void SNGam_Libro::fadeInFromTurnPage(double delay)
{
	std::chrono::milliseconds sleepFor(static_cast<int>(delay * 1000.0));
    std::this_thread::sleep_for(sleepFor);

    request = TR_FADEIN_INTRO;
}

//--------------------------------------------------------------------------------

void SNGam_Libro::tr_fade_in_intro()
{
	std::cout << "turning page animation has " << pageFadeTime << " secs left fade in the intro" << std::endl;

    // turning page animation is finished, now start the fade in animation
    intro_page_vt->resetToStart();

    lo_send(scAddr, "/chat", "si", "stop", (int)SND_LAYER_LOOP);
    lo_send(scAddr, "/chat", "ssii", "start", (audioBasePath+pages[actPage].intro_audio).c_str(), (int)SND_LAYER_INTRO, 0);

	opacity[1]->setEndFunc([this](){
		std::cout << "TURN PAGE ANIMATION FINISHED, NOW INTRO SHOULD BE VISIBLE, actpage: " << actPage << std::endl;
		request = TR_START_LOOP;
	});
	opacity[1]->start(0.f, 1.f, pageFadeTime, actTime, false);
}

//--------------------------------------------------------------------------------

void SNGam_Libro::tr_start_loop()
{
	std::cout << "now fading in loop, setting fadein delay to " << std::max(pages[actPage].dur_intro - pageFadeTime, 0.0) << std::endl;

    lo_send(scAddr, "/chat", "ssii", "start", (audioBasePath+pages[actPage].loop_audio).c_str(), (int)SND_LAYER_LOOP, 1);

	opacity[2]->setInitVal(0.f);
	opacity[2]->setVal(0.f);
	loop_page_vt->resetToStart();

	opacity[2]->setDelay(std::max(pages[actPage].dur_intro - pageFadeTime * 2.0, 0.0));
	opacity[2]->setEndFunc([this](){
		std::cout << "now PAGE_LOOPING should be visible and running" << std::endl;
		actDrawMode = PAGE_LOOPING;
		blockInteraction = false;
	});

	opacity[2]->start(0.f, 1.f, pageFadeTime, actTime, false);
}

//---------------------------------------------------------------

void SNGam_Libro::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long) ns;
	nanosleep(&tim, NULL);
}


//--------------------------------------------------------------------------------

void SNGam_Libro::loadCalib()
{
//	printf("loading calibration \n");
	cv::FileStorage fs(calibFileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		fs["tx"] >> tx;
		fs["ty"] >> ty;
		fs["tz"] >> tz;

		fs["rx"] >> rx;
		fs["ry"] >> ry;
		fs["rz"] >> rz;

		fs["sx"] >> sx;
		fs["sy"] >> sy;
		fs["sz"] >> sz;

		fs["yDistAmt"] >> yDistAmt;

		fs["nrQuadsX"] >> nrQuadsX;
		fs["nrQuadsY"] >> nrQuadsY;

		fs["zoomRatio"] >> zoomRatio;
		fs["borderSizeX"] >> borderSizeX;
		fs["borderSizeY"] >> borderSizeY;
	}

	_hasNewOscValues = true;
	update(0.0, 0.0);
}

//--------------------------------------------------------------------------------

void SNGam_Libro::saveCalib()
{
	cv::FileStorage fs(calibFileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		fs << "tx" << tx;
		fs << "ty" << ty;
		fs << "tz" << tz;

		fs << "rx" << rx;
		fs << "ry" << ry;
		fs << "rz" << rz;

		fs << "sx" << sx;
		fs << "sy" << sy;
		fs << "sz" << sz;

		fs << "yDistAmt" << yDistAmt;

		fs << "nrQuadsX" << nrQuadsX;
		fs << "nrQuadsY" << nrQuadsY;

		fs << "zoomRatio" << zoomRatio;
		fs << "borderSizeX" << borderSizeX;
		fs << "borderSizeY" << borderSizeY;

	}
}

//----------------------------------------------------

SNGam_Libro::~SNGam_Libro()
{
	delete modelRoot;
	delete [] opacity;
}

}
