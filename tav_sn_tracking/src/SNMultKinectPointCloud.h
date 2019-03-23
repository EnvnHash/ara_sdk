/*
 * SNMultKinectPointCloud.h
 *
 *  Created on: 14.01.2016
 *      Copyright by Sven Hahne
 */

/*
 * SNMultKinectPointCloud.h
 *
 *  Created on: 12.01.2016
 *      Copyright by Sven Hahne
 */

#ifndef TESTING_SNMultKinectPointCloud_H_
#define TESTING_SNMultKinectPointCloud_H_


#pragma once

#include <iostream>
#include <string>

#include <SceneNode.h>
#include "SNFreenect2Motion.h"
#include <Communication/OSC/OSCData.h>
#include <KinectInput/KinectInput.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/Shaders.h>

namespace tav
{
    class SNMultKinectPointCloud : public SceneNode
    {
    public:
        SNMultKinectPointCloud(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNMultKinectPointCloud();
    
        void initTrigGrid(camPar* cp);
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void procFlipAxis();

        void onKey(int key, int scancode, int action, int mods) {};
    	void initFlipAxisShdr(camPar* cp);

    private:
        KinectInput* 			kin;

        ShaderCollector*		shCol;
        Shaders*				flipAxisShdr;

        VAO*					emitTrig;

        Shaders*				renderShader;
        Shaders* 				renderGrayShader;

        OSCData*				osc;

    	GLint* 					texNrAr;
    	glm::mat4* 				rotMats;

    	bool					inited = false;

        int						maxNrPointAmp;
        int						nrPixPerGrid;

        unsigned int			nrDevices;
        unsigned int			destTexSize;
        unsigned int			nrPixPerGridSide;

        glm::ivec2				nrGrids;

        std::string*			serialMap;
        unsigned short*			xtionAssignMap;

        glm::ivec2      		emitTrigCellSize;
        glm::ivec2      		emitTrigGridSize;
        int						emitTrigNrPartPerCell;

        SNFreenect2Motion::mapConfig* mapConf;
    };
}
#endif /* TESTING_SNMultKinectPointCloud_H_ */
