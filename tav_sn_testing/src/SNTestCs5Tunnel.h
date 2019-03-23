//
//  SNTestCs5Tunnel.hpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#ifndef SNTestCs5Tunnel_hpp
#define SNTestCs5Tunnel_hpp

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/VAO.h>
#include <GLUtils/GWindowManager.h>
#include <GeoPrimitives/Quad.h>
#include <Communication/OSC/OSCData.h>
#include <Shaders/Shaders.h>
#include <GLUtils/TextureManager.h>


namespace tav
{

class SNTestCs5Tunnel : public SceneNode
{

public:
	SNTestCs5Tunnel(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestCs5Tunnel();

	void initShdr(camPar* cp);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void buildTunnelShape(camPar* cp);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	OSCData*            osc;
	Quad*               quad;
	TextureManager*     testPic;
	VAO*                tunnelShape;
	ShaderCollector*	shCol;

	VAO*                tunnelLeftWall;
	VAO*                tunnelBackWall;
	VAO*                tunnelRightWall;

	Shaders*            stdCol;
	Shaders*            stdTex;
	Shaders*            shdr;

	glm::mat4           modelMat;
	glm::mat4*          identMat;
	glm::vec3 			roomDim;

	camPar*             camP = 0;

	float               logoScale;

	bool                inited = false;
};
}

#endif /* SNTestCs5Tunnel_hpp */
