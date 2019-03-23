//
//  SNGLSLFluidTest.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__SNCamchLiquid__
#define __Tav_App__SNCamchLiquid__

#pragma once

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <glm/ext.hpp>

#include <SceneNode.h>
#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Circle.h>
#include <GeoPrimitives/Quad.h>
#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/Spline3D.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLFluid.h>
#include <Shaders/ShaderCollector.h>


namespace tav
{
    class SNCamchLiquid : public SceneNode
    {
    public:
        SNCamchLiquid(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNCamchLiquid();

        void initShader();
        void initFluidHeightShader();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        glm::vec2 toFluidCoord(glm::vec2 normPos);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(double xpos, double ypos);

    private:
        bool            	inited = false;

        Quad*				quad;
        Quad*				rawQuad;
        QuadArray*      	hmGrid;

        Circle*         	circle;
        GLSLFluid*      	fluidSim;
        FastBlurMem*		blur;

        ShaderCollector*	shCol;
        Shaders*        	colShader;
        Shaders*        	texShdr;
        Shaders*	        normShader;
        Shaders*      	  	fluidHeightShdr;

        Spline3D*			colSpline;

        FBO*            	normFbo;
        FBO*            	fluidAndShape;

        TextureManager* 	litsphereTex;
        TextureManager* 	bumpMap;
        TextureManager* 	cubeTex;

        glm::vec4*			chanCols;

        glm::vec2       	fluidSize;
        glm::vec2       	forceScale;
        glm::vec2			toLeft;
        glm::vec2*      	forcePos;
        glm::vec2*      	oldPos;

        double          	mouseX = 0;
        double          	mouseY = 0;
        double				lastUpdt;

        float 				propo;
        float 				oscPhase=0;
        float 				perlinPhase = 0;

        float 				alpha=0.f;
        float 				diss=0.99f;
        float 				veldiss=0.99f;
        float 				xPos=0.f;
        float 				rad=1.f;
        float 				oscSpeed=2.f;
        float 				oscAmt=0.f;
        float 				posRandAmtX=0.f;
        float 				posRandAmtY=0.f;
        float 				posRandSpd=1.f;
        float 				velScale=2.f;
        float 				timeStep=0.125f;
        float 				bouy=0.1f;
        float 				bouyWeight=0.05f;
        float 				colorSpd=0.05f;
        float 				drawAlpha=1.f;
        float 				normHeightAdj=1.f;
        float 				heightOffs=-0.5f;
        float 				heightScale=1.f;
        float 				reflAmt=1.f;
        float 				blurAlpha=0.6f;
        float 				brightScale=1.2f;
        float 				velBlendPos=0.f;
        float 				fluidScaleX=1.f;
        float 				fluidScaleY=1.f;
        float 				nrEmit=1.f;
        float 				yScale=1.f;

        unsigned int    	flWidth;
        unsigned int    	flHeight;
        int            	 	scrScale;
        int					nrEmitters;
    };
}

#endif /* defined(__Tav_App__SNGLSLFluidTest__) */
