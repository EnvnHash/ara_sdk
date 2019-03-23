//
// SNGam_Musica.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <functional>
#include <bitset>
#include <opencv2/core.hpp>

#include <Communication/UdpClient.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLParticleSystemCS.h>
#include <GLUtils/Typo/Nvidia/NVTextBlock.h>
#include <GLUtils/PingPongFbo.h>
//#include <GLUtils/Typo/FreetypeTex.h>
#include <GeoPrimitives/Cube.h>
#include <GeoPrimitives/Cylinder.h>
#include <MusicXml/MusicXml.h>
#include <Shaders/ShaderCollector.h>
#include <SceneNode.h>

#include <SoundFilePlayer.h>
#include <PAudio.h>
//#include <VideoTextureCv.h>
#include <FFMpegDecode.h>

#include <lo/lo.h>

namespace tav
{

struct PiContrButtSet
{
	int		byte;
	int		bit;
	bool	pressed;
	double  onTime;
	PiContrButtSet(int _byte=0, int _bit=0, bool _pressed=false)
		: byte(_byte), bit(_bit), pressed(_pressed), onTime(0.0)
	{}
};

class SNGam_Musica : public SceneNode
{
public:
	enum playState { SGNM_SELECT_DIALOG, SGNM_PLAY, SGNM_SELECT_REPEAT };
	enum iconNames { ICON_BAJO, ICON_BOMBO, ICON_BONGO, ICON_CAJA, ICON_CASCABEL, ICON_CLAVE,
		ICON_CONTRABAJO, ICON_FLAUTA, ICON_GUITARRA, ICON_HATZ, ICON_MANO, ICON_MARIMBA,
		ICON_TOM, NUM_INSTR_ICONS };

	SNGam_Musica(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
    ~SNGam_Musica();

    void initContrButtMap();
    void initSongData();
    void initGeometry();
    void initOscPar();
    void initDialogWindow(std::map<std::string, float>* _sceneArgs);
    void initParticleSystems();
    void initFbos();
    void initTextures();
    void initFadeShader();
    void initAlphaCutShader();

    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);

    void initTrackCylGlowShader();
    void drawTrackCylinderGlow(camPar* cp);

    void initTrackCylShader();
    void drawTrackCylinders(camPar* cp, int drawTop);

    void initNoteDrawShdr();
    void drawNotesPartSys(camPar* cp, double time);

    std::string getUpdtShader();
    void initPartDrawShdr();
    void drawParticles(camPar* cp, double time);

    std::string getSuccessPartUpdtShader();
    void initSuccessPartDrawShdr();
    void drawSuccessParticles(camPar* cp, double time);

    void initAudioWaveShdr();
    void drawAudioWave(double time);

    void drawIcons();
    void drawDialogWin(camPar* cp);

    void noteCb(int trackNr, float pitch, double duration);
    void songEndCb();
    void idleSongEndCb();

    void update(double time, double dt);
    void updateLineParts(double time, double dt);
    void updateSuccesParts(double time, double dt);

    void emitSuccessPart(unsigned int instNr, unsigned int lineNr);
    void procNoteCatching();

    void udpCb(boost::array<char, 1024>* recv_buffer);
    void onKey(int key, int scancode, int action, int mods);

    void requestPlayTrack(double time);
    void playSong(unsigned int _songNr);
    void stopSong();

	void loadCalib();
	void saveCalib();

private:

	AudioTexture*							audioTex;
    Cylinder*								trackCylinder;
    FastBlurMem*							glow;
    FBO*									alFbo;
    FBO*									glowFbo;
	FBO*									partFbo;
	FBO*									spFbo;
	FreeTypeFont** 							fonts;
	GWindowManager*							winMan;
    GLSLParticleSystemCS** 					linePart;
    GLSLParticleSystemCS** 					notesPartSys;
	GLSLParticleSystemCS* 					succesPart;
    MusicXml								musicXMLReader;
	NVTextBlock*							diagHead;
	NVTextBlock*							diagSongs;

    ShaderCollector*						shCol;
    PAudio*									pa;
	SoundFilePlayer*						sndPlayer;

	lo_address 								scAddr;

    //VideoTextureCv*							idleVideo=0;
	FFMpegDecode*							idleVideo=0;

    Quad*									hFlipQuad;
    Quad*									rawQuad;
	VAO*									testVAO;
	VAO*									lineVAO;

    GLuint 									mUBO;
    GLuint 									diagHeadTex;

	ShaderParams 							mShaderParams;

	Shaders*								audioWaveShader;
	Shaders*								alphaCutShdr;
	Shaders*								fadeShdr;
	Shaders*								noteShdr;
	Shaders*								partDrawShdr;
	Shaders*								partSuccDrawShdr;
	Shaders*								stdCol;
	Shaders*								stdParCol;
	Shaders*								stdTex;
	Shaders*								trackCylShdr;
	Shaders*								trackCylGlowShdr;

    TextureManager*							instIcons;
    TextureManager							dialogTex;
    TextureManager							idleLoopTex;

    glm::vec2								diagWinSize;
    glm::vec2								idleVideoSize;
    glm::vec3 								lightDir;
	glm::vec4* 								initPars;
	glm::vec4* 								initRand;
	glm::vec4*								baseCol;
	glm::mat4*								playerModelMat;
	glm::mat4*								trackCylModelMat;
	glm::mat4 								m_pv;
	glm::mat3*								cubeModelNormalMat;

	std::function<void(int, int, double)> 	noteCbFunc;

	bool									inited;
	bool									playTrackRequest;
	bool									playMidi;
	bool									typoInited;
	bool									wantInitPlay;
	bool									requestPlayInitSong;
	bool									requestSwitchToPlay;
	bool									requestEndSong;
	bool									requestStartSongSc=false;

	unsigned int							glowFboSize;
	unsigned int							nrSongs;
	unsigned int							nrInstruments;
	unsigned int							nrLinesPerInst;
	unsigned int							songPtr;
	unsigned int							nrLinePoints;

	int										lastPaFrame;

	double									lastPartUpdt;
	double									playTrackRequestTime;
    double									lastEmit = 0.0;
    double									actTime;
    double									partFboLastClear;
    double									endSongTime=0.0;
    double									requestStartSong=0.0;

    double									switchTime = 2.0;
    double									lastUpdt = 0.0;
    double*									songDuration;

	double									debugInputTime;
	double 									debugInputTimeInt;


    float 									alAmp;
    float 									alAlpha;
    float 									alLineThick;
    float 									alFdbk;
	float									alAlphaCut;
	float									alTwirlAmt;
	float									alInstTwirlOffs;

	float									diagWinHeadlHeight;
	float									diagWinHeadlBlockSpace;
	float									diagWinSizePad;
	float									diagWinSongLineHeight;

	float									instIconSize;
	float									instIconPos;

	float									glowAlpha;
	float									glowBright;
	float									glowOffsScale;
	float									glowNrBlurs;

	float									noteEmitDelay;
	float 									noteRadius;
	float 									noteHeight;
	float									noteBrightness;

	float									partAlpha;
	float									partAlphaCut;
	float									partBright;
	float 									partFdbk;
	float									partForceCenterAmt;
	float									partLifeTime;
	float									partSizeX;
	float									partSizeY;
	float									partSpeed;
	float									partWindAmt;
	float									partRotAngleSpeed;
	float									partRotForce;

	float									partGlowAlpha;
	float									partGlowBright;
	float									partGlowOffsScale;
	float									partGlowNrBlurs;

	float									spEmitXRand;
	float									spEmitYRand;
	float									spRandSpeed;
	float									spAlpha;
	float 									spFdbk;
	float									spAlphaCut;
	float									spSpriteSize;

	float									spriteSize;

	float									trackBlackThres;
	float									trackCylInterSpace;
	float 									trackCylWidth;
	float									trackCylDepth;
	float									trackCylHeight;
	float									trackCylYOffs;
	float									trackCylZOffs;
	float									trackCylAngle0;
	float									trackCylAngle1;
	float									trackCylAngle2;
	float									trackCylAngle3;

	float									playerMatYOffs;

	float									noteMaxDelayToCatch;

	float 									diagWinOffsX;
	float 									diagWinOffsY;

	float									idleInstIconSize;
	float									idleInstIconPos;

	float									idleVideonOffsX;

	std::string* 							songFilesAudio;
	std::string* 							songFilesXml;
	std::string* 							songNames;
	iconNames**								iconSet;

	playState								actPlayState;

	std::string 							calibFileName;

	UdpClient*								client;
	PiContrButtSet**						controlerSet;

	std::vector<double>**					actNotes;

};

}
