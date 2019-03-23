/*
 * SNTestObjImporter.h
 *
 *  Created on: 14.06.2016
 *      Copyright by Sven Hahne
 */

#ifndef TESTING_SNTESTOBJIMPORTER_H_
#define TESTING_SNTESTOBJIMPORTER_H_

#pragma once

#include <stdio.h>
//#include <half.hpp>

#include <SceneNode.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/ObjImporter.h>
#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Sphere.h>
#include <GLUtils/GWindowManager.h>

namespace tav
{

class SNTestObjImporter : public SceneNode
{
public:
	SNTestObjImporter(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	virtual ~SNTestObjImporter();

    float decode32(glm::vec4 rgba);

    void initShader();
    void initShaderNoFloat();
    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void onCursor(double xpos, double ypos);
    void onKey(int key, int scancode, int action, int mods);
    void update(double time, double dt);


    void cleanUp();
    
private:
    GWindowManager*                 winMan;
    OSCData* 						osc;
    ObjImporter* 					importer;
    Sphere*                         sphere;
    Shaders*						litShader;
    ShaderCollector*				shCol;

    TextureManager*                 litsphereTex;
    TextureManager*                 bumpMap;
    TextureManager*                 positionTex;
    TextureManager*                 cubeTex;
    
    unsigned int                    texWidth;
    unsigned int                    texHeight;
    
    bool                            floatTextures;
    double                          mouseX;
    glm::ivec2                      indexTexSize;
};

} /* namespace tav */

#endif /* TESTING_SNTESTOBJIMPORTER_H_ */
