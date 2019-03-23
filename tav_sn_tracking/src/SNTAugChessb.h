//
// SNTAugChessb.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <opencv2/core.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <GeoPrimitives/Line.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/GLSL/GodRays.h>
#include <GLUtils/GLSL/FastBlur.h>
#include <GLUtils/GLMCamera.h>
#include <GLUtils/PropoImage.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/VAO.h>
#include <GeoPrimitives/Quad.h>
#include <GeoPrimitives/QuadArray.h>
#include <KinectInput/KinectInput.h>
#include <OpenCvUtils/CvKalmanFilter.h>
#include <Median.h>
#include <Shaders/Shaders.h>
#include <Shaders/ShaderCollector.h>


#define GLM_FORCE_RADIANS

#include <SceneNode.h>

#include <GLFW/glfw3.h>
#include <headers/gl_header.h>

namespace tav
{
    class SNTAugChessb : public SceneNode
    {
    public:
        enum drawMode { KIN_RGB=0, CHECK_KIN_OVERLAY=1, CHESS_TO_CAM=2, CHECK_CAM_TO_BEAMER=3, SAVE_CAM_TO_BEAMER=4,
            CHECK_KIN_UNWARP=5, CHECK_BEAMER_TO_OBJ=6, SAVE_BEAMER_TO_OBJ=7 };
        enum fdbkMode { MEASSURE_CHESSWIDTH=0, DRAW_WITH_CHESSWIDTH=1, CHECK_NORMAL=2, DRAW_RESULT=3};
        enum beamerModel { ZOOM=0, WIDE=1, ULTRA_WIDE=2 };

        typedef struct
        {
            bool        invertMatr;
            cv::Size    imgSize;    // image size of the camera input
            cv::Size    depthImgSize;
            cv::Size    boardSize;
            cv::Size    virtBoardSize;
            int         nrSamples;
            float       beamerAspectRatio;
            float       beamerFovX;
            float       beamerFovY;
            float       beamerLookAngle;
            float       beamerLowEdgeAngle;
            float       beamerThrowRatio;
            float       beamerWallDist;
            float       camAspectRatio;
            float       camFovX;
            float       camFovY;
            float       camWallRectReal;
            float       chessRealWidth;
            
            float       distBeamerObj;
            
            glm::mat4   chessRotXY;
            glm::mat4   chessRotZ;
            glm::mat4   chessTrans;

            glm::mat4   camBeamerRotXY;
            glm::mat4   camBeamerRotZ;
            glm::mat4   camBeamerTrans;

            glm::vec2   oniFov;
            
            glm::vec3   chessNormal;
            glm::vec3   beamerMidToKinect;
            glm::vec3   camBeamerRealOffs;
            glm::vec3   beamerRealMid;
            glm::vec3   chessRealMid;
            glm::vec3   camBeamerRots;
            glm::vec3   rotations;
            glm::vec3   objRotations;
            
            beamerModel beamModel;
            cv::Mat     cameraMatrix;
            cv::Mat     distCoeffs;
            cv::Mat     camBeamerPerspTrans;
            cv::Mat     camBeamerInvPerspTrans;
        } kpCalibData;
        
        SNTAugChessb(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTAugChessb();
        
    
        void initKinDependent();
        void initLitShader();
        void update(double time, double dt);
        virtual void getChessCorners();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void drawChessToCam(double time, double dt, camPar* cp);
        void drawKinOverlay();
        void drawCamToBeamer();
        void drawCamToBeamerInv();
        void drawKinUnwrap();
        void drawBeamerToObj();
        
        void getChessWidth(float& _beamerDist, float& _chessWidth, float _scaleFact);
        void getRealWorldKinBeamerOffs(kpCalibData& kp);
        void getKinRealWorldCoord(glm::vec3& inCoord);
        void getKinRealWorldCoordUnwarp(glm::vec3& inCoord, glm::mat4 _perspMatr);
        
        glm::mat4 rtMatToGlm(int imgW, int imgH, cv::Mat& rVec, cv::Mat& tVec, bool inv=false);
        glm::mat4 getBeamerFromCv(cv::Mat& camMatr, float calibWidth, float calibHeight, float znear,
                                float zfar, bool yUp=true);
        
        cv::Mat getPerspTrans(std::vector<cv::Point2f>& inPoints, float _chessWidth, float _chessHeight);
        glm::mat4 getRotRealToGL(kpCalibData& kp);
        glm::mat4 getRotRealToGLScreen(kpCalibData& kp);
        glm::mat4 getRotRealToCamToGL(kpCalibData& kp);
        glm::mat4 getRotRealToCamUnwrap(kpCalibData& kp);
        
        glm::vec3 getRealOffs(glm::vec3& point, kpCalibData& kp, glm::mat4& rotXMat);
        glm::vec3 getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp, cv::Size boardSize);
        
        glm::mat4 calcChessRotXYWall(std::vector<cv::Point2f> _pointbuf, kpCalibData& kp, cv::Size boardSize);
        glm::mat4 calcChessRotXY(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp, cv::Size boardSize);
        glm::mat4 calcChessRotMed(std::vector< Median<glm::vec3>* >& _realWorldCoord, kpCalibData& kp, cv::Size boardSize);
        glm::mat4 calcChessRotZ(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp, cv::Size boardSize, glm::vec3 _rotations);
        glm::mat4 calcChessTrans(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp, cv::Size boardSize);

        void reCheckRot(kpCalibData& kp);

        glm::mat4 getBeamerPerspTrans(kpCalibData& kp, glm::vec3& _chessPos);
        void doPerspUndist(std::vector<glm::vec4>& inPoints, std::vector<glm::vec3>& outPoints, glm::mat4& perspMatr);
        void perspUnwarp(glm::vec2& inPoint, glm::mat4& perspMatr);
        
        void loadCamToBeamerCalib(cv::FileStorage& fs, kpCalibData& kp);
        void saveCamToBeamerCalib(std::string _filename, kpCalibData& kp);
        void saveBeamerToObjCalib(std::string _filename, kpCalibData& kp);
        
        cv::Mat glmToCvMat(glm::mat4& mat);
        glm::mat4 cvMatToGlm(cv::Mat& _mat);
        glm::mat4 cvMat33ToGlm(cv::Mat& _mat);
        inline double getRotX(glm::vec3& _normal);
        inline double getRotY(glm::vec3& _normal);

        void onKey(int key, int scancode, int action, int mods);
        void onCursor(GLFWwindow* window, double xpos, double ypos);
        void onMouseButton(GLFWwindow* window, int button, int action, int mods);
        
        std::string execCmd(char* cmd);

    private:
        KinectInput*    	kin;
        OSCData*        	osc;
        AssimpImport*       aImport;
    	GodRays*			godRays;
    	SceneNode* 			aModelNode;

        Quad*           	quad;
        Quad*           	noFlipQuad;
        Quad*           	whiteQuad;
        Quad*           	kinQuad;
        
        VAO*            	contrPoints;

        QuadArray*      	chessQA;
        
        Line*           	xLine;
        Line*           	yLine;
        
        ShaderCollector* 	shCol;
        Shaders*        	shdr;
        Shaders*        	colShader;
        Shaders*        	blendShader;
        Shaders*            litShdr;

        TextureManager* 	kinTex;
        TextureManager* 	kinDepthTex;
        TextureManager*		litsphereTex;
        TextureManager*		cubeTex;

        PropoImage*     	chessBoard;
        PropoImage*     	chessBoard2;
        PropoImage*     	chessBoard3;
        PropoImage*     	chessBoard_big;
        
        Median<glm::vec3>* 	cbNormal;
        Median<glm::vec3>* 	cbMidPoint;
        Median<glm::vec4>* 	cbLeftVec;
        Median<glm::vec3>* 	kinCamOffset;
        
        Median<float>*  	chessRealWidth;
        Median<float>*  	beamerWidth;
        Median<float>*  	beamerHeight;
        Median<float>*  	beamerWallDist;
        Median<double>* 	rotX;
        
        std::vector< Median<glm::vec3>* >rwCoord;
        
        CvKalmanFilter* 	kalmanRot;
        CvKalmanFilter* 	kalmanRotZ;
        CvKalmanFilter* 	kalmanTrans;
        GLuint          	colorTexNr;
        
        FastBlur*       	fastBlur;

        kpCalibData     	kpCalib;
        
        bool            isInited = false;
        bool            beamerUpsideDown;
        bool            invertCamBeamerMatr;
        bool            switchBeamerToObj = false;
        bool            saved = false;
        bool			gotChess = false;
        bool			startFirstChessThread = true;
        bool			gotNewChess = false;

        int             gotCamToBeamer = 0;
        int             gotBeamerToObj = 0;
        int             beamerToObjIntv;
        
        int             frameNr = -1;
        int             depthFrameNr = -1;
        int             getNrCheckSamples;
        int             measuredSamples = 0;
        int             nrMeasureSamples;
        int             nrDistIterations = 0;
        
        int             measuredChessWidth = 0;

        int             deDist = 0;
        
        float           beamerLowEdgeAngle;
        float           beamerThrowRatio;
        
        float           glChessBoardWidth;
        float           glChessBoardHeight;
        
        float           chessOffsX=0.f;
        float           chessOffsY=0.f;
        
        float           kinFovY;
        float           kinFovX;
        float           kinRealWorldXYScale;
        
        float           tBeamerDistRectReal = 1.f;
        float           oldBeamerDistRectReal = 800.f;
        
        float           realToNorm;
        float           realBoardWidth;
        float           coordNormFact;
        
        float           distScaleFact = 1.f;
        
        Median<float>*  lastChessWidth;
        Median<float>*  lastBeamerDist;
        Median<float>*  newChessWidth;
        Median<float>*  newBeamerDist;
        
        Median<glm::vec3>* rotations;

        uint16_t*       depthData;
        uint16_t*       depthDataBlur;
        
        drawMode        actDrawMode;
        fdbkMode        actFdbkMode;
                
        std::string     camBeamerMatrPath;
        
        cv::Mat         testChessFile;
        cv::Mat         map1, map2;
        cv::Mat         view, rview;
        cv::Mat         viewDepth, viewDepthBlur;
        cv::Mat         viewGray;
        cv::Mat         checkWarp, checkWarpDepth;
        
        GLMCamera       mapCam;

        std::vector<cv::Point2f> pointbuf;
        std::vector<glm::vec4> chessbXyPlane;
    	std::vector<glm::vec3> chessRealWorldCoord;

        glm::vec3       objNormal;
        glm::vec4			lightCol;

        glm::mat4       chessbMatr;
        glm::mat4       chessRotMatr;
        glm::mat4       camBeamerRotMatr;
        
        glm::mat4       beamPerspTrans;

        glm::mat4       objRotMatr;
        glm::mat4       objRotXYMatr;
        glm::mat4       objRotZMatr;
        glm::mat4       objTrans;

		boost::thread* 	m_Thread;
    };
}
