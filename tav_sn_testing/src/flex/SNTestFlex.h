//
// SNTestFlex.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/FBO.h>
#include <GeoPrimitives/Quad.h>
#include <Communication/OSC/OSCData.h>

#ifdef HAVE_FLEX
#include <flex/FlexUtil.h>
#endif

namespace tav
{

class SNTestFlex: public SceneNode
{
public:
	typedef struct
	{
		GLuint texture;
		GLuint framebuffer;
	} ShadowMap;

	SNTestFlex(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestFlex();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods) {}

#ifdef HAVE_FLEX
	void setFlexParam(sceneData* _scd);
	void StartFrame();
	void RenderScene(camPar* cp);
	void ShadowBegin(ShadowMap* map);
	void EndFrame(camPar* cp);
	ShadowMap* ShadowCreate();
	void UpdateCamera();
	void initShadowPassTroughShdr();
#endif
	
private:
	OSCData*			osc;
	Shaders*			clearShader;
	Shaders*			passTroughShdr=0;

	glm::vec4			clearCol;
	ShaderCollector*                shCol;
	FBO*				msaaFbo=0;
	ShadowMap*			g_shadowMap;

	GLuint				g_msaaFbo=0;
//	GLuint				g_msaaColorBuf=0;
//	GLuint				g_msaaDepthBuf=0;

	glm::vec3 			lightDir;
	glm::vec3 			lightPos;
	glm::vec3 			lightTarget;

	float 				radius = 0.1f;

	// deforming bunny
	float 				s = radius*0.5f;
	float 				m = 0.25f;

	double				g_realdt;
	const int 			kShadowResolution = 2048;

	int 				group = 1;
	int 				g_msaaSamples=4;

#ifdef HAVE_FLEX
	FlexUtil*			flex;
#endif
};
}
