//
// SNVektorTunnel.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <utility>
#include <cmath>

#include <SceneNode.h>

#include <Communication/OSC/OSCData.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/Typo/FreetypeTex.h>
#include <GLUtils/FBO.h>
#include "GLUtils/glm_utils.h"
#include <GLUtils/Spline3D.h>
#include <GLUtils/TextureManager.h>
#include <math_utils.h>

namespace tav
{
    class SNVektorTunnel : public SceneNode
    {
    	struct keywordPar {
    		glm::mat4		rotMat;
    		glm::mat4		transMat;
    		glm::mat4		scaleMat;
    		glm::mat4		scrPosTrans;
    		glm::vec4		color;
    		TextureManager* type;
    		float 			flipX = 0.f;
    		float 			alpha = 1.f;
    		float			propo=1.f;
    		float 			blendOffs = 0.f;
    		float			texPropo = 1.f;
    	};

    public:
        SNVektorTunnel(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNVektorTunnel();
        void buildTunel();
        void calcCircNormals();
        void calcCircs();
        void buildLines();
        void buildKeyWords();
        void buildRobot();
    
        float frand();
        float sfrand();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void drawLines(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
        		float alpha);
        void drawPoints(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
        		float alpha);
        void drawTypo(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
        		float alpha);
        void drawRobot(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
        		GLfloat* projM, GLfloat* viewM);
        void drawRobotCloseUp(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
        		GLfloat* projM, GLfloat* viewM);
        void drawRobotCloseUpFbo(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);

        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
        void updateGeo();
        void updateVAO(double _time);
        void initPointShader();
        void initLineShader();
        void initTypoShader();
        void initLitSphereShader();
        void initLitSphereAllVertShader();
    private:
        SceneNode*		robotNode;
        OSCData*        osc;
        Quad*			quad;

        std::vector< std::vector<VAO*> >		robotNodes;
        std::vector< std::vector<MaterialProperties*> > robotMaterials;
        std::vector< glm::mat4 >				robotBaseNodeMats;
        std::vector< glm::vec3 >				robotBaseNodeAxisCenters;
        std::vector< std::vector<glm::mat4> >	robotNodeMats;
        std::vector< std::vector<glm::vec3> >	robotNodeAxisCenters;

        Spline3D		tunelBasePath;
        Spline3D		tunelNormalPath;

        FBO*			closeUpFbo;

        TextureManager* litTex;
        TextureManager* cubeTex;
        TextureManager* robotLitTex;
        TextureManager* robotLitBlackTex;
        TextureManager* normalTexture;

        VAO*			pointVao;
        VAO*			lineVao;
        GLMCamera*		sideCam;
       // GLMCamera*		centerCam;

        ShaderCollector*				shCol;

        Shaders*		pointShader;
        Shaders*		lineShader;
        Shaders*		stdTextShader;
        Shaders*		stdDirLight;
		Shaders*		typoShader;
		Shaders*		robotLitShader;

        glm::mat4 		modelScale;
        glm::mat4 		centerCamProj;
        glm::mat4 		centerCamView;
        glm::mat4 		closeUpCam;

        std::vector<glm::vec3> 	tunelBasePathRefPoints;
        std::vector<glm::vec3> 	circBasePoints;
        std::vector<glm::vec3> 	circNormals;
        std::vector< std::vector<glm::vec3> > 	circlesAsPoints;

        std::vector<std::string> 	keyWords;
        std::vector<FreetypeTex*> 	types;
        std::vector<keywordPar>		kwPar;

        float*			circPointsFlat;
        float*			lines;
        float*			linesNormals;
        float*			linesColors;
        
        int             nrCircSegments;
        int             nrZSegments;
    	int				connectNrPointsZ;	// heisst einen +1 und einen -1
    	int				connectNrPointsCirc;
    	int				nrLines;
    	int				keyWordCircMod1;
		int				keyWordCircMod2;
		int				keyWordZMod;

        float           circRadius;
        float 			tunelLength;
        float			propo;
        float 			tunnelRenderDepth;
        float 			totemAndCenter;
        float			tunnelXDist;
        float			tunnelYDist;

        float			tunRenderDpth =10.f;
        float			alpha =0.f;
        float			pathPos =0.f;
        float			typoBlend =0.f;
        float			typoPosMin =0.3f;	// index relativ auf die Laenge des Tunnels, ab wo keywords auftauchen sollen
        float			typoPosMax =0.6f;	// index relativ auf die Laenge des Tunnels, bis wohin keywords auftauchen sollen
    	float			typoAlpha =0.f;
    	float			typoScale =0.4f;
    	float			typoBlendDist =3.f;
    	float			typoStayAmt =0.4f;
    	float			typoFadeOut =2.f;
    	float			robotAlpha =0.f;
    	float			robotPathPos =0.5f;
    	float			robotPathToCam =0.f;
    	float			robotXOffs =0.f;
    	float			robotYOffs =0.f;
    	float			robotSize =1.f;
    	float			closeCamAngle =0.f;
    	float			closeCamDist =0.f;
    	float			camBlend =0.f;
    	float			axis1 =0.f;
    	float			axis2 =0.f;
    	float			axis3 =0.f;
    	float			axis4 =0.f;
    	float			zange =0.f;
    	float			verbinder =0.f;

    	float			sideCamAlpha=0.f;

        glm::vec4*		chanCols;
        glm::vec3 		stdNormal;
        glm::vec3 		camPos;
        Median<glm::vec3>*	camNormal;
        glm::quat 		rot;

        glm::mat4 		rotMat;
        glm::mat4 		camModMatr;
        glm::mat4		robotPos;
    	glm::mat4 		clawLevel;
    	glm::mat4 		thisModelMat;
    	glm::mat4		hierachicalMat;

    	glm::mat4		proj;
    	glm::mat4		view;

        glm::mat4 		m_view;
        glm::mat3 		thisNormMat;
        glm::mat3   	m_normal;

    };
}
