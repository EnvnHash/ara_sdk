//
// SNGam_Danza.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  die "update" methode muss aufgerufen werden!!!

#include "SNGam_Danza.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{

SNGam_Danza::SNGam_Danza(sceneData* _scd, std::map<std::string, float>* _sceneArgs)
: SceneNode(_scd, _sceneArgs),
  actDrawMode(PRESENT),
  debugDrawMode(NONE),
  fastBlurAlpha(0.4f),
  fblurSize(512),
  flSize(512),
  alpha(1.f),
  spriteSize(0.02f),
  nrSongs(4),
  frameNr(0),
  enableInteract(true),
  fadeOutTime(2.0),
  selectIconRelSize(0.17f),
  selectIconThres(0.4f),
  canvasRotAngle(float(M_PI_2)),
  switchToSelectFromDance(false),
  blockIcons(false),
  soundLowLevel(0.7),
  soundHighLevel(1.0),
  actUpdtInt(0.3),
  totActSwitchLevel(0.08),
  timeToSwitchDanceIdle(10.0),
  requestSwitchToPresent(false),
  changeVidTime(3.0),
  nrVidThreads(2),
  debugLoop(true)
{
	kin = static_cast<KinectInput*>(scd->kin);
	kinRepro = static_cast<KinectReproTools*>(scd->kinRepro);

	shCol = (ShaderCollector*)_scd->shaderCollector;
	winMan = static_cast<GWindowManager*>(scd->winMan);

	// get onKey function
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

    scAddr = lo_address_new("127.0.0.1", "57120");

	// --------- VideoTexturen ------------------------------------


//    vt_back_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/entrada_loop");
    vt_back_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/00_Entrada_Loop.m4v");
    vt_back_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/Loop_Background.mp4");

    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_JAVIERA.mp4");
    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_ALEX.mp4");
    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_MONLAFERTE.mp4");
    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_DJRAFF.mp4");

/*
    vt_back_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/00_Entrada_Loop_rot.m4v");
    vt_back_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/Loop_Background_rot.m4v");

    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_JAVIERA_rot.m4v");
    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_ALEX_rot.m4v");
    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_MONLAFERTE_rot.m4v");
    vt_dance_paths.push_back((*_scd->dataPath)+"movies/gam_mustakis/BAILE_DJRAFF_rot.m4v");
*/
    back_vid_size = glm::ivec2(1920, 1080);
//    back_vid_size = glm::ivec2(1080, 1920);

    vt_back = new FFMpegDecode();
    vt_back->OpenFile(shCol, (char*) vt_back_paths[0].c_str(), nrVidThreads,
    		back_vid_size.x, back_vid_size.y, false, true);
	vt_back->start();


    vt_dance = new FFMpegDecode();
    vt_dance->OpenFile(shCol, (char*) vt_dance_paths[0].c_str(), 3, 1280, 720, false, true);
    vt_dance->start();

	// --------- Pistas de Audio -----------------------------------

/*	songFilesAudio.push_back((*_scd->dataPath)+"audio/BAILE_ALEX_44.wav");	// PRESENT
	songFilesAudio.push_back((*_scd->dataPath)+"audio/BAILE_ALEX_44.wav");	// SELECT

	songFilesAudio.push_back((*_scd->dataPath)+"audio/BAILE_JAVIERA_44.wav");	// DANCE0
	songFilesAudio.push_back((*_scd->dataPath)+"audio/BAILE_ALEX_44.wav");	// DANCE1
	songFilesAudio.push_back((*_scd->dataPath)+"audio/BAILE_MONLAFERTE_44.wav");	// DANCE2
	songFilesAudio.push_back((*_scd->dataPath)+"audio/BAILE_DJRAFF_44.wav");	// DANCE3
*/

	songFilesAudioDur.push_back(220.0); // alex
	songFilesAudioDur.push_back(220.0); // alex

	songFilesAudioDur.push_back(269.0); // javiera
	songFilesAudioDur.push_back(220.0); // alex
	songFilesAudioDur.push_back(205.0); // alex
	songFilesAudioDur.push_back(206.0); // dfraff


	silRgbFact = new glm::vec4[4];

	//----------------------------------------
	// --------- Shader ---------------------------------------------

	texShader = shCol->getStdTex();

	texAlphaShader = shCol->getStdTexAlpha();
	initDrawShdr();
	initDrawLumaKey();
	initDrawSilBlackShdr();
	initSilBackShdr();
	//initUserMapShader();

	// --------- complex inits --------------------------------------

	initFluidSim();
	initOscPar();
	initParticleSystem();
	initTextures();
	initSelectionIcons();

	// -------- Geo Primitives --------------------------------------

	rawQuad = _scd->stdQuad;
	rotateQuad = _scd->stdHFlipQuad;

	// -------- FastBlurs -------------------------------------------

	fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
	fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);

	// ------ Kinect Geo Rotate Parameters --------------------------

	transMat = glm::mat4(1.f);
	transMat2D = glm::mat4(1.f);

	transTexId = new GLint[2];
	transTexId[0] = 0;
	transTexId[1] = 1;

	// ------- calibration i/o --------------------------------------

	calibFileName = (*scd->dataPath)+"calib_cam/gam_danza.yml";

	// check if calibration file exists, if this is the case load it
	if (access(calibFileName.c_str(), F_OK) != -1)
		loadCalib();

	totActivity = new Median<float>(9.f);
	totActivity->setInitVal(0.f);

	blendVal0 = new AnimVal<float>(RAMP_LIN_UP, [this](){});
	blendVal0->setInitVal(1.f);
	blendVal1 = new AnimVal<float>(RAMP_LIN_UP, [this](){});
	blendVal1->setInitVal(0.f);
}

//----------------------------------------------------

void SNGam_Danza::initOscPar()
{
	addPar("alpha", &alpha);
	addPar("spriteSize", &spriteSize);
	addPar("fastBlurAlpha", &fastBlurAlpha);
	addPar("partSpeed", &partSpeed);

	// kinect geo rotate parameters
	addPar("kinRotX", &kinRotX);
	addPar("kinRotY", &kinRotY);
	addPar("kinRotZ", &kinRotZ);
	addPar("kinTransX", &kinTransX);
	addPar("kinTransY", &kinTransY);
	addPar("kinTransZ", &kinTransZ);
	addPar("screenScaleX", &screenScaleX);
	addPar("screenScaleY", &screenScaleY);
	addPar("screenTransX", &screenTransX);
	addPar("screenTransY", &screenTransY);
	addPar("pointSize", &pointSize);
	addPar("pointWeight", &pointWeight);
	addPar("nearThres", &nearThres);
	addPar("farThres", &farThres);


	addPar("nrPartPerEmit", &nrPartPerEmit);
	addPar("optFlowVelAmt", &optFlowVelAmt);
	addPar("optFlowPosAmt", &optFlowPosAmt);
	addPar("fluidVelTexForce", &fluidVelTexForce);
	addPar("windAmt", &windAmt);



	addPar("lumaThres", &lumaThres);
	addPar("lumaMult", &lumaMult);

	addPar("silRgbFact0R", &silRgbFact[0].r);
	addPar("silRgbFact0G", &silRgbFact[0].g);
	addPar("silRgbFact0B", &silRgbFact[0].b);
	addPar("silRgbFact0A", &silRgbFact[0].a);

	addPar("silRgbFact1R", &silRgbFact[1].r);
	addPar("silRgbFact1G", &silRgbFact[1].g);
	addPar("silRgbFact1B", &silRgbFact[1].b);
	addPar("silRgbFact1A", &silRgbFact[1].a);

	addPar("silRgbFact2R", &silRgbFact[2].r);
	addPar("silRgbFact2G", &silRgbFact[2].g);
	addPar("silRgbFact2B", &silRgbFact[2].b);
	addPar("silRgbFact2A", &silRgbFact[2].a);

	addPar("silRgbFact3R", &silRgbFact[3].r);
	addPar("silRgbFact3G", &silRgbFact[3].g);
	addPar("silRgbFact3B", &silRgbFact[3].b);
	addPar("silRgbFact3A", &silRgbFact[3].a);

	addPar("silColDepth", &silColDepth);



}

//----------------------------------------------------

void SNGam_Danza::initParticleSystem()
{
	// vao mit irgendwas drin, damit die sache mit dem shader storage buffer richtig funktioniert
	testVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat pos[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
	testVAO->upload(POSITION, pos, 4);
	GLuint ind[] = { 0, 1, 3, 1, 3, 2 };
	testVAO->setElemIndices(6, ind);

	// create ubo and initialize it with the structure data
	glGenBuffers(1, &mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);

	mParticles = new tav::GLSLParticleSystemCS(1<<20, shCol, GLSLParticleSystemCS::DRAW_GEN_QUADS);
	mParticles->addBuffer("vel");
	mParticles->addBuffer("aux0");
	mParticles->addBuffer("aux1");

	mParticles->zero();	// all buffers to zero
	mParticles->setUpdateShader(getUpdtShader().c_str());
	mParticles->setEmitShader();

	initPars = new glm::vec4[4];
}

//----------------------------------------------------

void SNGam_Danza::initTextures()
{
	flowers = new TextureManager[nrSongs];
	flowers_normals = new TextureManager[nrSongs];

	flowers[0].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/03_JAVIERA.png", 8);
	flowers_normals[0].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/03_JAVIERA_norm.png", 8);
	flowers[1].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/01_ALEX.png", 8);
	flowers_normals[1].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/01_ALEX_norm.png", 8);
	flowers[2].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/04_MONLAFERTE.png", 8);
	flowers_normals[2].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/04_MONLAFERTE_norm.png", 8);
	flowers[3].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/02_RAFF.png", 8);
	flowers_normals[3].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/02_RAFF_norm.png", 8);

	flowers_back.loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/flowers_back.jpg", 8);
	text_select.loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/Texto_Selecciona_00000.png", 1);

	// selection icons
	select_icons = new TextureManager[nrSongs];
	select_icons[0].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/BT_JAVIERA_00001.png", 1);
	select_icons[1].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/BT_ALEX_00001.png", 1);
	select_icons[2].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/BT_MONLAFERTE_00001.png", 1);
	select_icons[3].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/BT_DJRAFF_00001.png", 1);

	// dance background textures
	dance_back_texs = new TextureManager[nrSongs];
	dance_back_texs[0].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/FONDO_JAVIERA.png", 1);
	dance_back_texs[1].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/FONDO_ALEX.png", 1);
	dance_back_texs[2].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/FONDO_MONLAFERTE.png", 1);
	dance_back_texs[3].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/FONDO_DJRAFF.png", 1);

	unsigned int nrLevels = 8;
	flowers_fill = new TextureManager[nrLevels];
	for (unsigned int i=0; i<nrLevels; i++)
		flowers_fill[i].loadTexture2D((*scd->dataPath)+"/textures/gam_mustakis/danza/flower_tex"+std::to_string(i)+".jpg", 8);

	uint8_t* allLevels = new uint8_t[flowers_fill[0].getWidth() * flowers_fill[0].getHeight() *nrLevels *3 * sizeof(uint8_t)];
	for (unsigned int i=0; i<nrLevels; i++){
		std::memcpy(&allLevels[flowers_fill[0].getWidth() * flowers_fill[0].getHeight() *3 *i],
				flowers_fill[i].getBits(),
				flowers_fill[0].getWidth() * flowers_fill[0].getHeight() *3 * sizeof(uint8_t));
	}

	// generate 3d texture
	glGenTextures(1, &flowers_3Dtex);
	glBindTexture(GL_TEXTURE_3D, flowers_3Dtex);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, flowers_fill[0].getWidth(), flowers_fill[0].getHeight(),
			nrLevels, 0, GL_RGB, GL_UNSIGNED_BYTE, allLevels);
	glBindTexture(GL_TEXTURE_3D, 0);

	sil_back_fbo = new FBO(shCol, 1024, 1024, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);
	bailarina_fbo = new PingPongFbo(shCol, scd->screenWidth, scd->screenHeight, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);
}

//----------------------------------------------------

void SNGam_Danza::initSelectionIcons()
{
	glm::vec4 quadPoints[2];
	glm::vec4 modQuadPoints[2];

	quadPoints[0] = glm::vec4(-1.f, -1.f, 0.f, 1.f);
	quadPoints[1] = glm::vec4( 1.f,  1.f, 0.f, 1.f);

	selectIconBorder = (2.f - float(nrSongs) * selectIconRelSize) / float(nrSongs +1);
	icon_mat = new glm::mat4[nrSongs];
	pixCounter = new PixelCounter*[nrSongs];
	icon_scale_offs = new AnimVal<float>*[nrSongs];

	for (unsigned int i=0; i<nrSongs; i++)
	{
		float fInd = float(i);

		icon_scale_offs[i] = new AnimVal<float>(RAMP_LIN_UP, [this](){});
		icon_scale_offs[i]->setInitVal(0.f);

		// calculate the relative size of the icons in relation to screen space (-1|1)
		glm::vec3 icon_tex_scale = glm::vec3(selectIconRelSize,
				selectIconRelSize
				* select_icons[i].getHeightF() / select_icons[i].getWidthF()
				* scd->screenHeight / scd->screenWidth,
				1.f);

		// calculate the relative position of the icons in relation to screen space (-1|1)
		glm::vec3 icon_tex_translate = glm::vec3(
				-1.f
				+ (fInd + 1.f) * selectIconBorder + (fInd + 0.5f)
				* selectIconRelSize,
			0.675f, 0.f);

		// calculate the model matrix of the icons with the screen rotated by 90 degrees
		icon_mat[i] = glm::rotate(canvasRotAngle, glm::vec3(0.f, 0.f, 1.f) )
					* glm::translate(icon_tex_translate)
					* glm::scale(icon_tex_scale);

		// calculate modulate quad points
		for (unsigned int j=0; j<2; j++)
		{
			modQuadPoints[j] = icon_mat[i] * quadPoints[j];

			// move to normalized (0|1) space and calculate pixel values
			modQuadPoints[j] = glm::vec4((modQuadPoints[j].x * 0.5f + 0.5f) * float(fblurSize),
										 (modQuadPoints[j].y * 0.5f + 0.5f) * float(fblurSize),
										 0.f, 1.f);
		}

		pixCounter[i] = new PixelCounter(
				static_cast<int>(modQuadPoints[0].x - modQuadPoints[1].x),
				static_cast<int>(modQuadPoints[1].y - modQuadPoints[0].y),
				static_cast<int>(modQuadPoints[1].x) +1,
				static_cast<int>(modQuadPoints[0].y),
				".r > 0.1", shCol);
	}
}

//----------------------------------------------------

void SNGam_Danza::initFluidSim()
{
	// - Fluid System --
	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flSize, flSize, 0.5f);
	fluidSim->dissipation = 0.999f;
	fluidSim->velocityDissipation = 0.999f;
	fluidSim->setVelTexThresh(0.8f);
	fluidSim->setSmokeBuoyancy(0.457f);
	fluidSim->setVelTexRadius(0.2f);

	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));
}

//----------------------------------------------------

void SNGam_Danza::initDrawLumaKey()
{
	std::string vert = STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			\n
			uniform mat4 m_pvm;\n
			\n
			out vec2 tex_coord;\n
			\n
			void main(){\n
				tex_coord = texCoord;\n
				gl_Position = position;\n
			});

	vert = shCol->getShaderHeader() +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform sampler2D back;\n
			in vec2 tex_coord;\n
			uniform float thres;\n
			uniform float mult;\n
			uniform float alpha;\n
			\n
			layout (location = 0) out vec4 color;\n
			\n
			void main(){\n
				vec4 texCol = texture(tex, tex_coord);\n
				vec4 backCol = texture(back, tex_coord);\n

				float luma = texCol.r + texCol.g + texCol.b;
				luma = min(max(luma - thres, 0.0) * mult, 1.0);
				color = mix(backCol, texCol, luma);\n
				color.a = alpha;\n
			});
	frag = "// SNGam_Danza luma_key_shader frag\n"+shCol->getShaderHeader()+frag;

	luma_key_shader = shCol->addCheckShaderText("SNGam_Danza_luma_key", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Danza::initDrawShdr()
{
	std::string vert = STRINGIFY(
	layout(std140, binding=1) uniform ShaderParams\n
	{\n
		mat4 ModelView;\n
		mat4 ModelViewProjection;\n
		mat4 ProjectionMatrix;\n
		vec4 attractor;\n
		uint  numParticles;\n
		float spriteSize;\n
		float damping;\n
		float noiseFreq;\n
		vec3 noiseStrength;\n
	};\n
	\n
	layout( std140, binding=1 ) buffer Pos { vec4 pos[]; };\n
	layout( std140, binding=2 ) buffer Aux0 { vec4 aux0[]; };\n
	layout( std140, binding=3 ) buffer Aux1 { vec4 aux1[]; };\n
	\n
	uniform float flowersTexStep;\n
	uniform float flowersTexPerRow;\n
	\n
	out gl_PerVertex {\n
		vec4 gl_Position;\n
	};\n
	\n
	out block {\n
		vec2 texCoord;\n
		vec3 position;\n
		vec3 normal;\n
		float lifeTime;\n
		float bright;\n
	} Out;\n
	\n
	mat4 matFromEulerAngles(vec3 angles) {\n
		mat4 mat;\n
		\n
		float A       = cos(angles.x);\n
		float B       = sin(angles.x);\n
		float C       = cos(angles.y);\n
		float D       = sin(angles.y);\n
		float E       = cos(angles.z);\n
		float F       = sin(angles.z);\n
		\n
		float AD      =   A * D;\n
		float BD      =   B * D;\n

	    mat[0].x  =   C * E;\n
	    mat[1].x  =  -C * F;\n
	    mat[2].x  =  -D;\n
	    mat[0].y  = -BD * E + A * F;\n
	    mat[1].y  =  BD * F + A * E;\n
	    mat[2].y  =  -B * C;\n
	    mat[0].z  =  AD * E + B * F;\n
	    mat[1].z  = -AD * F + B * E;\n
	    mat[2].z =   A * C;\n
	    \n
	    mat[3].x  =  mat[3].y = mat[3].z = mat[0].w = mat[1].w = mat[2].w = 0;\n
	    mat[3].w =  1.0;\n
	    \n
	    return mat;\n
	}\n

	void main() {
		// expand points to quads without using GS
		int particleID = gl_VertexID >> 2;\n // 4 vertices per particle
		vec4 particlePos = pos[particleID];\n

		//map vertex ID to quad vertex
		vec2 quadPos = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);\n
		float texOffs = floor(aux0[particleID].w);\n
		Out.texCoord = quadPos * flowersTexStep\n
				+ vec2(mod(texOffs, flowersTexPerRow) * flowersTexStep,\n
								floor(texOffs / flowersTexPerRow) * flowersTexStep);\n // quadPos is 0 - 1

		quadPos = (quadPos * 2.0 - 1.0) * aux1[particleID].x;\n
//		mat4 rotMat = matFromEulerAngles(vec3(0.0, 0.0, aux0[particleID].z));
		mat4 rotMat = matFromEulerAngles(aux0[particleID].xyz);
		vec4 rotQuadPos = rotMat * vec4(quadPos, 0.0, 1.0);\n

		Out.lifeTime = min(sqrt(particlePos.a), 1.0);\n
		Out.bright = 1.0 + max(-1.0, particlePos.z * 0.5);\n
		Out.normal = (rotMat * vec4(0.0, 0.0, 1.0, 0.0)).xyz;\n
		Out.position = rotQuadPos.xyz;\n

		gl_Position = ModelViewProjection * vec4(particlePos.xyz + rotQuadPos.xyz, 1.0);
	});

	vert = shCol->getShaderHeader() +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;\n
	\n
	in block {\n
		vec2 texCoord;\n
		vec3 position;\n
		vec3 normal;\n
		float lifeTime;\n
		float bright;\n
	} In;\n
	\n
	uniform sampler2D flowerTex;\n
	uniform sampler2D flowerTexNormals;\n
	uniform float flowersTexStep;\n
	uniform float alpha;\n
	\n
	uniform vec3 LPosition;\n
	uniform vec3 LSpecular;\n
	uniform float shininess;\n     // exponent for sharping highlights
	\n
	void main() {\n
		\n
		vec4 texCol = texture(flowerTex, In.texCoord);\n
		float compAlpha = In.lifeTime * texCol.a;\n
		if (compAlpha < 0.01) discard;\n
		\n
		vec4 modNorm = texture(flowerTexNormals, In.texCoord);\n
		modNorm = vec4(In.normal, 0.0) + modNorm;\n
		\n
		vec3 L = normalize(LPosition - In.position);\n
		vec3 E = normalize(-In.position); \n // we are in Eye Coordinates, so EyePos is (0,0,0)\n"
		vec3 R = normalize(-reflect(L, modNorm.xyz));\n

		//calculate Diffuse Term:
		vec4 Idiff = vec4(2.0) * max(abs(dot(modNorm.xyz, L)), 0.1);\n
		Idiff = clamp(Idiff, 0.0, 1.0);\n

		// calculate Specular Term:
		vec4 Ispec = vec4(LSpecular, 1.0) * pow(max(dot(R, E), 0.0), 0.3 * shininess);\n
		Ispec = clamp(Ispec, 0.0, 1.0);\n

		//vec4 color = (Idiff + Ispec) * texCol;\n
		fragColor = vec4(texCol.rgb * (Idiff.rgb + Ispec.rgb) * In.bright, compAlpha);\n
	});

	frag = "// SNGam_Danza frag\n" + shCol->getShaderHeader() + frag;

	mRenderProg = shCol->addCheckShaderText("SNGam_Danza", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Danza::initUserMapShader()
{
	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	uniform mat4 m_pvm;\n
	out vec2 tex_coord;\n
	void main(){\n
		tex_coord = texCoord;\n
		gl_Position = m_pvm * position;\n
	});

	vert = shCol->getShaderHeader() + vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	uniform sampler2D tex;\n
	in vec2 tex_coord;\n
	layout (location = 0) out vec4 color;\n
	void main(){\n
		vec4 outCol = texture(tex, tex_coord);\n
		color = outCol;\n
	});

	frag = "// SNGam_Danza user amp shader frag\n" + shCol->getShaderHeader() + frag;

	useMapShader = shCol->addCheckShaderText("SNGam_Danza_user_map", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Danza::initDrawSilBlackShdr()
{
	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	uniform mat4 m_pvm;\n
	out vec2 tex_coord;\n
	void main(){\n
		tex_coord = texCoord;\n
		gl_Position = m_pvm * position;\n
	});

	vert = shCol->getShaderHeader() + vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	uniform sampler2D tex;\n
	in vec2 tex_coord;\n
	layout (location = 0) out vec4 color;\n
	void main(){\n
		float outCol = texture(tex, tex_coord).r;\n
		if (outCol < 0.05){\n
			discard;\n
		} else {\n
			color = vec4(0.0, 0.0, 0.0, 1.0);\n
		}\n
	});

	frag = "// SNGam_Danza sil black frag\n" + shCol->getShaderHeader() + frag;

	drawSilBlack = shCol->addCheckShaderText("SNGam_Danza_sil_black", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Danza::initSilBackShdr()
{
	std::string vert = STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			uniform mat4 m_pvm;\n
			out vec2 tex_coord;\n
			void main(){\n
				tex_coord = texCoord;\n
				gl_Position = m_pvm * position;\n
			});

	vert = shCol->getShaderHeader() +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D mask;\n
			uniform sampler3D flowers;\n
			uniform sampler3D noise;\n
			uniform float time;\n
			uniform float colDepth;\n
			uniform vec4 rgbFact;\n

			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n

			void main(){\n
				vec4 maskCol = texture(mask, tex_coord);\n
				if (maskCol.r < 0.05){\n
					discard;\n
				} else {\n
					vec3 randVal = texture(noise, vec3(tex_coord * 0.5 + (time * 0.011), time*0.01)).xyz;\n
					vec3 fl_tex_coord = vec3(tex_coord.x,
							tex_coord.y + randVal.z * 0.05,
							time * 0.12 + randVal.y * 0.1);\n
					color = texture(flowers, fl_tex_coord);\n

					color.rgb = (color.rgb * colDepth) + vec3(1.0 - colDepth);

					color.rgb *= rgbFact.rgb;
					color.a = rgbFact.a * maskCol.a;
				}\n
			});
	frag = "// SNGam_Danza sil back fbo frag\n"+shCol->getShaderHeader()+frag;

	sil_back_shader = shCol->addCheckShaderText("SNGam_Danza_silback", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

std::string SNGam_Danza::getUpdtShader()
{
	// version statement kommt von aussen
	std::string src = shCol->getShaderHeader();

	src += "layout(local_size_x="+std::to_string(mParticles->getWorkGroupSize())+", local_size_y=1, local_size_z=1) in;\n";

	src += STRINGIFY(
			layout( std140, binding=0 ) buffer Pos  { vec4 pos[]; };\n
			layout( std140, binding=1 ) buffer Vel  { vec4 vel[]; };\n
			layout( std140, binding=2 ) buffer Aux0 { vec4 aux0[]; };\n
			\n
			uniform float dt;\n
			uniform float time;\n
			uniform float optFlowPosAmt;\n
			uniform float optFlowVelAmt;\n
			uniform float partSpeed;\n
			uniform float windAmt;\n
			uniform uint numParticles;\n
			\n
			uniform sampler3D noiseTex3D;\n
			uniform sampler2D fluidVel;\n
			\n
			// compute shader to update particles
			void main() {\n
				uint i = gl_GlobalInvocationID.x;\n
				if (i >= numParticles) return;\n

				// read particle position and velocity from buffers
				vec3 p = pos[i].xyz;\n
				vec3 v = vel[i].xyz;\n

				// randomValue -1 to 1
				vec3 texCoord = p * 0.5 + 0.5;\n
				vec4 randVal = texture(noiseTex3D, texCoord * 0.1 + time * 0.005);\n
				vec4 optFlowVal = texture(fluidVel, pos[i].xy * 0.5 + vec2(0.5));\n

				aux0[i].xyz += randVal.xyz * dt * 5.0;
				p += randVal.xyz * dt * windAmt;	// wind

				// optflow
				//p += vec3(optFlowVal.xy, 0.0) * optFlowPosAmt;\n
				v = mix(v, optFlowVal.xyz * optFlowPosAmt, optFlowVelAmt) * partSpeed;\n

				// integrate
				p += v;\n
				//v *= damping;\n

				// write new values
				pos[i] = vec4(p, max(pos[i].a - dt, 0.0));\n // lower lifetime
				vel[i] = vec4(v, 0.0);\n
			});
	return src;
}

//----------------------------------------------------

void SNGam_Danza::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(inited)
	{
		if (_tfo) _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (debugDrawMode != NONE)
		{
			drawDebug(time, dt, cp, _shader, _tfo);

		} else
		{
			switch(actDrawMode)
			{
				case PRESENT :
				{
					glDisable(GL_DEPTH_TEST);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

					glm::mat4 rot90deg = glm::rotate(canvasRotAngle*2.f, glm::vec3(0.f, 0.f, 1.f) );

					//texShader->begin();
					//texShader->setIdentMatrix4fv("m_pvm");
					//texShader->setUniform1i("tex", 0);
					//glActiveTexture(GL_TEXTURE0);

					//glBindTexture(GL_TEXTURE_2D, vt_back->getTex());

					//rotateQuad->draw();


					if (vt_back && vt_back->getTex() != 0 && vt_back->getShader() != 0)
					{
						vt_back->shaderBegin();
						vt_back->getShader()->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
						vt_back->getShader()->setUniform1f("alpha", 1.f);
//						vt_back->getShader()->setUniform1f("alpha", opacity);

						rotateQuad->draw();

//						vt_back->shaderEnd();
					}

					break;
				}

				case SELECT: drawSelect(cp); break;
				case DANCE: drawDance(time, dt, cp); break;

				default:
					break;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void SNGam_Danza::drawDebug(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);
	glActiveTexture(GL_TEXTURE0);

	switch(debugDrawMode)
	{
	case RAW_DEPTH :
		glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
		break;
/*
	case USER_MAP:
		useMapShader->begin();
		useMapShader->setIdentMatrix4fv("m_pvm");
		useMapShader->setUniform1i("tex", 0);
		userMapTex.bind(0);
		break;
		*/
	case TRANS_DEPTH:
		glBindTexture(GL_TEXTURE_2D, transTexId[1]);
		break;
	case DEPTH_THRESH:
		glBindTexture(GL_TEXTURE_2D, threshFbo->getResult());
		break;
	case DEPTH_BLUR:
		glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
		break;
	case OPT_FLOW:
		glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
		break;
	case OPT_FLOW_BLUR:
		glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());
		break;
	case FLUID:
		glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());
		break;
	default:
		break;
	}

	rawQuad->draw();
}

//-----------------------------------------------------------------------------

void SNGam_Danza::drawSelect(camPar* cp)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	glm::mat4 rot90deg = glm::rotate(canvasRotAngle*2.f, glm::vec3(0.f, 0.f, 1.f) );

	// ------ background --------------------------------

	if (vt_back->getTex() != 0 && vt_back->getShader() != 0)
	{
		vt_back->shaderBegin();
		vt_back->getShader()->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
		vt_back->getShader()->setUniform1f("alpha", opacity);

		rotateQuad->draw();

		vt_back->shaderEnd();
	}

	// ------ typo "selecciona tu artista" --------------

	float textRelWidth = 0.6f;
	glm::mat4 text_select_mat = glm::translate(glm::vec3(-0.9f, 0.f, 1.f) )
		* glm::rotate(canvasRotAngle, glm::vec3(0.f, 0.f, 1.f) )
		* glm::scale(glm::vec3(textRelWidth,
								text_select.getHeightF() / text_select.getWidthF()
									* cp->actFboSize.y / cp->actFboSize.x
									* textRelWidth,
								1.f));

	texAlphaShader->begin();
	texAlphaShader->setUniform1i("tex", 0);
	texAlphaShader->setUniform1f("alpha", opacity);
	texAlphaShader->setUniformMatrix4fv("m_pvm", &text_select_mat[0][0]);
	text_select.bind(0);
	rawQuad->draw();


	// ------- silhoutte in black -------------------------

	drawSilBlack->begin();
	drawSilBlack->setIdentMatrix4fv("m_pvm");
	drawSilBlack->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());

	rawQuad->draw();


	// ------- iconos -----------------------------------

	texAlphaShader->begin();
	texAlphaShader->setUniform1i("tex", 0);
	texAlphaShader->setUniform1f("alpha", opacity);

	for (unsigned int i=0; i<nrSongs; i++)
	{
		float fInd = float(i);

		// calculate the relative size of the icons in relation to screen space (-1|1)
		glm::vec3 icon_tex_scale = glm::vec3(selectIconRelSize,
				selectIconRelSize
				* select_icons[i].getHeightF() / select_icons[i].getWidthF()
				* scd->screenHeight / scd->screenWidth,
				1.f);

		// calculate the relative position of the icons in relation to screen space (-1|1)
		glm::vec3 icon_tex_translate = glm::vec3(
				-1.f
				+ (fInd + 1.f) * selectIconBorder + (fInd + 0.5f)
				* selectIconRelSize,
			0.675f, 0.f);


	//	std::cout << icon_scale_offs[i]->getVal() << std::endl;

		icon_tex_scale.x += icon_scale_offs[i]->getVal();
		icon_tex_scale.y += icon_scale_offs[i]->getVal();


		// calculate the model matrix of the icons with the screen rotated by 90 degrees
		icon_mat[i] = glm::rotate(canvasRotAngle, glm::vec3(0.f, 0.f, 1.f) )
					* glm::translate(icon_tex_translate)
					* glm::scale(icon_tex_scale);

		texAlphaShader->setUniformMatrix4fv("m_pvm", &icon_mat[i][0][0]);
		select_icons[i].bind(0);
		rawQuad->draw();
	}

}

//----------------------------------------------------

void SNGam_Danza::drawDance(double time, double dt, camPar* cp)
{
	// ------ background ----------------------------------------

	bailarina_fbo->dst->bind();

	glm::mat4 rot90deg = glm::rotate(canvasRotAngle, glm::vec3(0.f, 0.f, 1.f) );

	texAlphaShader->begin();
	texAlphaShader->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
	texAlphaShader->setUniform1i("tex", 0);
	texAlphaShader->setUniform1f("alpha", opacity);
	dance_back_texs[actSong].bind(0);
	rotateQuad->draw();

	// ------ Particles (Flowers) --------------------------------

//	drawParticles(cp, time, dt);

	bailarina_fbo->dst->unbind();

	// ------- generate silhuette background ---------------------

	sil_back_fbo->bind();
	sil_back_fbo->clear();

	sil_back_shader->begin();
	sil_back_shader->setIdentMatrix4fv("m_pvm");
	sil_back_shader->setUniform1i("mask", 0);
	sil_back_shader->setUniform1i("flowers", 1);
	sil_back_shader->setUniform1i("noise", 2);
	sil_back_shader->setUniform1f("time", time);
	sil_back_shader->setUniform4fv("rgbFact", &silRgbFact[actSong][0]);
	sil_back_shader->setUniform1f("colDepth", silColDepth);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, flowers_3Dtex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, mParticles->getNoiseTex());

	rawQuad->draw();

	sil_back_shader->end();
	sil_back_fbo->unbind();

	// ------- draw silhuette background ---------------------

	bailarina_fbo->dst->bind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sil_back_fbo->getColorImg());
	rawQuad->draw();


	// ------ Particles (Flowers) --------------------------------

	texAlphaShader->begin();
	texAlphaShader->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
	texAlphaShader->setUniform1i("tex", 0);
	texAlphaShader->setUniform1f("alpha", opacity);
	dance_back_texs[actSong].bind(0);

	drawParticles(cp, time, dt);


	bailarina_fbo->dst->unbind();
	bailarina_fbo->swap();

	// ------ "instruccion" --------------------------------

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	bailarina_fbo->dst->bind();
	bailarina_fbo->dst->clear();


	rot90deg = glm::translate(glm::vec3(-0.6f, 0.6f, 0.f))
		* glm::rotate(canvasRotAngle * 2.f, glm::vec3(0.f, 0.f, 1.f) )
		* glm::scale(glm::vec3(0.4f, 0.4f, 1.f));

	if (vt_dance && vt_dance->getTex() != 0)
	{
		vt_dance->shaderBegin();
		vt_dance->getShader()->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
		vt_dance->getShader()->setUniform1f("alpha", 1.f);

		rotateQuad->draw();

		vt_dance->shaderEnd();
	}


	/*
	texShader->begin();
	texShader->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
	texShader->setUniform1i("tex", 0);
	//vt_dance->bind(0);
	rotateQuad->draw();
	*/

	bailarina_fbo->dst->unbind();
	bailarina_fbo->swap();

	// ------ draw result --------------------------------

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	luma_key_shader->begin();
	luma_key_shader->setUniformMatrix4fv("m_pvm", &rot90deg[0][0]);
	luma_key_shader->setUniform1i("tex", 0);
	luma_key_shader->setUniform1i("back", 1);
	luma_key_shader->setUniform1f("mult", lumaMult);
	luma_key_shader->setUniform1f("thres", lumaThres);
	luma_key_shader->setUniform1f("alpha", opacity);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bailarina_fbo->src->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bailarina_fbo->dst->getColorImg());

	rawQuad->draw();

}

//---------------------------- draw particles --------------------------------------------------

void SNGam_Danza::drawParticles(camPar* cp, double time, double dt)
{
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_CLAMP);

	mShaderParams.ModelView = cp->view_matrix_mat4;
	mShaderParams.ModelViewProjection = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
	mShaderParams.ProjectionMatrix = cp->projection_matrix_mat4;

	// update struct representing UBO
	mShaderParams.numParticles = mParticles->getSize();
	mShaderParams.spriteSize = spriteSize;
	mShaderParams.attractor.w = 0.0f;

	glActiveTexture(GL_TEXTURE0);

	// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // additive blend
	glDisable(GL_CULL_FACE);

	// draw particles
	mRenderProg->begin();
	mRenderProg->setUniform1f("alpha", alpha);
	mRenderProg->setUniform1i("flowerTex", 1);
	mRenderProg->setUniform1i("flowerTexNormals", 2);
	mRenderProg->setUniform1f("flowersTexStep", 1.f / 3.f);
	mRenderProg->setUniform1f("flowersTexPerRow", 3.f);
	mRenderProg->setUniform1f("shininess", 10.f);
	mRenderProg->setUniform3f("LPosition", -1.f, 1.f, 0.f);
	mRenderProg->setUniform3f("LSpecular", 0.4f, 0.4f, 0.4f);

	flowers[actSong].bind(1);
	flowers_normals[actSong].bind(2);

	// reference the compute shader buffer, which we will use for the particle
	// wenn kein vao gebunden ist, funktioniert glDrawElements nicht...
	testVAO->bind();

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  mParticles->getBuffer("pos")->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  mParticles->getBuffer("aux0")->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  mParticles->getBuffer("aux1")->getBuffer() );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mParticles->getIndexBuffer()->getBuffer() );

	glDrawElements(GL_TRIANGLES, mParticles->getSize()*6, GL_UNSIGNED_INT, 0);

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	mRenderProg->end();
	testVAO->unbind();

	/*
	// silhouette in schwarz drueber
	drawSilBlack->begin();
	drawSilBlack->setIdentMatrix4fv("m_pvm");
	drawSilBlack->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
	rawQuad->draw();

	drawSilBlack->end();
	*/
}

//----------------------------------------------------

void SNGam_Danza::update(double time, double dt)
{
	/*
	// FOR DEBUGGING close an open the videoplayer continously
	if (time > changeVidTime &&  vt_back && time - lastChangeVidTime > changeVidTime)
	{
		std::cout << "switch" << std::endl;

		lastChangeVidTime = time;
		vt_back->stop();
		delete vt_back;
		vt_back = NULL;

		vt_back = new FFMpegDecode();
	    vt_back->OpenFile(shCol, (char*) vt_back_paths[0].c_str(), 3, 1920, 1080, true, true);
	    vt_back->start();
	}
*/


	// update video textures
	if (actDrawMode != DANCE && vt_back)
	{
		vt_back->loadFrameToTexture(time);
	}

	if (actDrawMode == DANCE && vt_dance)
		vt_dance->loadFrameToTexture(time);


	for (unsigned int i=0; i<nrSongs; i++)
		icon_scale_offs[i]->update(time);

	// update animation values
	blendVal0->update(time);
	blendVal1->update(time);
	opacity = blendVal0->getVal() + blendVal1->getVal();

	// osc values
	if (_hasNewOscValues)
	{
		// Kinect Geo Rotate
		transMat = glm::rotate(glm::mat4(1.f), kinRotX, glm::vec3(1.f, 0.f, 0.f))
				* glm::rotate(glm::mat4(1.f), kinRotY, glm::vec3(0.f, 1.f, 0.f))
				* glm::rotate(glm::mat4(1.f), kinRotZ, glm::vec3(0.f, 0.f, 1.f));

		transMat *= glm::translate( glm::vec3( kinTransX, kinTransY, kinTransZ ) );

		transMat2D = glm::translate( glm::vec3( screenTransX, screenTransY, 0.f ) )
			* glm::scale(glm::vec3(screenScaleX, screenScaleY, 1.f));

		saveCalib();

		_hasNewOscValues = false;
	}

	if (kin && kin->isReady())
	{
		if (!inited)
		{

			// start sound at layer 0
		    lo_send(scAddr, "/chat", "sif", "start", 0, soundLowLevel);

			kin->setCloseRange(true);
			kin->setDepthVMirror(true);

			threshFbo = new ThreshFbo(scd, kin->getDepthWidth(), kin->getDepthHeight(),
					GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

			// create a texture for uploading the usermap
			// will be converted to floats on upload
			userMapTex.allocate(kin->getDepthWidth(), kin->getDepthHeight(), GL_R16, GL_RED,
					GL_TEXTURE_2D, GL_SHORT);

			// - Optical Flow --
			optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(), kin->getDepthHeight());
			optFlow->setMedian(0.8f);
			optFlow->setBright(0.3f);

			optFlowBlur = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(), kin->getDepthHeight(), GL_RGB16F);


			totPixCounter = new PixelCounter(fblurSize, fblurSize, 0, 0, ".r > 0.1", shCol);
			inited = true;
		} else
		{
			if (kin->uploadDepthImg(false))
			{
				kin->uploadDepthImg(false);
				frameNr = kin->getDepthUplFrameNr(false);

				transTexId = kinRepro->transformDepth(0, false, &transMat[0][0], &transMat2D[0][0],
						pointSize, pointWeight, nearThres, farThres, 1);

				//transTexId = kinRepro->transformDepth(0, false, &transMat[0][0], &transMat2D[0][0],
				//		pointSize, pointWeight, nearThres, farThres, 1);

				// -- threshhold the kinect depth texture

				threshFbo->proc(GL_TEXTURE_2D, transTexId[1], 0.01f, 1.f);

				// -- apply blur on silhoutte--

				fblur->setAlpha(fastBlurAlpha);
				fblur->proc(threshFbo->getResult());

				fblur2nd->setAlpha(fastBlurAlpha);
				fblur2nd->proc(fblur->getResult());

				// -- calculate optical flow, the result will be used to add velocity to the fluid --

				optFlow->update(fblur2nd->getResult(), fblur2nd->getLastResult());

				optFlowBlur->setAlpha(0.76f);
				optFlowBlur->proc(optFlow->getResTexId());

				// -- if we are in SELECT mode, calculate activity on the select icons
				if (time > 2.0 && enableInteract && actDrawMode == SELECT && time - lastActUpdtTime > actUpdtInt)
				{
					actIt = (actIt +1 ) % nrSongs;

						pixCounter[actIt]->count(fblur2nd->getResult());
						std::cout << "[" << actIt << "]" << pixCounter[actIt]->getResultFloat() << std::endl;

						if( !blockIcons && pixCounter[actIt]->getResultFloat() > selectIconThres ){

							actSong = actIt;
							switchTo(DANCE, time);
							blockIcons = true;

							icon_scale_offs[actIt]->start(0.f, 0.05f, 0.1, time, false);
						}
						lastActUpdtTime = time;
				}

				if (actDrawMode != SELECT && time - lastActUpdtTime > actUpdtInt)
				{
					totPixCounter->count(fblur2nd->getResult());
					lastActUpdtTime = time;
				}
			}

			// update fluid simulation
			//--- Update Fluid, muss jeden Frame passieren, sonst flimmer effekte ----

			fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
			fluidSim->addColor(optFlowBlur->getResult(), glm::vec3(1.f, 0.f, 0.f), 1.f, true);
			fluidSim->update();

			//-------------------------------------------------

			// this buffer is also used by the compute shader inside GLSLParticleSystem
			// make framerate independent
			mParticles->procEmit((float)time); // muss immer laufen, sonst wird der QuadTreeSorter nicht angeworfen.

			if(actDrawMode == DANCE) updateParticles(time, dt);

			emitParticles(time, dt);

			//-------------------------------------------------

			//DEBUG switch scenes continously
/*
			if (time - lastDebugSwitchTime > timeToSwitch)
			{
				printf("switch scenes \n");

				std::cout << " debug switch to " << (((int)actDrawMode + 1) % 3) << std::endl;
				switchTo( (drawMode) (((int)actDrawMode + 1) % 3), time);
				lastDebugSwitchTime = time;
			}
*/


			// init values may be false, so time > 2
			if ( time > 2.0
					&& enableInteract
					&& totPixCounter->getResultFloat() > totActSwitchLevel
					&& actDrawMode == PRESENT)
			{
				printf("switch to SELECT because of activity \n");

				for (unsigned int i=0; i<nrSongs; i++){
					icon_scale_offs[i]->setInitVal(0.f);
					icon_scale_offs[i]->setVal(0.f);
				}
				switchTo(SELECT, time);
			}

			// switch back from DANCE to present if there is no activity
			// set request if there is no activity
			if ( enableInteract
					&& !requestSwitchToPresent
					&& totPixCounter->getResultFloat() < totActSwitchLevel
					&& (actDrawMode == DANCE || actDrawMode == SELECT))
			{
				printf("switch to DANCE, no activity \n");

				requestSwitchToPresent = true;
				requestSwitchToPresentTime = time;
			}


			if(requestSwitch != NR_MODES)
			{
				std::cout << "requestSwitch != NR_MODES" << std::endl;

				drawMode lastDrawMode = actDrawMode;
				actDrawMode = requestSwitch;
				requestSwitch = NR_MODES;

				unsigned int songFileNr = 0;
				if (actDrawMode != DANCE){
					songFileNr = (unsigned int)actDrawMode;
				} else {
					songFileNr = actSong + 2;
				}

				// ----- change video ---------------------

				printf(" change video \n");

				// brute force for the moment...
				if (lastDrawMode == PRESENT || lastDrawMode == SELECT){
					std::cout << "stop back" << std::endl;

					vt_back->stop();
					delete vt_back;
					vt_back = NULL;

					printf(" vt_back stopped\n");

				}

				if (lastDrawMode == DANCE){

					std::cout << "stop dance" << std::endl;

					vt_dance->stop();
					delete vt_dance;
					vt_dance = NULL;

					printf(" vt_dance stopped\n");

				}

				// reopen video
				if (actDrawMode != DANCE)
				{
					std::cout << "loading new vt_back video " << std::endl;

					vt_back = new FFMpegDecode();
					vt_back->OpenFile(shCol, (char*) vt_back_paths[actDrawMode].c_str(), 3, back_vid_size.x, back_vid_size.y, false, true);
					vt_back->start();				// start the decoder thread

				} else {

					std::cout << "loading new vt_dance video " << std::endl;

					vt_dance = new FFMpegDecode();
					vt_dance->OpenFile(shCol, (char*) vt_dance_paths[actSong].c_str(), 3, back_vid_size.x, back_vid_size.y, false, true);
					vt_dance->start();
				}

				std::cout << "video stopped" << std::endl;

				// ----- change audio ---------------------

				printf(" vt_dance send osc  all\n");

				//lo_send(scAddr, "/chat", "s", "stopall");

				// change audio
				if (actDrawMode == DANCE)
				{
					lo_send(scAddr, "/chat", "sif", "start", songFileNr, soundHighLevel);
				} else {
					lo_send(scAddr, "/chat", "sif", "start", songFileNr, soundLowLevel);
				}

				if (actDrawMode == DANCE)
				{
					double delay = songFilesAudioDur[songFileNr];
					timeToStopSong = time + delay;
					checkForSongEnd = true;
					//std::cout << "switch to song: at " << time << " stop song at time " << timeToStopSong << std::endl;
				}

				blockIcons = false;
			}

			if (actDrawMode == DANCE)
			{
				if(checkForSongEnd &&  time > timeToStopSong)
				{
					printf(" song end\n");

					for (unsigned int i=0; i<nrSongs; i++)
					{
						icon_scale_offs[i]->setInitVal(0.f);
						icon_scale_offs[i]->setVal(0.f);
					}

					blockIcons = false;
					checkForSongEnd = false;
					switchTo(SELECT, time);
					switchToSelectFromDance = false;
				}
			}


			// stop if activity came back
			if (requestSwitchToPresent
					&& totPixCounter->getResultFloat() > totActSwitchLevel)
			{
				printf(" activity came back\n");

				requestSwitchToPresent = false;
			}

			if (requestSwitchToPresent && (time - requestSwitchToPresentTime) > timeToSwitchDanceIdle)
			{
				printf(" switch to present because of no activity for long  time \n");

				requestSwitchToPresent = false;
				switchTo(PRESENT, time);
			}

		}
	}
}

//----------------------------------------------------

void SNGam_Danza::updateParticles(double time, double dt)
{
	// Invoke the compute shader to integrate the particles
	mParticles->getUpdtShdr()->begin();
	mParticles->getUpdtShdr()->setUniform1f("dt", (float)dt);
	mParticles->getUpdtShdr()->setUniform1f("time", (float)time);
	mParticles->getUpdtShdr()->setUniform1ui("numParticles", (unsigned int)mParticles->getSize());
	mParticles->getUpdtShdr()->setUniform1f("optFlowPosAmt", optFlowPosAmt);
	mParticles->getUpdtShdr()->setUniform1f("optFlowVelAmt", optFlowVelAmt);
	mParticles->getUpdtShdr()->setUniform1i("noiseTex3D", 0);
	mParticles->getUpdtShdr()->setUniform1i("fluidVel", 1);
	mParticles->getUpdtShdr()->setUniform1f("partSpeed", partSpeed);
	mParticles->getUpdtShdr()->setUniform1f("windAmt", windAmt);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mParticles->getNoiseTex());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());

	mParticles->bindBuffers();

	glDispatchCompute((unsigned int)std::max((double)mParticles->getSize()
			/ (double)mParticles->getWorkGroupSize(), 1.0), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	mParticles->unbindBuffers();
	mParticles->getUpdtShdr()->end();
}

//----------------------------------------------------

void SNGam_Danza::emitParticles(double time, double dt)
{
	// emit particles, proc always, to get the total activity from NTreeSorter inside GLSLParticleSystemCS
//			if (time - lastEmit > 0.012)
		// emit particles
		initPars[0] = glm::vec4(0.f, -0.15f, getRandF(0.f, -0.2f), 4.f); // pos: last parameter = lifetime
		initPars[1] = glm::vec4(-getRandF(0.001f, 0.01f), 0.f, 0.f, 0.f);
		initPars[2] = glm::vec4(getRandF((float)-M_PI_4, (float)M_PI_4),
								getRandF((float)-M_PI_4, (float)M_PI_4),
								0.f,
								getRandF(0.f, 9.f));
		initPars[3] = glm::vec4(getRandF(0.001f, 0.03f) * spriteSize, 0.f, 0.f, 0.f); // aux1.x = spriteSize

		mParticles->emit((int)getRandF(0.f, nrPartPerEmit),
				initPars, fblur2nd->getResult(),
				fblurSize, fblurSize); // hereby we do activity estimation at the same time

		// konsumiert super viel cpu, nicht immer ausführen
		//if ( time - lastActUpdtTime > actUpdtInt)
		//{
			//totActivity->update( mParticles->getEmitTexActivity() );
			//std::cout << totActivity->get() << std::endl;
		//	lastActUpdtTime = time;
		//}
//					lastEmit = time;
}

//----------------------------------------------------

void SNGam_Danza::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_1 : debugDrawMode = NONE;
				printf("debugDrawMode = NONE \n");
				break;
			case GLFW_KEY_2 : debugDrawMode = RAW_DEPTH;
				printf("debugDrawMode = RAW_DEPTH \n");
				break;
			case GLFW_KEY_3 : debugDrawMode = TRANS_DEPTH;
				printf("debugDrawMode = TRANS_DEPTH \n");
				break;
			case GLFW_KEY_4 : debugDrawMode = DEPTH_THRESH;
				printf("debugDrawMode = DEPTH_THRESH \n");
				break;
			case GLFW_KEY_5 : debugDrawMode = DEPTH_BLUR;
				printf("debugDrawMode = DEPTH_BLUR \n");
				break;
			case GLFW_KEY_6 : debugDrawMode = OPT_FLOW;
				printf("debugDrawMode = OPT_FLOW \n");
				break;
			case GLFW_KEY_7 : debugDrawMode = OPT_FLOW_BLUR;
				printf("debugDrawMode = OPT_FLOW_BLUR \n");
				break;
			case GLFW_KEY_8 : debugDrawMode = FLUID;
				printf("debugDrawMode = FLUID \n");
				break;

		}
	}
}

//--------------------------------------------------------------------------------

void SNGam_Danza::switchTo(drawMode _mode, double time)
{
	enableInteract = false;

	if (_mode == SELECT)
		for (unsigned int i=0; i<nrSongs; i++)
			icon_scale_offs[i]->setInitVal(0.f);

	// fade out
	blendVal0->setEndFunc([this, time, _mode](){
		std::cout << " requestSwitch from blendVal0 EndFunc " << std::endl;
		requestSwitch = _mode;
	});
	blendVal0->start(1.f, 0.f, fadeOutTime, time, false);


	// fade in
	blendVal1->setEndFunc([this](){
		enableInteract = true;
	});
	blendVal1->setDelay(fadeOutTime);
	blendVal1->start(0.f, 1.f, fadeOutTime, time, false);
}

//--------------------------------------------------------------------------------

void SNGam_Danza::songEndCb()
{
	if (actDrawMode == DANCE){
		switchToSelectFromDance = true;
	}
}

//--------------------------------------------------------------------------------

void SNGam_Danza::loadCalib()
{
	printf("loading calibration \n");
	cv::FileStorage fs(calibFileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{

		fs["fastBlurAlpha"] >> fastBlurAlpha;
		fs["nrPartPerEmit"] >> nrPartPerEmit;
		fs["partSpeed"] >> partSpeed;

		fs["kinRotX"] >> kinRotX;
		fs["kinRotY"] >> kinRotY;
		fs["kinRotZ"] >> kinRotZ;

		fs["kinTransX"] >> kinTransX;
		fs["kinTransY"] >> kinTransY;
		fs["kinTransZ"] >> kinTransZ;

		fs["screenScaleX"] >> screenScaleX;
		fs["screenScaleY"] >> screenScaleY;

		fs["screenTransX"] >> screenTransX;
		fs["screenTransY"] >> screenTransY;

		fs["pointSize"] >> pointSize;
		fs["pointWeight"] >> pointWeight;

		fs["nearThres"] >> nearThres;
		fs["farThres"] >> farThres;

		fs["spriteSize"] >> spriteSize;

		fs["lumaMult"] >> lumaMult;
		fs["lumaThres"] >> lumaThres;

		fs["silRgbFact0R"] >> silRgbFact[0].r;
		fs["silRgbFact0G"] >> silRgbFact[0].g;
		fs["silRgbFact0B"] >> silRgbFact[0].b;
		fs["silRgbFact0A"] >> silRgbFact[0].a;

		fs["silRgbFact1R"] >> silRgbFact[1].r;
		fs["silRgbFact1G"] >> silRgbFact[1].g;
		fs["silRgbFact1B"] >> silRgbFact[1].b;
		fs["silRgbFact1A"] >> silRgbFact[1].a;

		fs["silRgbFact2R"] >> silRgbFact[2].r;
		fs["silRgbFact2G"] >> silRgbFact[2].g;
		fs["silRgbFact2B"] >> silRgbFact[2].b;
		fs["silRgbFact2A"] >> silRgbFact[2].a;

		fs["silRgbFact3R"] >> silRgbFact[3].r;
		fs["silRgbFact3G"] >> silRgbFact[3].g;
		fs["silRgbFact3B"] >> silRgbFact[3].b;
		fs["silRgbFact3A"] >> silRgbFact[3].a;

		fs["silColDepth"] >> silColDepth;


		fs["windAmt"] >> windAmt;

		fs["optFlowPosAmt"] >> optFlowPosAmt;
		fs["optFlowVelAmt"] >> optFlowVelAmt;
		fs["fluidVelTexForce"] >> fluidVelTexForce;
	}

	_hasNewOscValues = true;
}

//--------------------------------------------------------------------------------

void SNGam_Danza::saveCalib()
{
	cv::FileStorage fs(calibFileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{

		fs << "fastBlurAlpha" << fastBlurAlpha;
		fs << "nrPartPerEmit" << nrPartPerEmit;
		fs << "partSpeed" << partSpeed;

		fs << "kinRotX" << kinRotX;
		fs << "kinRotY" << kinRotY;
		fs << "kinRotZ" << kinRotZ;

		fs << "kinTransX" << kinTransX;
		fs << "kinTransY" << kinTransY;
		fs << "kinTransZ" << kinTransZ;

		fs << "screenScaleX" << screenScaleX;
		fs << "screenScaleY" << screenScaleY;

		fs << "screenTransX" << screenTransX;
		fs << "screenTransY" << screenTransY;

		fs << "pointSize" << pointSize;
		fs << "pointWeight" << pointWeight;

		fs << "nearThres" << nearThres;
		fs << "farThres" << farThres;

		fs << "spriteSize" << spriteSize;

		fs << "lumaMult" << lumaMult;
		fs << "lumaThres" << lumaThres;


		fs << "silRgbFact0R" << silRgbFact[0].r;
		fs << "silRgbFact0G" << silRgbFact[0].g;
		fs << "silRgbFact0B" << silRgbFact[0].b;
		fs << "silRgbFact0A" << silRgbFact[0].a;

		fs << "silRgbFact1R" << silRgbFact[1].r;
		fs << "silRgbFact1G" << silRgbFact[1].g;
		fs << "silRgbFact1B" << silRgbFact[1].b;
		fs << "silRgbFact1A" << silRgbFact[1].a;

		fs << "silRgbFact2R" << silRgbFact[2].r;
		fs << "silRgbFact2G" << silRgbFact[2].g;
		fs << "silRgbFact2B" << silRgbFact[2].b;
		fs << "silRgbFact2A" << silRgbFact[2].a;

		fs << "silRgbFact3R" << silRgbFact[3].r;
		fs << "silRgbFact3G" << silRgbFact[3].g;
		fs << "silRgbFact3B" << silRgbFact[3].b;
		fs << "silRgbFact3A" << silRgbFact[3].a;

		fs << "silColDepth" <<  silColDepth;
		fs << "windAmt" << windAmt;


		fs << "optFlowPosAmt" << optFlowPosAmt;
		fs << "optFlowVelAmt" << optFlowVelAmt;
		fs << "fluidVelTexForce" << fluidVelTexForce;
	}
}

//----------------------------------------------------

SNGam_Danza::~SNGam_Danza()
{
	delete rawQuad;
	delete rotateQuad;
	delete threshFbo;
}

}
