//
//  SNRotatingObjMapping2.h
//  tav_scene
//
//  Created by Sven Hahne on 4/10/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __tav_scene__SNRotatingObjMapping2__
#define __tav_scene__SNRotatingObjMapping2__


#pragma once

#include <stdio.h>
#include <iostream>
#include <opencv2/core.hpp>

#include <headers/gl_header.h>
#include <Communication/OSC/OSCData.h>
#include <Communication/Arduino.h>
#include <GeoPrimitives/Line.h>
#include <GeoPrimitives/Quad.h>
#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/GWindowManager.h>
#include <KinectInput/KinectInput.h>
#include <Shaders/Shaders.h>
#include <Shaders/ShaderCollector.h>
#include <KinectInput/KinectReproTools.h>
#include <KinectInput/KinectReproCalibData.h>
#include <AnimVal.h>
#include <Median.h>
#include <SceneNode.h>
#include <UDPServer.h>


namespace tav
{
    class SNRotatingObjMapping2 : public SceneNode
    {
    public:
        SNRotatingObjMapping2(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNRotatingObjMapping2();
        
        void initDyn();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        std::string execCmd(char* cmd);
        void addLitSphereShader();
        void swapBlendTex(float* startInd, float* endInd);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        void myNanoSleep(uint32_t ns);

    private:
        ShaderCollector*   	shCol;
        Shaders*            shdr;
        Shaders*            colShader;
        Shaders*            texShader;
        Shaders*            texShaderDist;
        Shaders*            mirror;
        Shaders*            glass;
        
        std::vector<Shaders*> shdrSet;
        
        KinectInput*        kin;
        KinectReproTools*   kinRepro;
        GWindowManager*     winMan;
        SceneNode*       	busNode;
        UDPServer*          udp;
        Arduino*            arduino = 0;
        OSCData*            osc;
        
        PropoImage*         chessBoard;
        PropoImage*         chessBoard2;
        
        FastBlur*           fastBlur;
        TextureManager*     kinDepthTex;
        TextureManager*     switchTextures;
        TextureManager*     cubeTex;
        TextureManager*     litsphereTexture;
        TextureManager*     normalTexture;
        TextureManager*     lucesTex;

        KinectReproCalibData kpCalib;

        Line*               xLine;
        Line*               yLine;
        Quad*               whiteQuad;
        QuadArray*          chessQA;
        
        AnimVal<float>*     blendText;

        glm::mat4           transMatr;
        glm::mat4           beamerMatr;
        glm::mat4           modelMatr;
        glm::mat4           positionMatr;
        glm::mat4           scaleToRealDimMatr;
        glm::mat4           chessRotMatr;
        
        cv::Mat             view;
        std::vector<cv::Point2f> pointbuf;
        std::vector< Median<glm::vec3>* >rwCoord;
        Median<glm::vec3>*  cbNormal;

        std::vector<std::string> tex_paths;
        std::string         calibFilePath;
        
        float dir;

        float               near;
        float               far;
        float               objDist;
        float               modelRealHeight;
        float               ardMoveToPos=0.f;
        float               kinFovX;
        float               kinFovY;
        float               realBoardWidth;
        float               coordNorm;
        float               transZOffs;
        float               rotXOffs;
        float               rotPos = 0.f;
        
        float               blendTextStart;
        float               blendTextEnd;
        
        double              actTime=0.0;
        
        int                 kinDeviceNr;
        
        int                 motorActPos=0;
        int                 motorActLastPos=0;
        int                 motorActSpeed=1;

        int                 motorMaxSteps;
        
        int                 motorMaxAbsPos=0;
        int                 motorAbsPos=0;
        int                 motorAbsPosOffs=0;
        int                 motorAbsPos16bitCrossed=0;
        int                 motorLastAbsPos=0;
        
        int                 ardNrStepLoop=1;
        int                 actNrStepLoop=1;
        int                 actMoveDir=1; // 1 forward, 0 backward
        int                 lastMoveDir=1;

        int                 modMotorPos = 0;
        int                 lastModMotorPos = 0;
        int                 maxModMotorInc;
        
        int                 frameNr=-1;
        
        int                 actText = 0;
        int                 actMesh = 0;
        
        bool                ardInit;
        bool                useArduino;
        bool                inited;
        bool                calibLoaded;
        bool                useKinect;
        
        KinectReproTools::mode  actDrawMode;
    };
}

#endif /* defined(__tav_scene__SNRotatingObjMapping2__) */
