//
// SNTestAudio.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <Shaders/Shaders.h>
#include <Shaders/ShaderCollector.h>
#include <GeoPrimitives/Line.h>
#include <GeoPrimitives/Quad.h>

#ifdef HAVE_AUDIO
#include <PAudio.h>
#endif

namespace tav
{

class SNTestAudio : public SceneNode
{
public:
	typedef struct {
		glm::vec4   color;
		float       chanYOffs;
#ifdef HAVE_AUDIO
		float       (*getVal)(PAudio* _pa, int chanNr, float ind);
#endif
	} testAudioPar;

	SNTestAudio(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestAudio();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

#ifdef HAVE_AUDIO
	void updateWave(int parNr, int chanNr);

	static float    sampData(PAudio* _pa, int chanNr, float fInd);
	static float    fftData(PAudio* _pa, int chanNr, float fInd);
	static float    pllData(PAudio* _pa, int chanNr, float fInd);
	static float    pitch(PAudio* _pa, int chanNr, float fInd);
	static float    onset(PAudio* _pa, int chanNr, float fInd);
#endif

private:

#ifdef HAVE_AUDIO
	PAudio*                     pa;
	std::vector<testAudioPar*>  audioPar;
#endif

	Line***                     lines;
	Quad*                       quad;
	int                         lastBlock = -1;
	int                         nrPar;
	Shaders*                    colShader;
	ShaderCollector*				shCol;
};
}
