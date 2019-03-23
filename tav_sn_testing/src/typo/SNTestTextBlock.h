//
// SNTestTextBlock.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __SNTestTextBlock__
#define __SNTestTextBlock__

#pragma once

#include <SceneNode.h>
#include <GLUtils/Typo/Nvidia/NVTextBlock.h>
#include <GLUtils/Typo/Nvidia/NVText.h>

namespace tav
{

class SNTestTextBlock : public SceneNode
{
public:
	SNTestTextBlock(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	virtual ~SNTestTextBlock();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

private:
	NVTextBlock*			textBlock;
	ShaderCollector*		shCol;

	bool					isInited=false;
};

}

#endif
