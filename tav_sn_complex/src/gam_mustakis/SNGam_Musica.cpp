/*

* 	SNGam_Musica.cpp
*   tav
*
*   Created by Sven Hahne on 19.08.14.
*   Copyright (c) 2014 Sven Hahne. All rights reserved.
*
*
*	Guitar Hero aehnliches Spiel
*
*	Noten rotieren auf einem Zylinder, wenn die Noten in einem bestimmten Bereich ist,
*	muss der Benutzer eine Taste druecken, was zur selben Zeit ist, zu der die Note zu
*	hoeren ist. Sind beide Ereignisse synchron wird der Benutzer mit einer schoenen Grafik
*	belohnt.
*
*	Es laufen Songs, von denen jeweils 4 Instrumente transkriebiert wurden. Diese muessen
*	als MusicXML vorliegen.
*
*	Pro Instrument gibt es eine Rotationsmatrize die auf der y-achse verschiebt und um ein
*	vielfaches von 45 grad dreht.
*
*	Die Zylinder-Klasse erzeugt standarmaessig einen Zylinder, der parallel zur y-achse
*	liegt (die "deckel"-flaechen in der x,z ebene)
*
*	Die "rotierenden" Basis zylinder werden zuerst um 90 Grad um die x-achse und dann um
*	90 Grad um die y-achse gedreht. Zuletzt mit der Rotationsmatrize des entsprechenden
*	Instrumentes multipliziert.
*
*	Noten werden immer am zur Kamera abgewandeten Seite eines Basis-("Track-")Zylinders
*	emittiert (per GLSLParticlesystem verwaltet). In der Variable "noteEmitDelay" wird die
*	Zeit gespeichert die eine Note braucht um von der unteren Seite des Trackzylinders zur
*	oberen Seite zu gelangen. Wenn das Abspielen eines Track angefordert wird, startet
*	zuerst der MusicXML-Player und nach "noteEmitDelay", der Audio-Player.Die Koordinierung
*	dieses Mechanismus passiert in der Methode "playSong".
*
*	Dem MusicXML Player wird eine Callback-Funktion übergeben, die bei jeder Note aufgerufen
*	wird. Parameter sind trackNr, pitch, duration. Bei jeder Note wird im entsprechenden
*	Noten-Partikelsystem eine Note (Partikel) emittiert. Die "lifetime" einer Note ist
*	genau 2 * noteEmitDelay (laeuft theoretisch einmal komplett um den Track-zylinder)
*
*/


#include "SNGam_Musica.h"

#define STRINGIFY(A) #A


namespace tav
{

SNGam_Musica::SNGam_Musica(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
SceneNode(_scd, _sceneArgs),

actPlayState(SGNM_SELECT_DIALOG),
nrInstruments(4),
nrSongs(6),
nrLinesPerInst(5),
glowFboSize(1024),
nrLinePoints(200),

inited(false),
playMidi(false),
typoInited(false),
playTrackRequest(false),
wantInitPlay(true),
requestPlayInitSong(false),
songPtr(0),
idleVideo(NULL),

noteEmitDelay(3.f), // = speed
noteMaxDelayToCatch(0.4f),

spriteSize(0.01f),
playerMatYOffs(1.41421356237f),
lastPaFrame(-1),

debugInputTime(0.0),
debugInputTimeInt(12.0)
{
	pa = static_cast<PAudio*>(scd->pa);
//	sndPlayer = static_cast<SoundFilePlayer*>(_scd->soundFilePlayer);

    scAddr = lo_address_new("127.0.0.1", "57120");


	shCol = (ShaderCollector*)_scd->shaderCollector;
	winMan = static_cast<GWindowManager*>(scd->winMan);
	fonts = static_cast<FreeTypeFont**>(scd->ft_fonts);
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	// ------------------

	baseCol = new glm::vec4[nrInstruments];
	for (int i=0; i<nrInstruments; i++)
		for (int j=0; j<4; j++)
			baseCol[i][j] = scd->colors[static_cast<unsigned int>(sceneArgs->at("col" + std::to_string(i)))][j];

	// setup model matrices for each particle system
	playerModelMat = new glm::mat4[nrInstruments];
	cubeModelNormalMat = new glm::mat3[nrInstruments];

	// ------- complex inits -----------

	initSongData();
	initOscPar();
	initDialogWindow(_sceneArgs);
	initParticleSystems();
	initFbos();
	initTextures();
	initGeometry();
	initContrButtMap();

	// ------- Shaders -----------------

	initAudioWaveShdr();
	initAlphaCutShader();
	initFadeShader();
    initNoteDrawShdr();
    initTrackCylGlowShader();
    initTrackCylShader();
    initPartDrawShdr();
    initSuccessPartDrawShdr();
    stdCol = shCol->getStdCol();
    stdParCol = shCol->getStdParCol();
    stdTex = shCol->getStdTex();

    // trackCylinderLightDir
    lightDir = glm::vec3(-1.f, 0.f, 1.f);
    trackCylModelMat = new glm::mat4[nrInstruments];

	// create ubo and initialize it with the structure data
	glGenBuffers(1, &mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);

	glow = new FastBlurMem(0.6f, shCol, glowFboSize, glowFboSize, GL_RGBA8, false, FastBlurMem::KERNEL_5);

	idleVideoSize.x = 1.0;
	idleVideoSize.y = 1.0;

	calibFileName = (*scd->dataPath)+"calib_cam/gam_musica.yml";
	if (access(calibFileName.c_str(), F_OK) != -1)
		loadCalib();



	//idleVideo = new VideoTextureCv((char*)((*_scd->dataPath)+"/movies/video_leche.mp4").c_str());
	idleVideo = new FFMpegDecode();
	idleVideo->OpenFile(shCol, (char*)((*_scd->dataPath)+"/movies/video_leche.mp4").c_str(), 3,
    		1280, 800, true, true);
	idleVideo->start();

	std::cout << "----- start leche" << std::endl;

    lo_send(scAddr, "/chat", "si", "play", 0);
}

//----------------------------------------------------

void SNGam_Musica::initContrButtMap()
{
    // byte 0 - 4, bit 0 - 8 (right to left)
	controlerSet = new PiContrButtSet*[nrInstruments];
	for (unsigned int i=0; i< nrInstruments; i++)
		controlerSet[i] = new PiContrButtSet[nrLinesPerInst];

    // first set viewn from where the machine is placed
    // going counter clockwise
    // from left to right, user side
    controlerSet[0][0] = PiContrButtSet(1, 0);
    controlerSet[0][1] = PiContrButtSet(1, 4);
    controlerSet[0][2] = PiContrButtSet(1, 1);
    controlerSet[0][3] = PiContrButtSet(1, 2);
    controlerSet[0][4] = PiContrButtSet(1, 3);

    controlerSet[1][0] = PiContrButtSet(2, 2);
    controlerSet[1][1] = PiContrButtSet(2, 5);
    controlerSet[1][2] = PiContrButtSet(2, 6);
    controlerSet[1][3] = PiContrButtSet(2, 3);
    controlerSet[1][4] = PiContrButtSet(2, 4);

    controlerSet[2][0] = PiContrButtSet(3, 3);
    controlerSet[2][1] = PiContrButtSet(3, 1);
    controlerSet[2][2] = PiContrButtSet(3, 0);
    controlerSet[2][3] = PiContrButtSet(3, 2);
    controlerSet[2][4] = PiContrButtSet(3, 4);

    controlerSet[3][0] = PiContrButtSet(2, 1);
    controlerSet[3][1] = PiContrButtSet(1, 5);
    controlerSet[3][2] = PiContrButtSet(1, 6);
    controlerSet[3][3] = PiContrButtSet(2, 0);
    controlerSet[3][4] = PiContrButtSet(1, 7);


    // start udp listen loop (FIREWALL CHECKEN!!!)
    client = new UdpClient(6024);
	client->setCb( std::bind( &SNGam_Musica::udpCb, this, std::placeholders::_1 ) );
    client->startThread();
}

//----------------------------------------------------

void SNGam_Musica::initSongData()
{
	songFilesXml = new std::string[nrSongs];
	songFilesXml[0] = ((*scd->dataPath)+"/musicXml/GAM_Midi_ElViajeRedondo_sven.xml");
	songFilesXml[1] = ((*scd->dataPath)+"/musicXml/GAM_Midi_Leche_sven.xml");
	songFilesXml[2] = ((*scd->dataPath)+"/musicXml/GAM_Midi_Pajarito_sven.xml");
	songFilesXml[3] = ((*scd->dataPath)+"/musicXml/GAM_Midi_Primavera_sven.xml");
	songFilesXml[4] = ((*scd->dataPath)+"/musicXml/GAM_Midi_trompetin_sven.xml");
	songFilesXml[5] = ((*scd->dataPath)+"/musicXml/GAM_Midi_UnRayoDeSol_sven.xml");


	songFilesAudio = new std::string[nrSongs];
	songFilesAudio[0] = ((*scd->dataPath)+"audio/ElViajeRedondo_Edit_1807.wav");
	songFilesAudio[1] = ((*scd->dataPath)+"audio/Leche_Edit_1807.wav");
	songFilesAudio[2] = ((*scd->dataPath)+"audio/Pajarito_Edit_1807.wav");
	songFilesAudio[3] = ((*scd->dataPath)+"audio/GAM_Midi_Primavera_sven.wav");
	songFilesAudio[4] = ((*scd->dataPath)+"audio/Trompetin_Edit_1807.wav");
	songFilesAudio[5] = ((*scd->dataPath)+"audio/RayoDeSol_Edit_1807.wav");

	songNames = new std::string[nrSongs];
	songNames[0] = ("El viaje redondo");
	songNames[1] = ("Leche");
	songNames[2] = ("Pajarito");
	songNames[3] = ("Primavera");
	songNames[4] = ("Trompetin");
	songNames[5] = ("Un Rayo de Sol");

	songDuration = new double[nrSongs];
	songDuration[0] = 61.5; // viaje redondo
	songDuration[1] = 60.0; // leche
	songDuration[2] = 61.2; // pajarito
	songDuration[3] = 80.0; // primavera
	songDuration[4] = 57.0; // primavera
	songDuration[5] = 64.0; // primavera

	iconSet = new iconNames*[nrSongs];
	for (unsigned int i=0; i<nrSongs; i++)
		iconSet[i] = new iconNames[nrInstruments];

	// El Viaje Redondo
	iconSet[0][0] = ICON_BAJO;
	iconSet[0][1] = ICON_GUITARRA;
	iconSet[0][2] = ICON_BOMBO;
	iconSet[0][3] = ICON_BONGO;


	// Leche
	iconSet[1][0] = ICON_BAJO;
	iconSet[1][1] = ICON_GUITARRA;
	iconSet[1][2] = ICON_HATZ;
	iconSet[1][3] = ICON_BOMBO;

	// Pjarito
	iconSet[2][0] = ICON_BAJO;
	iconSet[2][1] = ICON_GUITARRA;
	iconSet[2][2] = ICON_GUITARRA;
	iconSet[2][3] = ICON_FLAUTA;

	// Primavera
	iconSet[3][0] = ICON_BAJO;
	iconSet[3][1] = ICON_GUITARRA;
	iconSet[3][2] = ICON_CLAVE;
	iconSet[3][3] = ICON_BONGO;

	// Trompetin
	iconSet[4][0] = ICON_BAJO;
	iconSet[4][1] = ICON_BOMBO;
	iconSet[4][2] = ICON_HATZ;
	iconSet[4][3] = ICON_GUITARRA;

	// UnRayo De Sol
	iconSet[5][0] = ICON_BAJO;
	iconSet[5][1] = ICON_BOMBO;
	iconSet[5][2] = ICON_BONGO;
	iconSet[5][3] = ICON_CAJA;

	// make an array saving a last cuple of notes
	actNotes = new std::vector<double>*[nrInstruments];
	for (unsigned int i=0; i<nrInstruments; i++)
		actNotes[i] = new std::vector<double>[nrLinesPerInst];


	/*
//	sndPlayer->open_file(((*scd->dataPath)+"audio/ElViajeRedondo_Edit_1807_cut.wav").c_str());
	sndPlayer->open_file(((*scd->dataPath)+"audio/audio_leche.wav").c_str());
	sndPlayer->setRouteToIn(true);
	sndPlayer->setVolume(1.f);
	sndPlayer->setStopCallback(std::bind(&SNGam_Musica::idleSongEndCb, this), 0.0);
	sndPlayer->play();
*/
}

//----------------------------------------------------

void SNGam_Musica::initGeometry()
{
	trackCylinder = new Cylinder(40);
	rawQuad = scd->stdQuad;
	hFlipQuad = scd->stdHFlipQuad;

	// quad prototype for ampliying geometry in vertex shader
	testVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat pos[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
	testVAO->upload(POSITION, pos, 4);
	GLuint ind[] = { 0, 1, 3, 1, 3, 2 };
	testVAO->setElemIndices(6, ind);

	// line grid
	lineVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat linePos[nrLinePoints *2 *3];

	for (unsigned int i=0; i<nrLinePoints; i++){
		float fInd = static_cast<float>(i) / static_cast<float>(nrLinePoints-1);
		for (unsigned int j=0; j<2; j++){
			linePos[(i*2 +j) *3] = fInd * 2.f - 1.f;
			linePos[(i*2 +j) *3 +1] = 0.f;
			linePos[(i*2 +j) *3 +2] = 0.f;
		}
	}
	lineVAO->upload(POSITION, linePos, nrLinePoints*2);
}

//----------------------------------------------------

void SNGam_Musica::initOscPar()
{
	addPar("alTwirlAmt", &alTwirlAmt);
	addPar("alLineThick", &alLineThick);
	addPar("alAmp", &alAmp);
	addPar("alAlphaCut", &alAlphaCut);
	addPar("alFdbk", &alFdbk);
	addPar("alAlpha", &alAlpha);
	addPar("alInstTwirlOffs", &alInstTwirlOffs);


	addPar("instIconSize", &instIconSize);
	addPar("instIconPos", &instIconPos);

	addPar("noteRadius", &noteRadius);
	addPar("noteHeight", &noteHeight);
	addPar("noteBrightness", &noteBrightness);

	addPar("glowAlpha", &glowAlpha);
	addPar("glowBright", &glowBright);
	addPar("glowOffsScale", &glowOffsScale);
	addPar("glowNrBlurs", &glowNrBlurs);

	addPar("spriteSize", &spriteSize);

	addPar("partFdbk", &partFdbk);
	addPar("partAlpha", &partAlpha);
	addPar("partAlphaCut", &partAlphaCut);
	addPar("partBright", &partBright);
	addPar("partSizeX", &partSizeX);
	addPar("partSizeY", &partSizeY);
	addPar("partLifeTime", &partLifeTime);
	addPar("partSpeed", &partSpeed);
	addPar("partWindAmt", &partWindAmt);
	addPar("partForceCenterAmt", &partForceCenterAmt);
	addPar("partRotForce", &partRotForce);
	addPar("partRotAngleSpeed", &partRotAngleSpeed);

	addPar("partGlowAlpha", &partGlowAlpha);
	addPar("partGlowBright", &partGlowBright);
	addPar("partGlowOffsScale", &partGlowOffsScale);
	addPar("partGlowNrBlurs", &partGlowNrBlurs);

	addPar("spEmitXRand", &spEmitXRand);
	addPar("spEmitYRand", &spEmitYRand);
	addPar("spRandSpeed", &spRandSpeed);
	addPar("spAlpha", &spAlpha);
	addPar("spFdbk", &spFdbk);
	addPar("spAlphaCut", &spAlphaCut);
	addPar("spSpriteSize", &spSpriteSize);

	addPar("trackBlackThres", &trackBlackThres);
	addPar("trackCylWidth", &trackCylWidth);
	addPar("trackCylDepth", &trackCylDepth);
	addPar("trackCylHeight", &trackCylHeight);
	addPar("trackCylYOffs", &trackCylYOffs);
	addPar("trackCylZOffs", &trackCylZOffs);
	addPar("trackCylInterSpace", &trackCylInterSpace);

	addPar("trackCylAngle0", &trackCylAngle0);
	addPar("trackCylAngle1", &trackCylAngle1);
	addPar("trackCylAngle2", &trackCylAngle2);
	addPar("trackCylAngle3", &trackCylAngle3);

	addPar("lightDirZ", &lightDir.z);

	addPar("diagWinOffsX", &diagWinOffsX);
	addPar("diagWinOffsY", &diagWinOffsY);

	addPar("diagWinSizeX", &diagWinSize.x);
	addPar("diagWinSizeY", &diagWinSize.y);

	addPar("idleInstIconSize", &idleInstIconSize);
	addPar("idleInstIconPos", &idleInstIconPos);

	addPar("idleVideoSizeX", &idleVideoSize.x);
	addPar("idleVideoSizeY", &idleVideoSize.y);


	addPar("idleVideonOffsX", &idleVideonOffsX);



}

//----------------------------------------------------

void SNGam_Musica::initDialogWindow(std::map<std::string, float>* _sceneArgs)
{
	/*

	diagWinSize = glm::vec2(0.6f, 0.6f);
	diagWinSizePad = 0.1f;
	diagWinHeadlHeight = 0.1f;
	diagWinHeadlBlockSpace = 0.05f;

    // Fonts
	if ( _sceneArgs->find("font1") != _sceneArgs->end() )
	{
		diagHead = new NVTextBlock(shCol, fonts[ static_cast<int>(_sceneArgs->at("font1")) ], 28.f, 4 );
		diagHead->setSize(diagWinSize.x - diagWinSizePad, diagWinHeadlHeight);
		diagHead->setPos(0.f, diagWinSize.y * 0.5f - diagWinSizePad);
		diagHead->setBackColor(0.f, 0.f, 0.f, 0.f);
		diagHead->setAlign(ALIGN_LEFT);
		diagHead->setTextColor(1.f, 1.f, 1.f, 1.f);
		diagHead->setLineHeight(20.f);
		diagHead->setTextStroke(false);
	} else {
		std::cout << "SNTestNVFonts font0 definition  not found in setup xml" << std::endl;
	}

	if ( _sceneArgs->find("font0") != _sceneArgs->end() )
	{
		diagSongs = new NVTextBlock(shCol, fonts[ static_cast<int>(_sceneArgs->at("font0")) ], 24.f, 4 );
		diagSongs->setSize(diagWinSize.x - diagWinSizePad, diagWinSize.y - diagWinSizePad);
		diagSongs->setPos(0.f, -diagWinHeadlHeight);
		diagSongs->setBackColor(0.f, 0.f, 0.f, 0.f);
		diagSongs->setAlign(ALIGN_LEFT);
		diagSongs->setTextColor(1.f, 1.f, 1.f, 1.f);
		diagSongs->setLineHeight(26.f);
		diagSongs->setTextStroke(false);
	} else {
		std::cout << "SNTestNVFonts font0 definition  not found in setup xml" << std::endl;
	}

	diagWinSongLineHeight = diagSongs->getFontSize() * 0.001f;
	*/
}

//----------------------------------------------------

void SNGam_Musica::initParticleSystems()
{
    notesPartSys = new GLSLParticleSystemCS*[nrInstruments];
    for(int i=0; i<nrInstruments; i++){
    	notesPartSys[i] = new GLSLParticleSystemCS(1<<5, shCol, GLSLParticleSystemCS::DRAW_POINT, false);
    	notesPartSys[i]->addBuffer("vel");
    	notesPartSys[i]->addBuffer("aux0");
    	notesPartSys[i]->zero();	// all buffers to zero
    	notesPartSys[i]->setUpdateShader();
    	notesPartSys[i]->setEmitShader();
    }

    linePart = new GLSLParticleSystemCS*[nrInstruments];
    for(int i=0; i<nrInstruments; i++){
    	linePart[i] = new GLSLParticleSystemCS(1<<14, shCol, GLSLParticleSystemCS::DRAW_GEN_QUADS, true);
    	linePart[i]->addBuffer("vel");
    	linePart[i]->addBuffer("aux0");
    	linePart[i]->zero();	// all buffers to zero
    	linePart[i]->setUpdateShader(getUpdtShader().c_str());
    	linePart[i]->setEmitShader();
    }

    succesPart = new GLSLParticleSystemCS(1<<14, shCol, GLSLParticleSystemCS::DRAW_GEN_QUADS, true);
    succesPart->addBuffer("vel");
    succesPart->addBuffer("aux0");
    succesPart->zero();	// all buffers to zero
    succesPart->setUpdateShader(getSuccessPartUpdtShader().c_str());
   	succesPart->setEmitShader(true);

	initPars = new glm::vec4[3];	// for emitting, pos, vel, aux0
	initRand = new glm::vec4[3];
}

//----------------------------------------------------

void SNGam_Musica::initFbos()
{
	glowFbo = new FBO(shCol, glowFboSize, glowFboSize);
	glowFbo->setMagFilter(GL_LINEAR);
	glowFbo->setMinFilter(GL_LINEAR);

	std::cout << " partfbo: " << scd->screenWidth << ", " << scd->screenHeight << std::endl;


	partFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);
	partFbo->clear();

	spFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight, GL_RGBA8,
			GL_TEXTURE_2D, false, 1, 1, 2, GL_CLAMP_TO_BORDER, false);
	spFbo->clear();

	alFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight, GL_RGBA8,
			GL_TEXTURE_2D, false, 1, 1, 2, GL_CLAMP_TO_BORDER, false);
	alFbo->clear();
}

//----------------------------------------------------

void SNGam_Musica::initTextures()
{
	// iconos
	instIcons = new TextureManager[NUM_INSTR_ICONS];
	instIcons[ICON_BAJO].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/bajo.png", 2);
	instIcons[ICON_BOMBO].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/bombo.png", 2);
	instIcons[ICON_BONGO].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/bongos.png", 2);
	instIcons[ICON_CAJA].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/caja.png", 2);
	instIcons[ICON_CASCABEL].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/cascabel.png", 2);
	instIcons[ICON_CLAVE].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/clave.png", 2);
	instIcons[ICON_CONTRABAJO].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/contrabajo.png", 2);
	instIcons[ICON_FLAUTA].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/flauta.png", 2);
	instIcons[ICON_GUITARRA].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/guitarra.png", 2);
	instIcons[ICON_HATZ].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/hatz.png", 2);
	instIcons[ICON_MANO].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/mano.png", 2);
	instIcons[ICON_MARIMBA].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/marimba.png", 2);
	instIcons[ICON_TOM].loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/musica/iconos/tom.png", 2);

    dialogTex.loadTexture2D((*scd->dataPath)+"textures/gam_mustakis/idle_loop_gfx.png");

    audioTex = new AudioTexture(shCol, nrInstruments, pa->getFramesPerBuffer());
}

//----------------------------------------------------

void SNGam_Musica::initFadeShader()
{
	std::string shdr_Header = shCol->getShaderHeader();

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	\n
	out vec2 tex_coord;\n
	\n
	void main() {
		tex_coord = texCoord;
		gl_Position = position;
	});

	vert = shdr_Header +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;\n
	\n
	in vec2 tex_coord;\n
	uniform float alphaCut;\n
	uniform sampler2D tex;\n
	uniform sampler2D glow;\n
	\n
	void main() {\n
		vec4 col = texture(tex, tex_coord);
		vec4 glow = texture(glow, tex_coord);
		if (col.a < alphaCut) col = vec4(0.0);
		fragColor = mix(glow, col, col.a);
	});

	frag = "// SNGam_Musica fade shader frag\n"+shdr_Header+frag;

	fadeShdr = shCol->addCheckShaderText("SNGam_Musica_fadeShdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Musica::initAlphaCutShader()
{
	std::string shdr_Header = shCol->getShaderHeader();

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;\n
	layout( location = 2 ) in vec2 texCoord;\n
	\n
	out vec2 tex_coord;\n
	\n
	void main() {
		tex_coord = texCoord;
		gl_Position = position;
	});

	vert = shdr_Header +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;\n
	\n
	in vec2 tex_coord;\n
	uniform float alphaCut;\n
	uniform sampler2D tex;\n
	\n
	void main() {\n
		vec4 col = texture(tex, tex_coord);
		col.a *= length(col.xyz);
		if (col.a < alphaCut) discard;
		fragColor = col;
	});

	frag = "// SNGam_Musica fade shader frag\n"+shdr_Header+frag;

	alphaCutShdr = shCol->addCheckShaderText("SNGam_Musica_alphaCutShdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Musica::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	m_pv = cp->projection_matrix_mat4 * cp->view_matrix_mat4;

	float cylAngles[nrInstruments];
	cylAngles[0] = trackCylAngle0;
	cylAngles[1] = trackCylAngle1;
	cylAngles[2] = trackCylAngle2;
	cylAngles[3] = trackCylAngle3;


	// setup matrices for each pĺayer
    for(unsigned int i=0; i<nrInstruments; i++)
    {
    	playerModelMat[i] = glm::mat4(1.f);	// init matrix
        // Rotate around z axis
    	playerModelMat[i] = glm::rotate(playerModelMat[i], cylAngles[i], glm::vec3(0.f, 0.f, 1.f));
//    	playerModelMat[i] = glm::rotate(playerModelMat[i], float(M_PI) * (0.5f * float(i) + 0.25f), glm::vec3(0.f, 0.f, 1.f));
    	// translate matrices to edge of normalized quad
    	playerModelMat[i] = glm::translate(playerModelMat[i], glm::vec3(0.f, playerMatYOffs, 0.f));

    	cubeModelNormalMat[i] = glm::mat3( glm::transpose( glm::inverse( playerModelMat[i] ) ) );
    }

	glDepthMask(true);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	if (actPlayState != SGNM_SELECT_DIALOG)
	{
		drawParticles(cp, time);
		drawTrackCylinders(cp, 0);
		drawTrackCylinderGlow(cp);
		drawTrackCylinders(cp, 1);
	}

	if (musicXMLReader.playing())
		drawNotesPartSys(cp, time);		// draw the notes

	if (actPlayState != SGNM_SELECT_DIALOG){
		drawSuccessParticles(cp, time);
		drawIcons();
		drawAudioWave(time);
	}


	// show selection dialog
	if (actPlayState == SGNM_SELECT_DIALOG) drawDialogWin(cp);

}

//----------------------------------------------------

void SNGam_Musica::initTrackCylGlowShader()
{
	std::string vert = "uniform int buttPressed["+std::to_string(nrLinesPerInst)+"];\n";

	vert += STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec3 normal;\n
		\n
		out block {\n
			float hide;\n
		} vertex_out;\n
		\n
		uniform float cylWidth;\n
		uniform float cylInterSpace;\n
		uniform uint nrLinesPerCyl;\n

		uniform mat4 m_pv;\n
		uniform mat4 m_model;\n
		uniform mat4 m_playerModel;\n
		\n
		void main()\n
		{\n
			// transform the normal, without perspective, and normalize it
			vertex_out.hide = (normal.y != 0.0 || buttPressed[gl_InstanceID] == 0) ? 1.0 : 0.0;

			vec4 modTrack = m_model * position;\n

			// offset by instanceid
			modTrack.x += cylWidth * (float(gl_InstanceID) - float(nrLinesPerCyl) * 0.5)
						+ cylInterSpace * (float(gl_InstanceID) - float(nrLinesPerCyl-1) * 0.5);\n

			gl_Position = m_pv * m_playerModel * modTrack;\n
		});

	vert = "// SNGam_Musica directional light piste, vert\n" + shCol->getShaderHeader() + vert;


	std::string frag = STRINGIFY(
		uniform vec3 trackCol;\n
		\n
		in block {\n
			float hide;\n
		} vertex_in;\n
		\n
		float pi = 3.1415926535897932384626433832795;
		\n
		out vec4 gl_FragColor;\n
		\n
		void main() {\n
			if (vertex_in.hide > 0.0) discard;
			gl_FragColor = vec4(trackCol, 1.0);\n
		});


	frag = "// SNGam_Musica directional light piste frag\n" + shCol->getShaderHeader() + frag;

	trackCylGlowShdr = shCol->addCheckShaderText("SNGam_Musica_track_glow_shdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Musica::drawTrackCylinderGlow(camPar* cp)
{
	glowFbo->bind();
	glowFbo->clear();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_CULL_FACE);

	trackCylGlowShdr->begin();
	trackCylGlowShdr->setUniformMatrix4fv("m_pv", &m_pv[0][0]);
	trackCylGlowShdr->setUniform1f("cylWidth", trackCylWidth);
	trackCylGlowShdr->setUniform1f("cylInterSpace", trackCylInterSpace);
	trackCylGlowShdr->setUniform1ui("nrLinesPerCyl", nrLinesPerInst);

	for(unsigned int i=0; i<nrInstruments; i++)
	{
		// make temporary int array with button states
		int buttState[nrLinesPerInst];
		bool nonePressed = true;
		for(unsigned int j=0; j<nrLinesPerInst; j++)
		{
			buttState[j] = (int)controlerSet[i][j].pressed;
			if (buttState[j]) nonePressed = false;
		}

		if(!nonePressed)
		{
			unsigned int colInd = (-i + nrInstruments) % nrInstruments;
			trackCylGlowShdr->setUniform1iv("buttPressed", &buttState[0], nrLinesPerInst);
			trackCylGlowShdr->setUniform3f("trackCol", baseCol[colInd].r, baseCol[colInd].g, baseCol[colInd].b);
			trackCylGlowShdr->setUniformMatrix4fv("m_playerModel", &playerModelMat[i][0][0]);
			trackCylGlowShdr->setUniformMatrix4fv("m_model", &trackCylModelMat[i][0][0]);
			trackCylinder->drawElementsInst(GL_TRIANGLES, nrLinesPerInst);
		}
	}

	trackCylGlowShdr->end();

	glowFbo->unbind();

	// ------- proc blurs ----------------

	glow->setAlpha(glowAlpha);
	glow->setBright(glowBright);
	glow->setOffsScale(glowOffsScale);
	glow->proc(glowFbo->getColorImg());
	for (unsigned short i=0; i<(unsigned int)glowNrBlurs; i++)
		glow->proc(glow->getResult(), i);

	// ------- draw result ----------------

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glow->getResult());

    rawQuad->draw();

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

}

//----------------------------------------------------

void SNGam_Musica::initTrackCylShader()
{
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec3 normal;\n
		\n
		out block {\n
			vec3 normal;\n
			vec4 modPos;\n
		} vertex_out;\n
		\n
		uniform float cylWidth;\n
		uniform float cylInterSpace;\n
		uniform uint nrLinesPerCyl;\n

		uniform mat4 m_pv;\n
		uniform mat4 m_model;\n
		uniform mat4 m_playerModel;\n
		\n
		void main()\n
		{\n
			// transform the normal, without perspective, and normalize it
			vertex_out.normal = normal;\n
			vec4 modTrack = m_model * position;\n

			// offset by instanceid
			modTrack.x += cylWidth * (float(gl_InstanceID) - float(nrLinesPerCyl) * 0.5)
						+ cylInterSpace * (float(gl_InstanceID) - float(nrLinesPerCyl-1) * 0.5);\n
			vertex_out.modPos = m_playerModel * modTrack; \n

			gl_Position = m_pv * vertex_out.modPos;\n
		});

	vert = "// SNGam_Musica directional trackCylShdr, vert\n" + shCol->getShaderHeader() + vert;


	std::string frag = STRINGIFY(
		uniform vec4 diffuse;\n
		uniform vec3 lightDirection;\n 	// direction toward the light
		uniform vec3 halfVector;\n		// surface orientation for shiniest spots
		uniform float shininess;\n		// exponent for sharping highlights
		uniform float strength;\n		// extra factor to adjust shininess
		uniform float blackThres;\n		// radius (+/- from 0 to paint the cylinder black)
		uniform int drawTop;\n
		\n
		in block {\n
			vec3 normal;\n // interpolate the normalized surface normal
			vec4 modPos;\n // interpolate the normalized surface normal
		} vertex_in;\n
		\n
		float pi = 3.1415926535897932384626433832795;
		\n
		out vec4 gl_FragColor;\n
		\n
		void main() {\n

			if (drawTop == 1 && abs(vertex_in.normal.y) > 0.9) discard;
			if (drawTop == 0 && abs(vertex_in.normal.y) < 0.9) discard;

			float lightColor = 1.0;
			float ambientLight = 0.05;
			\n
			// compute cosine of the directions, using dot products,
			// to see how much light would be reflected
			float diffuseAmt = max(0.0, dot(vertex_in.normal, lightDirection));\n
			float specular = max(0.0, dot(vertex_in.normal, halfVector));\n
			\n
			// surfaces facing away from the light (negative dot products)
			// won’t be lit by the directional light
			if (diffuseAmt == 0.0)\n
				specular = 0.0;\n
			else
				specular = pow(specular, shininess);\n	// sharpen the highlight
			\n
			vec3 scatteredLight = vec3(ambientLight) + vec3(diffuse) * diffuseAmt;\n
			vec3 reflectedLight = vec3(lightColor) * specular * strength;\n
			\n
			// calculate cylinder phase from normal, cap normals have y = 1.0, make them always "white"
			// if we are dealing with cylinder body normal, get the cylinder phase from their x,z coordinates
			// phase goes from -pi to +pi, we are assigning black and white inverted since the phase 0
			// in this case lies at the bottom of the transformed cylinder seen from the camera
			float toBlackFact = vertex_in.normal.y > 0.9 ? 1.0
					: abs(atan(vertex_in.normal.z, vertex_in.normal.x)) < (blackThres * pi) ? 1.0 : 0.0;
			\n
			// don’t modulate the underlying color with reflected light,
			// only with scattered light
			vec3 rgb = min(vec3(toBlackFact) * scatteredLight + reflectedLight * toBlackFact + vec3(ambientLight), vec3(1.0));\n
			gl_FragColor = vec4(rgb, 1.0);\n
		});


	frag = "// SNGam_Musica directional trackCylShdr frag\n" + shCol->getShaderHeader() + frag;

	trackCylShdr = shCol->addCheckShaderText("SNGam_Musica_piste", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNGam_Musica::drawTrackCylinders(camPar* cp, int drawTop)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_CLAMP);
	glDepthMask(GL_TRUE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	trackCylShdr->begin();
	trackCylShdr->setUniformMatrix4fv("m_pv", &m_pv[0][0]);
	trackCylShdr->setUniform4f("diffuse", 1.f, 1.f, 1.f, 1.f);
	trackCylShdr->setUniform3fv("lightDirection", &lightDir[0]); 	// direction toward the light
	trackCylShdr->setUniform3fv("halfVector", &lightDir[0]);		// surface orientation for shiniest spots
	trackCylShdr->setUniform1f("shininess", 70.f);		// exponent for sharping highlights
	trackCylShdr->setUniform1f("strength", 0.5f);		// extra factor to adjust shininess
	trackCylShdr->setUniform1f("blackThres", trackBlackThres);		// extra factor to adjust shininess
	trackCylShdr->setUniform1f("cylWidth", trackCylWidth);
	trackCylShdr->setUniform1f("cylInterSpace", trackCylInterSpace);
	trackCylShdr->setUniform1ui("nrLinesPerCyl", nrLinesPerInst);
	trackCylShdr->setUniform1i("drawTop", drawTop);

	for(unsigned int i=0; i<nrInstruments; i++)
	{
		trackCylShdr->setUniformMatrix4fv("m_playerModel", &playerModelMat[i][0][0]);
		trackCylShdr->setUniformMatrix4fv("m_model", &trackCylModelMat[i][0][0]);
		trackCylinder->drawElementsInst(GL_TRIANGLES, nrLinesPerInst);
	}

	trackCylShdr->end();
}

//----------------------------------------------------

void SNGam_Musica::initNoteDrawShdr()
{
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec3 normal;\n

		layout( std140, binding=1 ) buffer Pos { vec4 pos[]; };\n
		layout( std140, binding=2 ) buffer Aux { vec4 aux0[]; };\n
		\n
		out block {\n
			float lifeTime;\n
			vec3 normal;\n // interpolate the normalized surface normal
			vec4 modPos;\n // interpolate the normalized surface normal
			vec3 color;\n // interpolate the normalized surface normal
		} vertex_out;\n
		\n
		//uniform float cylWidth;\n
		uniform float cylInterSpace;\n
		uniform vec3 cylOffs;\n
		uniform vec3 cylScale;\n
		uniform float noteEmitDelay;\n
		uniform float nrNotesPerTrack;\n
		uniform float nrLinesPerInst;\n
		uniform float actTime;\n
		uniform float debugPhase;\n
		uniform float blackThres;\n		// extra factor to adjust shininess
		uniform float noteWidthPhaseOffs;\n		// extra factor to adjust shininess
		\n
		uniform mat4 m_pv;\n
		uniform mat4 m_model;\n
		uniform mat4 m_playerModel;\n
		\n
		float pi = 3.1415926535897932384626433832795;
		\n
		// mat[col][row]
		mat4 rotXAxis(float angle) {\n
			mat4 outM;
			outM[0][0] = 1.0; outM[1][0] = 0.0; 		outM[2][0] = 0.0; 		  	outM[3][0] = 0.0;\n
			outM[0][1] = 0.0; outM[1][1] = cos(angle);	outM[2][1] = -sin(angle); 	outM[3][1] = 0.0;\n
			outM[0][2] = 0.0; outM[1][2] = sin(angle); 	outM[2][2] = cos(angle);	outM[3][2] = 0.0;\n
			outM[0][3] = 0.0; outM[1][3] = 0.0; 		outM[2][3] = 0.0;			outM[3][3] = 1.0;\n
			return outM;\n
		}\n
		\n
		void main()\n
		{\n
			vec4 particlePos = pos[gl_InstanceID];\n
			vertex_out.lifeTime = particlePos.a;\n

			// get phase
			float phaseThres = (1.0 - blackThres) * 0.25 - noteWidthPhaseOffs;\n
			float phase = 1.0 - min(max((actTime - particlePos.z) / (noteEmitDelay * 2.0), 0.0), 1.0);
//			float phase = (particlePos.a / noteEmitDelay);

			vertex_out.color = vec3( abs(phase - 0.5) < ((1.0 - blackThres) * 0.5 - noteWidthPhaseOffs) ? 1.0 : 0.0 );

			// + 0.75, means start from the buttom of the cylinder (seen from camera)
			phase += 0.75 + phaseThres;
			phase *= pi * 2.0;

			// calculate rotation around x-axis dependent of lifetime
			mat4 xRot = rotXAxis(phase); // angle from 0 - 2pi
	    	mat3 normalRot = mat3(transpose(inverse(xRot)));

	    	// apply scaling
			vec4 modTrack = m_model * position;\n	// m_model

			// rotate around the x-axis in relation to lifetime
			modTrack = xRot * modTrack;\n

			// x offset to the corresponding track cylinder
			modTrack.x += cylScale.x * (particlePos.x - nrLinesPerInst * 0.5)
						+ cylInterSpace * (particlePos.x - (nrLinesPerInst - 1.0) * 0.5);\n

			// center to the track cylinder
			modTrack.y += cylOffs.y;\n
			modTrack.z += cylOffs.z;\n

			// offset by y and z corresponding to the phase and the deformation of the track cylinder
			vec4 circleOffset = vec4(0.0, cos(phase) * cylScale.y, sin(phase) * cylScale.z, 0.0);

			modTrack += circleOffset;

			vertex_out.modPos = m_playerModel * modTrack; \n

			// transform the normal, without perspective, and normalize it
			vertex_out.normal = normalRot * normal;\n

			gl_Position = m_pv * vertex_out.modPos;\n
		});

	vert = "// SNGam_Musica directional light note, vert\n" + shCol->getShaderHeader() + vert;


	std::string frag = STRINGIFY(
		uniform vec4 diffuse;\n
		uniform vec3 lightDirection;\n 	// direction toward the light
		uniform vec3 halfVector;\n		// surface orientation for shiniest spots
		uniform float shininess;\n		// exponent for sharping highlights
		uniform float strength;\n		// extra factor to adjust shininess
		uniform float blackThres;\n		// extra factor to adjust shininess
		uniform float noteBrightness;\n		// extra factor to adjust shininess
		\n
		in block {\n
			float lifeTime;\n
			vec3 normal;\n // interpolate the normalized surface normal
			vec4 modPos;\n // interpolate the normalized surface normal
			vec3 color;\n // interpolate the normalized surface normal
		} vertex_in;\n
		\n
		float pi = 3.1415926535897932384626433832795;
		\n
		out vec4 gl_FragColor;\n
		\n
		void main() {\n
			float lightColor = 1.0;
			float ambientLight = 0.05;
			\n
			// compute cosine of the directions, using dot products,
			// to see how much light would be reflected
			float diffuseAmt = max(0.0, dot(vertex_in.normal, lightDirection));\n
			float specular = max(0.0, dot(vertex_in.normal, halfVector));\n
			\n
			// surfaces facing away from the light (negative dot products)
			// won’t be lit by the directional light
			if (diffuseAmt == 0.0)\n
				specular = 0.0;\n
			else
				specular = pow(specular, shininess);\n	// sharpen the highlight
			\n
			vec3 scatteredLight = vec3(ambientLight) + vec3(diffuse) * diffuseAmt;\n
			vec3 reflectedLight = vec3(lightColor) * specular * strength;\n
			\n
			// don’t modulate the underlying color with reflected light,
			// only with scattered light
			vec3 rgb = min(vertex_in.color.rgb * scatteredLight
								+ reflectedLight
								+ vec3(ambientLight),
							vec3(1.0));\n

			rgb = vertex_in.color;

			gl_FragColor = vec4(rgb * noteBrightness, 1.0);\n
		});

	frag = "// SNGam_Musica directional light note frag\n" + shCol->getShaderHeader() +frag;

	noteShdr = shCol->addCheckShaderText("SNGam_Musica_noteCube", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNGam_Musica::drawNotesPartSys (camPar* cp, double time)
{
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	noteShdr->begin();
	noteShdr->setUniformMatrix4fv("m_pv", &m_pv[0][0]);

	noteShdr->setUniform4f("diffuse", 1.f, 1.f, 1.f, 1.f);
	noteShdr->setUniform3fv("lightDirection", &lightDir[0]); 	// direction toward the light
	noteShdr->setUniform3fv("halfVector", &lightDir[0]);		// surface orientation for shiniest spots
	noteShdr->setUniform1f("shininess", 100.f);		// exponent for sharping highlights
	noteShdr->setUniform1f("strength", 0.1f);		// extra factor to adjust shininess
	noteShdr->setUniform1f("noteBrightness", noteBrightness);		// extra factor to adjust shininess

	noteShdr->setUniform1f("blackThres", trackBlackThres);		// extra factor to adjust shininess

	noteShdr->setUniform1f("cylInterSpace", trackCylInterSpace);
	noteShdr->setUniform3f("cylOffs", 0.f, trackCylYOffs, trackCylZOffs);
	noteShdr->setUniform3f("cylScale", trackCylWidth, trackCylDepth, trackCylHeight);

	noteShdr->setUniform1f("actTime", float(time));
	noteShdr->setUniform1f("noteEmitDelay", noteEmitDelay);
	noteShdr->setUniform1f("nrLinesPerInst", nrLinesPerInst);

	// umfang 2 * pi * r (radius aproximiert)
	float noteWidthPhaseOffs = (2.f * trackCylWidth * noteRadius)
			/ (M_PI * (trackCylDepth + trackCylHeight));
	noteShdr->setUniform1f("noteWidthPhaseOffs", noteWidthPhaseOffs);

	for(unsigned int i=0; i<nrInstruments; i++)
    {
		glm::mat4 m_model = glm::scale(glm::vec3(trackCylWidth * noteRadius,
								  	  	  	  	 noteHeight,
												 trackCylWidth * noteRadius));

		noteShdr->setUniformMatrix4fv("m_playerModel", &playerModelMat[i][0][0]);
		noteShdr->setUniformMatrix4fv("m_model", &m_model[0][0]);
		noteShdr->setUniform1f("nrNotesPerTrack", (float)notesPartSys[i]->getSize());

    	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, notesPartSys[i]->getBuffer("pos")->getBuffer() );
    	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, notesPartSys[i]->getBuffer("aux0")->getBuffer() );

    	trackCylinder->drawElementsInst(GL_TRIANGLES, notesPartSys[i]->getSize());

    	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, 0 );
    	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, 0 );
    }

	noteShdr->end();
}

//----------------------------------------------------

std::string SNGam_Musica::getUpdtShader()
{
	// version statement kommt von aussen
	std::string src = shCol->getShaderHeader();

	src += "layout(local_size_x="+std::to_string(linePart[0]->getWorkGroupSize())+", local_size_y=1, local_size_z=1) in;\n";

	src += STRINGIFY(
			layout( std140, binding=0 ) buffer Pos  { vec4 pos[]; };\n
			layout( std140, binding=1 ) buffer Vel  { vec4 vel[]; };\n
			layout( std140, binding=2 ) buffer Aux0 { vec4 aux0[]; };\n
			\n
			uniform float dt;\n
			uniform float time;\n
			uniform float windAmt;\n
			uniform float forceCenterAmt;\n
			uniform float centerYOffs;\n
			uniform float rotForceAmt;\n
			uniform float rotAngleSpeed;\n
			uniform uint numParticles;\n
			\n
			uniform sampler3D noiseTex3D;\n

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

				//---------------------------------------

				// distance to center in x,z plane
				vec3 center = vec3(0.0, centerYOffs, 0.0); // the particle system gets transformed by playerMat, to our concern here is only the y offs

				// vector from point to the center (of the later transformed player Coord System)
				vec3 vecToCenter = center - vec3(p.xy, 0.0);
				vec3 radius = normalize(-vecToCenter);
				float velLength = length(v);

				// calculate distance in xy plane to center
				// the closer to the center to more attraction
				float forceToCenter = min(max(1.0 - length(vecToCenter), 0.0), 1.0);

				// normalize the vector from center to point
				vecToCenter = normalize(vecToCenter);

				//---------------------------------------

				// spiral rotation, the closer to the center the more rotation clockwise around the center
				float angle = atan(radius.y, radius.x);
				angle -= rotAngleSpeed;

				// rotate velocity vector in xy plane
				vec3 radiusOffs = vec3( cos(angle), sin(angle), 0.0);

				v = mix(v, vec3(radiusOffs.xy - radius.xy, 0.0) * velLength, min(max(forceToCenter * rotForceAmt, 0.0), 1.0));

				// attraction to center, assume a velocity vector of same length pointing to the center
				// blend between the actual vector and the center vector
				v = mix(v, vecToCenter * velLength, min(max(forceToCenter * forceCenterAmt, 0.0), 1.0));


				// wind
				v += randVal.xyz * dt * windAmt;	// wind

				// integrate
				p += v;\n

				// dont allow z values higher than 0
				p.z = min(0.0, p.z);

				// write new values
				pos[i] = vec4(p, max(pos[i].a - dt, 0.0));\n // lower lifetime
				vel[i] = vec4(v, 0.0);\n
				aux0[i] = aux0[i];\n
			});
	return src;
}

//----------------------------------------------------

void SNGam_Musica::initPartDrawShdr()
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
	layout( std140, binding=2 ) buffer Vel { vec4 vel[]; };\n
	layout( std140, binding=3 ) buffer Aux0 { vec4 aux0[]; };\n
	\n
	out block {\n
		vec4 color;\n
		vec2 texCoord;\n
		float lifeTime;\n
	} Out;\n
	\n
	float pi = 3.1415926535897932384626433832795;
	\n
	uniform vec4 partCol;\n
	\n
	void main() {
		int particleID = gl_VertexID >> 2;\n // 4 vertices per particle
		vec4 particlePos = pos[particleID];\n
		\n
		Out.color = vec4(partCol.rgb, min(particlePos.a, 1.0) * partCol.a);\n
		\n
		//map vertex ID to quad vertex
		Out.texCoord = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);\n
		\n
		// make a quad by extruding a line along the velocity vector
		vec3 rQBaseVec = vec3(gl_VertexID%4 < 2 ? -aux0[particleID].x : aux0[particleID].x, 0.0, 0.0);\n
		\n
		vec3 rotQuadPos = rQBaseVec + (
				(gl_VertexID%4 == 0 || gl_VertexID%4 == 3) ? vec3(0.0) : vel[particleID].xyz * aux0[particleID].y
			);\n
		\n
	//	vec4 particlePosEye = ModelView * vec4(particlePos.xyz, 1.0);
	//	vec4 vertexPosEye = particlePosEye + vec4((quadPos*2.0-1.0) * aux0[particleID].x * 0.01, 0, 0);
		\n
		Out.lifeTime = min(sqrt(particlePos.a), 1.0);\n

		gl_Position = ModelViewProjection * vec4(particlePos.xyz + rotQuadPos, 1.0);
	});

	vert = shCol->getShaderHeader() +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	in block {
		vec4 color;
		vec2 texCoord;
		float lifeTime;\n
	} In;

	layout(location=0) out vec4 fragColor;
	uniform float alpha;

	void main() {
		// Quick fall-off computation
		float r = (In.texCoord.y * 2.0 - 1.0) *3.0;
//		float r = length( In.texCoord * 2.0 - 1.0) *3.0;
		float i = exp(-r*r);
		if (i < 0.01 || In.color.a < 0.01 ) discard;

		fragColor = vec4(In.color.rgb, i * In.color.a * alpha * In.lifeTime);
	});

	frag = "// SNGam_Musica Part frag\n"+shCol->getShaderHeader()+frag;

	partDrawShdr = shCol->addCheckShaderText("SNGam_Danza_part_draw", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNGam_Musica::drawParticles (camPar* cp, double time)
{
	partFbo->bind();
	partFbo->clearAlpha(partFdbk, 0.f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//std::cout << glm::to_string(baseCol[0]) << std::endl;

    for(unsigned int trackNr=0; trackNr<nrInstruments; trackNr++)
    {
    	//mShaderParams.ModelView = cp->view_matrix_mat4 * playerModelMat[trackNr];
		mShaderParams.ModelViewProjection = cp->projection_matrix_mat4 * cp->view_matrix_mat4
				 * playerModelMat[trackNr] * glm::translate(glm::vec3(0.f, trackCylYOffs, trackCylZOffs));
		mShaderParams.ProjectionMatrix = cp->projection_matrix_mat4;

		// update struct representing UBO
		mShaderParams.numParticles = linePart[trackNr]->getSize();
		mShaderParams.spriteSize = spriteSize;
		mShaderParams.initAmt = 0.1f;
		mShaderParams.attractor.w = 0.0f;

		glActiveTexture(GL_TEXTURE0);

		// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

		// draw particles
		partDrawShdr->begin();
		partDrawShdr->setUniform1f("alpha", partAlpha);

		unsigned int colInd = (-trackNr + nrInstruments) % nrInstruments;
		partDrawShdr->setUniform4f("partCol", baseCol[colInd][0] * partBright, baseCol[colInd][1]* partBright,
				baseCol[colInd][2] * partBright, 1.f);

		// reference the compute shader buffer, which we will use for the particle
		// wenn kein vao gebunden ist, funktioniert glDrawElements nicht...
		testVAO->bind();

		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  linePart[trackNr]->getBuffer("pos")->getBuffer() );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  linePart[trackNr]->getBuffer("vel")->getBuffer() );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  linePart[trackNr]->getBuffer("aux0")->getBuffer() );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, linePart[trackNr]->getIndexBuffer()->getBuffer() );

		glDrawElements(GL_TRIANGLES, linePart[trackNr]->getSize()*6, GL_UNSIGNED_INT, 0);

		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  0 );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
		glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

		partDrawShdr->end();
		testVAO->unbind();
    }

	partFbo->unbind();

	// ------- proc blurs ----------------

	glow->setAlpha(partGlowAlpha);
	glow->setBright(partGlowBright);
	glow->setOffsScale(partGlowOffsScale);
	glow->proc(partFbo->getColorImg());
	for (unsigned short i=0; i<(unsigned int)partGlowNrBlurs; i++)
		glow->proc(glow->getResult(), i);

	//--------------------------------

	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // additive blend

	fadeShdr->begin();
	fadeShdr->setIdentMatrix4fv("m_pvm");
	fadeShdr->setUniform1i("tex", 0);
	fadeShdr->setUniform1i("glow", 1);
	fadeShdr->setUniform1f("alphaCut", 0.2f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, partFbo->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, glow->getResult());

	rawQuad->draw();
}

//----------------------------------------------------

std::string SNGam_Musica::getSuccessPartUpdtShader()
{
	// version statement kommt von aussen
	std::string src = shCol->getShaderHeader();

	src += "layout(local_size_x="+std::to_string(linePart[0]->getWorkGroupSize())+", local_size_y=1, local_size_z=1) in;\n";

	src += STRINGIFY(
			layout( std140, binding=0 ) buffer Pos  { vec4 pos[]; };\n
			layout( std140, binding=1 ) buffer Vel  { vec4 vel[]; };\n
			layout( std140, binding=2 ) buffer Aux0 { vec4 aux0[]; };\n
			\n
			uniform float dt;\n
			uniform float time;\n
			uniform float windAmt;\n
			uniform float forceCenterAmt;\n
			uniform float centerYOffs;\n
			uniform float rotForceAmt;\n
			uniform float rotAngleSpeed;\n
			uniform uint numParticles;\n
			\n
			uniform sampler3D noiseTex3D;\n

			// compute shader to update particles
			void main() {\n
				uint i = gl_GlobalInvocationID.x;\n
				if (i >= numParticles) return;\n

				// read particle position and velocity from buffers
				vec3 p = pos[i].xyz;\n
				vec3 v = vel[i].xyz;\n

				// randomValue -1 to 1
				//vec3 texCoord = p * 0.5 + 0.5;\n
				//vec4 randVal = texture(noiseTex3D, texCoord * 0.1 + time * 0.005);\n

				// wind
				//v += randVal.xyz * dt * windAmt;	// wind

				// integrate
				p += v;\n

				// dont allow z values higher than 0
				p.z = min(0.0, p.z);

				// write new values
				pos[i] = vec4(p, max(pos[i].a - dt, 0.0));\n // lower lifetime
				vel[i] = vec4(v, 0.0);\n
				aux0[i] = aux0[i];\n
			});
	return src;
}

//----------------------------------------------------

void SNGam_Musica::initSuccessPartDrawShdr()
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
	layout( std140, binding=2 ) buffer Vel { vec4 vel[]; };\n
	layout( std140, binding=3 ) buffer Aux0 { vec4 aux0[]; };\n //rgb = col, w = size
	\n
	out block {\n
		vec4 color;\n
		vec2 texCoord;\n
	} Out;\n
	\n
	void main() {
		// expand points to quads without using GS
		int particleID = gl_VertexID >> 2; // 4 vertices per particle
		vec4 particlePos = pos[particleID];

		Out.color = vec4(aux0[particleID].rgb, min(particlePos.a, 1.0));

		//map vertex ID to quad vertex
		vec2 quadPos = vec2( ((gl_VertexID - 1) & 2) >> 1, (gl_VertexID & 2) >> 1);
		quadPos = quadPos * 2.0 - 1.0;
		Out.texCoord = quadPos;

		vec4 vertexPosEye = vec4(particlePos.xyz, 1.0) + vec4(quadPos * aux0[particleID].w, 0.0, 0.0);
		gl_Position = ModelViewProjection * vertexPosEye;
	});

	vert = shCol->getShaderHeader() +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	in block {
		vec4 color;
		vec2 texCoord;
	} In;
	\n
	layout(location=0) out vec4 fragColor;
	uniform float alpha;
	\n
	void main() {
		float r = sqrt(min(length(In.texCoord), 1.0)) * 2.0;
		float i = exp(-r*r);
		if (i < 0.01 || In.color.a * alpha < 0.01 ) discard;
		fragColor = vec4(In.color.rgb, i * In.color.a * alpha);
	});

	frag = "// SNGam_Musica Succes Part frag\n"+shCol->getShaderHeader()+frag;

	partSuccDrawShdr = shCol->addCheckShaderText("SNGam_Danza_part_success_draw", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNGam_Musica::drawSuccessParticles (camPar* cp, double time)
{
	spFbo->bind();
	spFbo->clearAlpha(spFdbk, 0.f);

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	mShaderParams.ModelViewProjection = cp->projection_matrix_mat4 * cp->view_matrix_mat4;

	// update struct representing UBO
	mShaderParams.numParticles = succesPart->getSize();

	glActiveTexture(GL_TEXTURE0);

	// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

	// draw particles
	partSuccDrawShdr->begin();
	partSuccDrawShdr->setUniform1f("alpha", spAlpha);

	// reference the compute shader buffer, which we will use for the particle
	// wenn kein vao gebunden ist, funktioniert glDrawElements nicht...
	testVAO->bind();

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, succesPart->getBuffer("pos")->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, succesPart->getBuffer("vel")->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3, succesPart->getBuffer("aux0")->getBuffer() );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, succesPart->getIndexBuffer()->getBuffer() );

	glDrawElements(GL_TRIANGLES, succesPart->getSize()*6, GL_UNSIGNED_INT, 0);

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	partSuccDrawShdr->end();
	testVAO->unbind();

	spFbo->unbind();

	//-----------------------------------------------------------------

	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	alphaCutShdr->begin();
	alphaCutShdr->setIdentMatrix4fv("m_pvm");
	alphaCutShdr->setUniform1i("tex", 0);
	alphaCutShdr->setUniform1f("alphaCut", spAlphaCut);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, spFbo->getColorImg());

	rawQuad->draw();
}

//----------------------------------------------------

void SNGam_Musica::initAudioWaveShdr()
{
	std::string shdr_Header = shCol->getShaderHeader();

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;\n
	layout( location = 2 ) in vec2 texCoord;\n
	\n
	out block {
		vec4 color;\n
	} vertex_out;\n
	\n
	uniform sampler1D tex;\n
	uniform vec4 lineCol;\n
	uniform float audioAmp;\n
	uniform float lineThick;\n
	uniform float time;\n
	uniform float twirlAmt;\n
	uniform float nrInstance;\n
	uniform float instTwirlOffs;\n
	\n
	// mat[col][row]
	mat4 rotXAxis(float angle) {\n
		mat4 outM;
		outM[0][0] = 1.0; outM[1][0] = 0.0; 		outM[2][0] = 0.0; 		  	outM[3][0] = 0.0;\n
		outM[0][1] = 0.0; outM[1][1] = cos(angle);	outM[2][1] = -sin(angle); 	outM[3][1] = 0.0;\n
		outM[0][2] = 0.0; outM[1][2] = sin(angle); 	outM[2][2] = cos(angle);	outM[3][2] = 0.0;\n
		outM[0][3] = 0.0; outM[1][3] = 0.0; 		outM[2][3] = 0.0;			outM[3][3] = 1.0;\n
		return outM;\n
	}\n
	\n
	void main() {
		float sampAmp = texture(tex, -time * 0.03 + position.x * 0.5 - 0.5).r * audioAmp;
		vertex_out.color = lineCol;
		float yCoord = sampAmp;
		yCoord += ((gl_VertexID % 2) == 0 ? -lineThick : lineThick);
		float fInst = float(gl_InstanceID) / nrInstance * sin(time * 3.0);
		vec4 pos = rotXAxis(position.x * twirlAmt + time + fInst * instTwirlOffs) * vec4(position.x, yCoord, 0.0, 1.0);
		gl_Position = pos;
	});

	vert = shdr_Header +vert;

	//----------------------------------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;\n
	\n
	in block {
		vec4 color;\n
	} vertex_in;\n
	\n
	uniform float alpha;\n
	\n
	void main() {\n
		fragColor = vertex_in.color;
		fragColor.a *= alpha;\n
	});

	frag = "// SNGam_Musica audioWaveShader frag\n"+shdr_Header+frag;

	audioWaveShader = shCol->addCheckShaderText("SNGam_Musica_audiowave", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Musica::drawAudioWave(double time)
{
	int nrInstance = 15;

	alFbo->bind();
	alFbo->clearAlpha(alFdbk, 0.f);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	audioWaveShader->begin();
	audioWaveShader->setIdentMatrix4fv("m_pvm");
	audioWaveShader->setUniform1i("tex", 0);
	audioWaveShader->setUniform1f("alpha", alAlpha);
	audioWaveShader->setUniform1f("audioAmp", alAmp);
	audioWaveShader->setUniform1f("lineThick", alLineThick);
	audioWaveShader->setUniform1f("twirlAmt", alTwirlAmt);
	audioWaveShader->setUniform1f("instTwirlOffs", alInstTwirlOffs);
	audioWaveShader->setUniform1f("time", time);
	audioWaveShader->setUniform1f("nrInstance", (float)nrInstance);

	glActiveTexture(GL_TEXTURE0);

	for(unsigned int i=0; i<nrInstruments; i++)
	{
		unsigned int colInd = (-i + nrInstruments) % nrInstruments;
		audioWaveShader->setUniform4f("lineCol", baseCol[colInd][0], baseCol[colInd][1], baseCol[colInd][2], 1.f);

		glBindTexture(GL_TEXTURE_1D, audioTex->getTex(i));

		lineVAO->drawInstanced(GL_TRIANGLE_STRIP, nrInstance);
	}

	alFbo->unbind();

	//-----------------------------------------------------------------

	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	alphaCutShdr->begin();
	alphaCutShdr->setIdentMatrix4fv("m_pvm");
	alphaCutShdr->setUniform1i("tex", 0);
	alphaCutShdr->setUniform1f("alphaCut", alAlphaCut);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, alFbo->getColorImg());

	rawQuad->draw();
}

//----------------------------------------------------

void SNGam_Musica::drawIcons()
{
	stdTex->begin();

	for (unsigned int instNr=0; instNr<nrInstruments; instNr++)
	{
		glm::mat4 icMat = playerModelMat[instNr]
						* glm::translate(glm::vec3(0.f, instIconPos, 0.f))
						* glm::rotate(float(M_PI), glm::vec3(0.f, 0.f, 1.f))
						* glm::scale(glm::vec3(instIconSize, instIconSize, 1.f));

		stdTex->setUniformMatrix4fv("m_pvm", &icMat[0][0]);
		stdTex->setUniform1i("tex", 0);

		instIcons[ iconSet[songPtr][instNr] ].bind(0);

		rawQuad->draw();
	}
}

//----------------------------------------------------

void SNGam_Musica::drawDialogWin(camPar* cp)
{
	// typo
	for (unsigned int i=0; i<2; i++)
	{
		glm::mat4 transMat = glm::translate(glm::vec3(diagWinOffsX, diagWinOffsY, 0.f));
		glm::mat4 rotMat = glm::rotate((float)M_PI * (float)i, glm::vec3(0.f, 0.f, 1.f));
		glm::mat4 winMat = glm::scale(glm::vec3(diagWinSize.x * 0.5f, diagWinSize.y * 0.5f, 1.f));
		glm::mat4 tMat = rotMat * transMat * winMat;

		stdTex->begin();
		stdTex->setUniformMatrix4fv("m_pvm", &tMat[0][0]);
		stdTex->setUniform1i("tex", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dialogTex.getId());

		rawQuad->draw();

		transMat = glm::translate(glm::vec3(idleVideonOffsX, diagWinOffsY, 0.f));
		winMat = glm::scale(glm::vec3(idleVideoSize.x * 0.5f, idleVideoSize.y * -0.5f, 1.f));
		tMat = rotMat * transMat * winMat;

		stdTex->setUniformMatrix4fv("m_pvm", &tMat[0][0]);

		if(idleVideo)
			glBindTexture(GL_TEXTURE_2D, idleVideo->getTex());

		rawQuad->draw();
	}

	stdTex->begin();

	for (unsigned int instNr=0; instNr<nrInstruments; instNr++)
	{
		glm::mat4 icMat = playerModelMat[instNr]
						* glm::translate(glm::vec3(0.f, idleInstIconPos, 0.f))
						* glm::rotate(float(M_PI), glm::vec3(0.f, 0.f, 1.f))
						* glm::scale(glm::vec3(idleInstIconSize, idleInstIconSize, 1.f));

		stdTex->setUniformMatrix4fv("m_pvm", &icMat[0][0]);
		stdTex->setUniform1i("tex", 0);

		instIcons[ICON_MANO].bind(0);

		rawQuad->draw();
	}
}

//----------------------------------------------------

void SNGam_Musica::noteCb(int trackNr, float pitch, double duration)
{
	// delete old notes from the actNoteArray
	for (unsigned int j=0; j<nrLinesPerInst; j++)
	{
		if (actNotes[trackNr][j].size() > 0)
		{
			// push all notes that are older that the actual time + catch range
			// and push them into an array
			actNotes[trackNr][j].erase(
				std::remove_if(
					actNotes[trackNr][j].begin(), actNotes[trackNr][j].end(),
			    [&](const int& x) {
			        return x < (actTime - noteMaxDelayToCatch); // put your condition here
			    }), actNotes[trackNr][j].end());
		}
	}

	if ((unsigned int)trackNr < nrInstruments)
	{
		// emit particles
		float normPitch = (pitch - musicXMLReader.getTrackMinPitch(trackNr))
						/ std::max(musicXMLReader.getTrackMaxPitch(trackNr) - musicXMLReader.getTrackMinPitch(trackNr), 1.f);

		// quantize pitch position to nrLinesPerInst
		unsigned int normPitchQuant = (unsigned int)(normPitch * float(nrLinesPerInst -1));

		// lifetime is set to noteEmitDelay - inside the NoteDrawShader this is normalized
		// to lifetime / noteEmitDelay, when it becomes 0.5 the note will be right on top of the cylinder
		initPars[0] = glm::vec4((float)normPitchQuant, 0.f, actTime, noteEmitDelay); // pos: z: emitTime, last parameters = lifetime
		initPars[1] = glm::vec4(0.f, 0.f, 0.f, 0.f); 	// vel, par 2 = speed
		initPars[2] = baseCol[trackNr]; 				//

		notesPartSys[trackNr]->emit(1, initPars);

		// save the emitted note in the actNotes array
		actNotes[trackNr][normPitchQuant].push_back(actTime + noteEmitDelay);
	}
}

//----------------------------------------------------

void SNGam_Musica::songEndCb()
{
	actPlayState = SGNM_SELECT_DIALOG;
	stopSong();

	std::cout << "song end cv" << std::endl;

	requestPlayInitSong = true;

	std::cout << "song ended!!!" << std::endl;
}

//----------------------------------------------------

void SNGam_Musica::idleSongEndCb()
{
	if(actPlayState != SGNM_PLAY){
		std::cout << "idle song callback" << std::endl;
		requestPlayInitSong = true;
	}
}

//----------------------------------------------------

void SNGam_Musica::update(double time, double dt)
{
	actTime = time;

/*
	if (idleVideo && (time - lastUpdt) > switchTime)
	{
		std::cout << "switch " << std::endl;

		idleVideo->stop();
		idleVideo->open((char*)((*scd->dataPath)+"/movies/video_leche.mp4").c_str());
		lastUpdt = time;
	}
*/

	// debug loop
	if(time - debugInputTime > debugInputTimeInt){
		if(actPlayState == SGNM_SELECT_DIALOG)
			requestSwitchToPlay = true;
		debugInputTime = time;
	}


	if (playTrackRequest && (time - playTrackRequestTime) > (double)noteEmitDelay)
	{
		//std::cout << "sndPlayer->play(); " << std::endl;
		//sndPlayer->play();

        lo_send(scAddr, "/chat", "si", "play", songPtr+1);
		playTrackRequest = false;
	}

	for(unsigned int i=0; i<nrInstruments; i++)
		notesPartSys[i]->update(time, dt);

	updateLineParts(time, dt);
	updateSuccesParts(time, dt);
	procNoteCatching();

	// check if we have a new audio frame, if this is the case update the audio texture
	if (lastPaFrame != pa->getFrameNr())
	{
		// interpret instrument as channel in audioTex, we only take the first channel as input
		for(unsigned int i=0; i<nrInstruments; i++)
			audioTex->update(pa->getPllSepBand(0, i), i);

	    lastPaFrame = pa->getFrameNr();
	}

	if (_hasNewOscValues)
	{
	    for(unsigned int i=0; i<nrInstruments; i++)
	    	// calculate cylinder model matrices
	   		trackCylModelMat[i] = glm::translate(glm::vec3(0.f, trackCylYOffs, trackCylZOffs))
	   						  * glm::rotate(float(M_PI_2), glm::vec3(0.f, 1.f, 0.f))
	   						  * glm::rotate(float(M_PI_2), glm::vec3(1.f, 0.f, 0.f))
	   						  * glm::scale(glm::vec3(trackCylHeight, trackCylWidth, trackCylDepth));

		_hasNewOscValues = false;
		saveCalib();
	}

	if (actPlayState == SGNM_SELECT_DIALOG){
		if(idleVideo){
			idleVideo->loadFrameToTexture(time);
		}
	}

	if(requestPlayInitSong)
	{
        lo_send(scAddr, "/chat", "si", "play", 0);
//		idleVideo->open((char*)((*scd->dataPath)+"/movies/video_leche.mp4").c_str());
        //idleVideo->seek(0.0);
        idleVideo->start();
		requestPlayInitSong = false;
	}

	if(requestSwitchToPlay)
	{
		if (idleVideo)
			idleVideo->stop();

        lo_send(scAddr, "/chat", "s", "stopall");

		requestSwitchToPlay = false;
		actPlayState = SGNM_PLAY;
		songPtr = (unsigned int) getRandF(0.f, (float)nrSongs);
		songPtr = std::min(songPtr, nrSongs -1);

		playSong(songPtr);
	}

	if(requestEndSong && time > endSongTime)
	{
		std::cout << "song end" << std::endl;
		requestEndSong = false;
		songEndCb();
	}

	if (requestStartSongSc && time > requestStartSong)
	{
		lo_send(scAddr, "/chat", "si", "play", songPtr+1);
		requestStartSongSc = false;
	}

}

//----------------------------------------------------

void SNGam_Musica::updateLineParts(double time, double dt)
{
	// this buffer is also used by the compute shader inside GLSLParticleSystem
	// make framerate independent
	for(unsigned int i=0; i<nrInstruments; i++)
	{
		// update success particle
		// this buffer is also used by the compute shader inside GLSLParticleSystem
		// make framerate independent

		// Invoke the compute shader to integrate the particles
		linePart[i]->getUpdtShdr()->begin();
		linePart[i]->getUpdtShdr()->setUniform1f("dt", (float)dt);
		linePart[i]->getUpdtShdr()->setUniform1f("time", (float)time);
		linePart[i]->getUpdtShdr()->setUniform1f("windAmt", partWindAmt);
		linePart[i]->getUpdtShdr()->setUniform1f("forceCenterAmt", partForceCenterAmt);
		linePart[i]->getUpdtShdr()->setUniform1f("centerYOffs", -(playerMatYOffs + trackCylYOffs));
		linePart[i]->getUpdtShdr()->setUniform1f("rotAngleSpeed", partRotAngleSpeed);
		linePart[i]->getUpdtShdr()->setUniform1f("rotForceAmt", partRotForce);

		linePart[i]->getUpdtShdr()->setUniform1ui("numParticles", (unsigned int)linePart[i]->getSize());
		linePart[i]->getUpdtShdr()->setUniform1i("noiseTex3D", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, linePart[i]->getNoiseTex());

		linePart[i]->bindBuffers();

		glDispatchCompute((unsigned int)std::max((double)linePart[i]->getSize()
				/ (double)linePart[i]->getWorkGroupSize(), 1.0), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		linePart[i]->unbindBuffers();
		linePart[i]->getUpdtShdr()->end();

		//---------------------------------------------

		// emit particles
		// emit position is right below the centers of the track cylinders
		// for updating the particles we are working in the "standard" coordinate system (center at 0|0|0)
		// for drawing the particle positions will be multiplied by the playerMod Matrices
		initPars[0] = glm::vec4( getRandF(-0.1f, 0.1f), 0.f, 0.f, partLifeTime); // pos: last parameter = lifetime
		initPars[1] = glm::vec4(0.f, getRandF(-0.001f, -0.01f) * partSpeed, 0.f, 0.f);
		initPars[2] = glm::vec4(partSizeX, partSizeY, 0.f, 0.f); 	// keine werte kleiner als 0.001???? rasterung???

		linePart[i]->emit(8, initPars);
		linePart[i]->procEmit((float)time);

		lastEmit = time;
	}
}

//----------------------------------------------------

void SNGam_Musica::updateSuccesParts(double time, double dt)
{
	// update success particle
	// this buffer is also used by the compute shader inside GLSLParticleSystem
	// make framerate independent

	// Invoke the compute shader to integrate the particles
	succesPart->getUpdtShdr()->begin();
	succesPart->getUpdtShdr()->setUniform1f("dt", (float)dt);
	succesPart->getUpdtShdr()->setUniform1f("time", (float)time * 2.f);
	succesPart->getUpdtShdr()->setUniform1f("windAmt", 0.008f);
	succesPart->getUpdtShdr()->setUniform1ui("numParticles", (unsigned int)succesPart->getSize());
	succesPart->getUpdtShdr()->setUniform1i("noiseTex3D", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, succesPart->getNoiseTex());

	succesPart->bindBuffers();

	glDispatchCompute((unsigned int)std::max((double)succesPart->getSize()
			/ (double)succesPart->getWorkGroupSize(), 1.0), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	succesPart->unbindBuffers();
	succesPart->getUpdtShdr()->end();
}

//----------------------------------------------------

void SNGam_Musica::emitSuccessPart(unsigned int instNr, unsigned int lineNr)
{
	// emit particles
	// emit position is right below the centers of the track cylinders
	// for updating the particles we are working in the "standard" coordinate system (center at 0|0|0)
	// for drawing the particle positions will be multiplied by the playerMod Matrices

	float cylAngles[nrInstruments];
	cylAngles[0] = trackCylAngle0;
	cylAngles[1] = trackCylAngle1;
	cylAngles[2] = trackCylAngle2;
	cylAngles[3] = trackCylAngle3;


	// calculate emitMatrix
	float noteWidthPhaseOffs = (2.f * trackCylWidth * noteRadius)
			/ (M_PI * (trackCylDepth + trackCylHeight));

	// matrix for note Position:
	float phaseThres = (1.f - trackBlackThres) * 0.5f;
	float phase = 0.5f + phaseThres;
	phase *= float(M_PI) * 2.0;

	// x offset to the corresponding track cylinder
	float noteXOffs = trackCylWidth * (lineNr - nrLinesPerInst * 0.5f)
				+ trackCylInterSpace * (lineNr - (nrLinesPerInst - 1.0f) * 0.5f);

	glm::vec4 emitPos = glm::rotate(cylAngles[instNr], glm::vec3(0.f, 0.f, 1.f))
						* glm::translate(glm::vec3(0.f, playerMatYOffs +trackCylYOffs, trackCylZOffs))
						// rotation around the x-axis -> cylinder surface
						* glm::vec4(noteXOffs, std::sin(phase) * trackCylDepth, -std::cos(phase) * trackCylHeight, 1.f);

	initPars[0] = emitPos; // pos: last parameter = lifetime
	initPars[1] = glm::vec4(0.f, 0.f, 0.f, 0.f);

	unsigned int colInd = (nrInstruments - instNr + nrInstruments) % nrInstruments;
	initPars[2] = glm::vec4(baseCol[colInd].r, baseCol[colInd].g, baseCol[colInd].g, 0.0005f); 	// keine werte kleiner als 0.001???? rasterung???

	initRand[0] = glm::vec4(spEmitXRand, spEmitYRand, 0.f, 0.f);
	initRand[1] = glm::vec4(spRandSpeed, spRandSpeed, 0.f, 0.f);
	initRand[2] = glm::vec4(0.f, 0.f, 0.f, spSpriteSize);

	succesPart->emit(32, initPars, initRand);
	succesPart->procEmit((float)actTime);
}

//----------------------------------------------------

void SNGam_Musica::procNoteCatching()
{
	for (unsigned int i=0; i<nrInstruments; i++)

		for (unsigned int j=0; j<nrLinesPerInst; j++)

			// if button pressed check if we do have a note at the corresponding instrument/track
			// that just passed the trigger
			if (controlerSet[i][j].pressed)

				// check if we have a note that matches that is close enough to the button down time
				for (std::vector<double>::iterator it=actNotes[i][j].begin(); it!=actNotes[i][j].end(); ++it)
				{
					if (std::fabs((*it) - controlerSet[i][j].onTime) < noteMaxDelayToCatch)
						emitSuccessPart(i, j);	// emit success particles
				}
}

//----------------------------------------------------

void SNGam_Musica::udpCb(boost::array<char, 1024>* recv_buffer)
{
	int readOffset = 2;

	for (unsigned int j=0; j<3; j++)
	{
		boost::array<char, 1024>::iterator it = recv_buffer->begin() + readOffset +j;

		std::bitset<8> b(*it);
		//std::cout << " " <<  b.to_string() ;
	}
	//std::cout << std::endl;


	for (unsigned int i=0; i<nrInstruments; i++)
	{
		for (unsigned int j=0; j<nrLinesPerInst; j++)
		{
			boost::array<char, 1024>::iterator it = recv_buffer->begin() + readOffset + controlerSet[i][j].byte;
			bool pressed = (*it) & 1 << controlerSet[i][j].bit;

			if (!controlerSet[i][j].pressed && pressed)
			{
				if(actPlayState == SGNM_SELECT_DIALOG)
					requestSwitchToPlay = true;

				controlerSet[i][j].onTime = actTime;
			}

			controlerSet[i][j].pressed = pressed;
		}
	}
}

//----------------------------------------------------

void SNGam_Musica::requestPlayTrack(double time)
{
	playTrackRequest = true;
	playTrackRequestTime = time;
	playMidi = true;
	musicXMLReader.play();
}

//----------------------------------------------------

void SNGam_Musica::playSong(unsigned int _songNr)
{
	songPtr = _songNr;

	if (songPtr < nrSongs)
	{
		/*
		sndPlayer->open_file(songFilesAudio[songPtr].c_str());
		sndPlayer->setRouteToIn(true);
		sndPlayer->setLoop(false);
		sndPlayer->setVolume(1.f);
		sndPlayer->setStopCallback(std::bind(&SNGam_Musica::songEndCb, this), 0.0);
*/

        requestStartSongSc = true;
        requestStartSong = actTime + noteEmitDelay;

        requestEndSong = true;
        endSongTime = actTime + songDuration[songPtr] + noteEmitDelay;

		musicXMLReader.read( songFilesXml[songPtr].c_str() );
		if (musicXMLReader.getNrTracks() != nrInstruments){
			std::cerr << "SNGam_Musica::Error !!! song " << songPtr << " has wrong number of tracks!!!" << std::endl;
		}
		musicXMLReader.setNoteCallbackFunction(std::bind(&SNGam_Musica::noteCb, this,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	} else {
		std::cerr << "SNGam_Musica::playSong Error!!! Requested song which has no xml part!!!" << std::endl;
	}

	requestPlayTrack(actTime);
	actPlayState = SGNM_PLAY;
}

//----------------------------------------------------

void SNGam_Musica::stopSong()
{
	std::cout << "stop song" << std::endl;

	musicXMLReader.stop();
	musicXMLReader.close();
	playMidi = false;
	actPlayState = SGNM_SELECT_DIALOG;
}

//--------------------------------------------------------------------------------

void SNGam_Musica::loadCalib()
{
	cv::FileStorage fs(calibFileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		fs["alInstTwirlOffs"] >> alInstTwirlOffs;
		fs["alTwirlAmt"] >> alTwirlAmt;
		fs["alAmp"] >> alAmp;
		fs["alLineThick"] >> alLineThick;
		fs["alAlphaCut"] >> alAlphaCut;
		fs["alFdbk"] >> alFdbk;
		fs["alAlpha"] >> alAlpha;

		fs["noteRadius"] >> noteRadius;
		fs["noteHeight"] >> noteHeight;
		fs["noteBrightness"] >> noteBrightness;

		fs["glowAlpha"] >> glowAlpha;
		fs["glowBright"] >> glowBright;
		fs["glowOffsScale"] >> glowOffsScale;
		fs["glowNrBlurs"] >> glowNrBlurs;

		fs["spriteSize"] >> spriteSize;

		fs["partFdbk"] >> partFdbk;
		fs["partAlpha"] >> partAlpha;
		fs["partAlphaCut"] >> partAlphaCut;
		fs["partBright"] >> partBright;
		fs["partSizeX"] >> partSizeX;
		fs["partSizeY"] >> partSizeY;
		fs["partLifeTime"] >> partLifeTime;
		fs["partSpeed"] >> partSpeed;
		fs["partWindAmt"] >> partWindAmt;
		fs["partForceCenterAmt"] >> partForceCenterAmt;
		fs["partRotAngleSpeed"] >> partRotAngleSpeed;
		fs["partRotForce"] >> partRotForce;

		fs["partGlowAlpha"] >> partGlowAlpha;
		fs["partGlowBright"] >> partGlowBright;
		fs["partGlowOffsScale"] >> partGlowOffsScale;
		fs["partGlowNrBlurs"] >> partGlowNrBlurs;

		fs["spEmitXRand"] >> spEmitXRand;
		fs["spEmitYRand"] >> spEmitYRand;
		fs["spRandSpeed"] >> spRandSpeed;
		fs["spAlpha"] >> spAlpha;
		fs["spFdbk"] >> spFdbk;
		fs["spAlphaCut"] >> spAlphaCut;
		fs["spSpriteSize"] >> spSpriteSize;

		fs["trackCylDepth"] >> trackCylDepth;
		fs["trackCylWidth"] >> trackCylWidth;
		fs["trackCylHeight"] >> trackCylHeight;
		fs["trackCylYOffs"] >> trackCylYOffs;
		fs["trackCylZOffs"] >> trackCylZOffs;
		fs["trackBlackThres"] >> trackBlackThres;
		fs["trackCylInterSpace"] >> trackCylInterSpace;

		fs["trackCylAngle0"] >> trackCylAngle0;
		fs["trackCylAngle1"] >> trackCylAngle1;
		fs["trackCylAngle2"] >> trackCylAngle2;
		fs["trackCylAngle3"] >> trackCylAngle3;


		fs["instIconSize"] >> instIconSize;
		fs["instIconPos"] >> instIconPos;

		fs["lightDirZ"] >> lightDir.z;

		fs["diagWinSizeX"] >> diagWinSize.x;
		fs["diagWinSizeY"] >> diagWinSize.y;

		fs["diagWinOffsX"] >> diagWinOffsX;
		fs["diagWinOffsY"] >> diagWinOffsY;

		fs["idleInstIconSize"] >> idleInstIconSize;
		fs["idleInstIconPos"] >> idleInstIconPos;

		fs["idleVideoSizeX"] >> idleVideoSize.x;
		fs["idleVideoSizeY"] >> idleVideoSize.y;

		fs["idleVideonOffsX"] >> idleVideonOffsX;


	}

	_hasNewOscValues = true;
	update(0.0, 0.0);
}

//--------------------------------------------------------------------------------

void SNGam_Musica::saveCalib()
{
	cv::FileStorage fs(calibFileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		fs << "alTwirlAmt" << alTwirlAmt;
		fs << "alInstTwirlOffs" << alInstTwirlOffs;

		fs << "alAmp" << alAmp;
		fs << "alLineThick" << alLineThick;
		fs << "alAlphaCut" << alAlphaCut;
		fs << "alFdbk" << alFdbk;
		fs << "alAlpha" << alAlpha;

		fs << "noteRadius" << noteRadius;
		fs << "noteHeight" << noteHeight;
		fs << "noteBrightness" << noteBrightness;

		fs << "glowAlpha" << glowAlpha;
		fs << "glowBright" << glowBright;
		fs << "glowOffsScale" << glowOffsScale;
		fs << "glowNrBlurs" << glowNrBlurs;

		fs << "spriteSize" << spriteSize;

		fs << "partFdbk" << partFdbk;
		fs << "partAlpha" << partAlpha;
		fs << "partAlphaCut" << partAlphaCut;
		fs << "partBright" << partBright;
		fs << "partSizeX" << partSizeX;
		fs << "partSizeY" << partSizeY;
		fs << "partLifeTime" << partLifeTime;
		fs << "partSpeed" << partSpeed;
		fs << "partWindAmt" << partWindAmt;
		fs << "partForceCenterAmt" << partForceCenterAmt;
		fs << "partRotAngleSpeed" << partRotAngleSpeed;
		fs << "partRotForce" << partRotForce;

		fs << "partGlowAlpha" << partGlowAlpha;
		fs << "partGlowBright" << partGlowBright;
		fs << "partGlowOffsScale" << partGlowOffsScale;
		fs << "partGlowNrBlurs" << partGlowNrBlurs;

		fs << "spEmitXRand" << spEmitXRand;
		fs << "spEmitYRand" << spEmitYRand;
		fs << "spRandSpeed" << spRandSpeed;
		fs << "spAlpha" << spAlpha;
		fs << "spFdbk" << spFdbk;
		fs << "spAlphaCut" << spAlphaCut;
		fs << "spSpriteSize" << spSpriteSize;

		fs << "trackCylDepth" << trackCylDepth;
		fs << "trackCylWidth" << trackCylWidth;
		fs << "trackCylHeight" << trackCylHeight;
		fs << "trackCylYOffs" << trackCylYOffs;
		fs << "trackCylZOffs" << trackCylZOffs;
		fs << "trackBlackThres" << trackBlackThres;
		fs << "trackCylInterSpace" << trackCylInterSpace;

		fs << "trackCylAngle0" << trackCylAngle0;
		fs << "trackCylAngle1" << trackCylAngle1;
		fs << "trackCylAngle2" << trackCylAngle2;
		fs << "trackCylAngle3" << trackCylAngle3;

		fs << "instIconSize" << instIconSize;
		fs << "instIconPos" << instIconPos;

		fs << "diagWinSizeX" << diagWinSize.x;
		fs << "diagWinSizeY" << diagWinSize.y;
		fs << "diagWinOffsX" << diagWinOffsX;
		fs << "diagWinOffsY" << diagWinOffsY;

		fs << "idleInstIconSize" << idleInstIconSize;
		fs << "idleInstIconPos" << idleInstIconPos;


		fs << "idleVideoSizeX" << idleVideoSize.x;
		fs << "idleVideoSizeY" << idleVideoSize.y;

		fs << "idleVideonOffsX" << idleVideonOffsX;

		fs << "lightDirZ" << lightDir.z;
	}
}

//----------------------------------------------------

void SNGam_Musica::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_1 :
				controlerSet[3][0].onTime = glfwGetTime();
				controlerSet[3][0].pressed = true;
				break;
			case GLFW_KEY_2 :
				controlerSet[3][1].onTime = glfwGetTime();
				controlerSet[3][1].pressed = true;
				break;
			case GLFW_KEY_3 :
				controlerSet[3][2].onTime = glfwGetTime();
				controlerSet[3][2].pressed = true;
				break;
			case GLFW_KEY_4 :
				controlerSet[3][3].onTime = glfwGetTime();
				controlerSet[3][3].pressed = true;
				break;
			case GLFW_KEY_5 :
				controlerSet[3][4].onTime = glfwGetTime();
				controlerSet[3][4].pressed = true;
				break;

			case GLFW_KEY_UP : songPtr = std::max((int)songPtr -1, 0);
				printf("key up, songPtr: %d\n", songPtr);
				break;
			case GLFW_KEY_DOWN : songPtr = std::min((int)songPtr +1, (int)nrSongs -1);
				printf("key down, songPtr: %d\n", songPtr);
				break;
			case GLFW_KEY_ENTER :
				if (!playMidi)
				{
					printf("enter !, play song\n");
					playSong(songPtr);
				} else {
					printf("stop the song\n");
					stopSong();
				}
				break;
		}
	}

	if (action == GLFW_RELEASE)
	{
		switch (key)
		{
		case GLFW_KEY_1 :
			controlerSet[3][0].pressed = false;
			break;
		case GLFW_KEY_2 :
			controlerSet[3][1].pressed = false;
			break;
		case GLFW_KEY_3 :
			controlerSet[3][2].pressed = false;
			break;
		case GLFW_KEY_4 :
			controlerSet[3][3].pressed = false;
			break;
		case GLFW_KEY_5 :
			controlerSet[3][4].pressed = false;
			break;
		}
	}
}

//----------------------------------------------------

SNGam_Musica::~SNGam_Musica()
{

	/*
	if (sndPlayer->isPlaying())
		sndPlayer->close_file();
*/


	delete [] songDuration;
	delete [] baseCol;
	delete diagHead;

	delete partFbo;
	delete spFbo;

	delete lineVAO;
	delete testVAO;

	delete alFbo;
}


}
