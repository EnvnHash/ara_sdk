//
//  SNTexMosaicShuffler.hpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef SNTexMosaicShuffler_hpp
#define SNTexMosaicShuffler_hpp

#pragma once

#include <iostream>

#include <glm/gtc/type_ptr.hpp>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include "math_utils.h"

namespace tav
{
class SNTexMosaicShuffler: public SceneNode
{

public:
	SNTexMosaicShuffler(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTexMosaicShuffler();

	void initShdr();
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods)
	{}

private:
	ShaderCollector* 	shCol;
	Shaders*			shufflShdr;
	Quad*				quad;
    TextureManager* 	tex0;

    int					gridSizeX;
    int					gridSizeY;
    int 				nrIds;
    int*				randIds;

    float				ySinScale=1.0;
    float				ySinFreq=1.0;
    float				ySinScale2=1.0;
    float				ySinFreq2=1.0;
    float				ySinScale3=1.0;
    float				ySinFreq3=1.0;
    float				ySinScale4=1.0;
    float				ySinFreq4=1.0;

    glm::vec2*			sizeMod;

	bool				inited;
};
}

#endif /* SNTexMosaicShuffler_hpp */
