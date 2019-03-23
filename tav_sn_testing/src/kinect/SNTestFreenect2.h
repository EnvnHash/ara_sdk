/*
* SNTestFreenect2.h
 *
 *  Created on: 12.01.2016
 *      Copyright by Sven Hahne
 */

#ifndef TESTING_SNTESTFREENECT2_H_
#define TESTING_SNTESTFREENECT2_H_


#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Freenect2/Freenect2In.h>

namespace tav
{

class SNTestFreenect2 : public SceneNode
{
public:
	SNTestFreenect2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestFreenect2();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};
	void initShaders();

private:
	Quad*   			quad;
	Freenect2In* 		fnc;

	Shaders*			renderShader;
	Shaders* 			renderGrayShader;
	ShaderCollector*	shCol;

};
}
#endif /* TESTING_SNTESTFREENECT2_H_ */
