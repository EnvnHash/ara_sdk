//
// SNLogoBlobAnimation.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <glm/ext.hpp>

#include <GLUtils/Noise3DTexGen.h>
#include <GLUtils/TextureManager.h>
#include <headers/opencv_headers.h>
#include <Shaders/Shaders.h>
#include <math_utils.h>
#include <SceneNode.h>

namespace tav
{
    class SNLogoBlobAnimation : public SceneNode
    {
    public:
    	typedef struct{
    		glm::vec3 scale;
    		glm::vec3 center;
    		glm::vec3 transCenter;
    		float blendPos = 0.f;
    		glm::vec2 texTrans;
    		glm::vec2 texScale;
    		cv::Mat	shape;
    		GLuint texNr;
    	} logoFrag;

        SNLogoBlobAnimation(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNLogoBlobAnimation();

    	void initShader();
    	void initContours();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        cv::Mat						img;
        cv::Point2f					imgDim;
        std::vector<cv::Rect> 		boundRect;

        std::vector<std::vector<cv::Point> > contours;
        std::vector<std::vector<cv::Point> > contours_poly;
        std::vector<logoFrag>		logoFrags;

        Noise3DTexGen*				noiseTex;
        Quad*						quad;
        TextureManager* 			tex;
        Shaders*					stdTex;
        Shaders*					shdr;
        ShaderCollector*				shCol;

        bool                		inited = false;

        float 						imgProp;
        float*						randPhase;

        float 						blendPos =0.f;
        float 						posRandAmt =0.3f;
        float 						posRandSpd =0.2f;
        float 						alpha= 0.f;

    };
}
