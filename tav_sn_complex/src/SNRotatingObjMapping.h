//
//  SNRotatingObjMapping.h
//  tav_scene
//
//  Created by Sven Hahne on 4/10/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __tav_scene__SNRotatingObjMapping__
#define __tav_scene__SNRotatingObjMapping__


#pragma once

#include <stdio.h>
#include <iostream>
#include <opencv2/core.hpp>

#include <KinectInput/KinectInput.h>

#include <Communication/Arduino.h>
#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Line.h>
#include <GeoPrimitives/Quad.h>
#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/TextureBuffer.h>
#include <GLUtils/TextureManager.h>
#include <headers/gl_header.h>

#include <Shaders/Shaders.h>
#include <AnimVal.h>
#include <Median.h>
#include <PAudio.h>
#include <SceneNode.h>
#include <UDPServer.h>


namespace tav
{
    class SNRotatingObjMapping : public SceneNode
    {
    public:
        enum drawMode { CHESSBOARD, CALIBRATE, CHECK, CHECK_LOADED, DRAW, TESTDIST };
        
        typedef struct
        {
            glm::mat4   rotXY;
            glm::mat4   rotZ;
            glm::mat4   trans;

            glm::vec3   camBeamerRealOffs;
            glm::vec3   chessNormal;
            glm::vec3   rotations;

            float       beamerFovY;
            float       beamerLookAtAngle;
            float       kinFovX;
            float       kinFovY;
            float       distBeamerObjCenter;
            
            cv::Size    boardSize;
        } kpCalibData;
        
        
        SNRotatingObjMapping(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNRotatingObjMapping();
        
        void initDyn();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        std::string execCmd(char* cmd);
        glm::mat4 getRotReal(kpCalibData& kp);
        glm::mat4 calcChessRotMed(std::vector< Median<glm::vec3>* >& _realWorldCoord, kpCalibData& kp);
        void reCheckRot(kpCalibData& kp);
        void getKinRealWorldCoord(glm::vec3& inCoord);
        glm::vec3 getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp);
        void loadCalib(std::string _filename, kpCalibData& kp);
        void saveCalib(std::string _filename, kpCalibData& kp);
        glm::mat4 cvMatToGlm(cv::Mat& _mat);
        cv::Mat glmToCvMat(glm::mat4& mat);

        void swapBlendTex(float* startInd, float* endInd);
        void addLitSphereShader();

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
        GWindowManager*     winMan;
        PAudio*             pa;
        SceneNode*       	busNode;
        UDPServer*          udp;
        Arduino*            arduino = 0;
        OSCData*            osc;
        std::string*        dataPath;
        
        PropoImage*         chessBoard;
        PropoImage*         chessBoard2;
        
        FastBlur*           fastBlur;
        TextureManager*     kinDepthTex;
        TextureManager*     switchTextures;
        TextureManager*     cubeTex;
        TextureManager*     litsphereTexture;
        TextureManager*     normalTexture;
        TextureManager*     lucesTex;

        TextureBuffer*      audioBuf;

        kpCalibData         kpCalib;

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
        double              lastSwitchTime=0.0;
        double              switchTexInt=2.0;
        
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
        
        unsigned int        lastAudioFrameNr;
        
        bool                ardInit;
        bool                useArduino;
        bool                inited;
        bool                calibLoaded;
        bool                useKinect;
        bool                checkCross;
        
        drawMode            actDrawMode;
    };
}

#endif /* defined(__tav_scene__SNRotatingObjMapping__) */
