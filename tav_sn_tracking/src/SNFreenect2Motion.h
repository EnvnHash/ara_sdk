/*
 * SNFreenect2Motion.h
 *
 *  Created on: 14.01.2016
 *      Copyright by Sven Hahne
 */


#ifndef TESTING_SNFreenect2Motion_H_
#define TESTING_SNFreenect2Motion_H_


#pragma once

#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <opencv2/core.hpp>


#include <GLUtils/GLSL/GLSLOpticalFlowDepth.h>

#include <SceneNode.h>
#include <GeoPrimitives/Quad.h>
#include <Freenect2/Freenect2In.h>
#include "GLUtils/GLSL/FastBlurMem.h"
#include <Communication/OSC/OSCData.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/Shaders.h>
//#include <GLUtils/PingPongFbo.h>
#include <NiTE/NISkeleton.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/GLSL/GLSLParticleSystemFbo.h>



namespace tav
{
    class SNFreenect2Motion : public SceneNode
    {
    public:
    	enum mode { CALIB=0, DRAW=1, PROC_ONLY=2 };

    	typedef struct {
            glm::vec3*		scale;
            glm::vec3*		trans;
            float*			rotAngleX;
            float*			rotAngleY;
            glm::vec2		kinFov;
    		glm::vec3*		roomDim;
    		glm::vec2		cropX;
    		glm::vec2		cropY;
    		glm::vec2		cropZ;
            unsigned short*	xtionAssignMap;
    	} mapConfig;

        SNFreenect2Motion(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNFreenect2Motion();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void displayInputs();
        void displayDebug();
        void displayFlipRes();

        void update(double time, double dt);
        void procFlipAxis();

        void onKey(int key, int scancode, int action, int mods);
    	void initShaders();
    	void initFlipAxisShdr();
    	void uploadNisImg(int device);

    	GLuint getVelTex(int device = 0);
    	GLuint getEmitTex(int device = 0);
    	int	getEmitTexHeight();
    	int	getEmitTexWidth();
    	mapConfig* getMapConfig();

    	void saveSetting();
    	void loadSetting();

    private:
        Quad*   				quad;
        Quad*					rotQuad;
        Freenect2In** 			fnc;
        KinectInput* 			kin;
        FastBlurMem**			blur;
        GLSLOpticalFlowDepth**	optFlow;
        //PingPongFbo*			ppFbo;
        NISkeleton**     		nis;
        TextureManager*			emitTex;
        //KinectReproTools**		reproTools;
        GWindowManager*			winMan;


        FastBlurMem*			mapBlur;
        FBO*					mapBlurFbo;

        ShaderCollector*		shCol;
        Shaders*				stdTexShader;
        Shaders*				flipAxisShdr;

        VAO*					emitTrig;

        Shaders*				renderShader;
        Shaders* 				renderGrayShader;

        OSCData*				osc;

        glm::mat4*				prevMats;
        glm::vec3*				mapOffs;

        bool					initRoomDimen = false;
        bool					useV2;
        bool*					nisTexInited;

        int*					lastFrame;
        int* 					lastColorFrame;
        int*					nisFrameNr;


    	GLint* 					texNrAr;
    	GLint* 					maskTexNrAr;
    	GLint* 					velTexNrAr;
    	glm::mat4* 				rotMats;

        GLuint*					nisTexId;

        int						maxNrPointAmp;
        int						nrPixPerGrid;
        int 					nrFnc;
        int						camSelect=0;
        int 					cropSelector=0;

        unsigned int			nrDevices;
        unsigned int			destTexSize;
        unsigned int			nrPixPerGridSide;

        glm::ivec2				nrGrids;

        std::string*			serialMap;

        mode					actMode;

        glm::ivec2      		emitTrigCellSize;
        glm::ivec2      		emitTrigGridSize;
        int						emitTrigNrPartPerCell;

        std::string				calibFilePath;
        mapConfig				mapConf;
    };
}
#endif /* TESTING_SNFreenect2Motion_H_ */
