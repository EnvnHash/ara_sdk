//
// SNIconParticles.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <GeoPrimitives/Circle.h>
#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/Noise3DTexGen.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/TFO.h>
#include <GLUtils/VAO.h>
#include <SceneNode.h>


namespace tav
{
    class SNIconParticles : public SceneNode
    {
    public:
    	typedef struct {
            float			rotSpeed;
            float			circSize;
            TextureManager*	tex;
            glm::vec4		color;
    	} circPar;

    	typedef struct {
            bool			active;
            int				index;
            double			initTime;
            double 			offsetTime;
            glm::vec4		initPos;
            glm::vec4		offsetPos;
    	} fotoPar;

        SNIconParticles(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNIconParticles();

        void initCircles(sceneData* _scd);
        void initNet();
        void initVoluNet();
        void initVoluNet2();
        void addLine(int ind, glm::vec4* pos, glm::vec3& unitCubeSize,
        		GLfloat* positions, int* posInd);
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void recPoints(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawVoluNet(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawLines(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawPoints(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawCircles(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawClicks(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawIcons(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
        void drawFotos(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);

        void initVoluShdr();
        void initVertShdr();
        void initCircsShdr();
        void initClickedShdr();
        void initDrawShdr();
        void initPointShdr();
        void initPointRecShdr();
        void initIconsShdr();
        void init3dFotoShdr();
        void initGodRayShdr();

        void getScreenTrans(glm::mat4* m_pvm, glm::vec4* quadCoord, glm::vec2* size);
		void genLayerTex(TextureManager** texAr, int nrLayers, GLuint* texNr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:

        FBO*                fotos3d;
        FBO*                godRaysFbo;
        TFO*				pointTfo;
        Circle*				lightCircle;
        glm::vec3			lightPos;
        glm::vec4*			chanCols;
        FastBlurMem*		blur;
        VAO*                singleVert;
        VAO*                part;
        VAO*				rawPart;
        VAO*				fotoPart;
        VAO*				voluNet;
        VAO*				voluNet2;
        Noise3DTexGen*		noiseTex;
        TextureManager*		icons;
        TextureManager***	fotos3dTexs;
        GLuint*				fotos3dLayerTexs;
        Quad*				quad;

        ShaderCollector*				shCol;

        Shaders*			circsShdr;
        Shaders*			clickShdr;
        Shaders*			drawShdr;
        Shaders*			foto3dShdr;
        Shaders*			iconsShdr;
        Shaders*			godRayShdr;
        Shaders*			pointShdr;
        Shaders*			voluShdr;
        Shaders*			recPointShdr;
        Shaders*			stdTexShdr;
        Shaders*			stdTexAlpaShdr;
        Shaders*			stdColShdr;

        circPar*			circPars;
        fotoPar*			fotoPars;

        bool                inited = false;

        double 				lastUpdt;

        float				timeVar;
        float				iconsBaseSize;
        float				clickDur;
        float 				fotoZoomDur;
        float				fotoZoomInitZ;
        float 				foto3dZDist;
        float 				propo;
        float				perlCoordScale;
        float 				iconAlpha;


        float				netDistAmt = 0.6f;
        float				netZDistAmt = 1.f;
        float				zModDepth = 0.6f;
        float				clickIcon = -1.f;
        float				circSize = 0.38f;
        float				iconSize = 1.f;

        float				foto3dZOffs = 0.1f;
        float				foto3dYScale = 1.f;
        float				foto3dXScale = 1.f;
    	//addPar("foto3dZPos", & 0.f;

        float				netAlpha = 0.f;
        float				voluNetAlpha = 0.f;
        float				voluNetZOffs = 0.f;
        float				voluDrawDepth = 1.f;
        float				voluNetDepth = 10.f;
        float				voluNetSizeX = 1.f;
        float				voluNetSizeY = 1.f;
        float				voluColBlend = 0.4f;
        float				perlTimeModu = 1.f;
        float				alpha = 0.f;
        float				lightAlpha = 0.f;
        float				godLineCol = 0.f;
        float				recPointTY = 1.f;
        float				recPointSY = 1.f;


        int					nrCircs;
        int					maxNrFotos3d;
        int					nrFotos3d;
        int					iconCountDiv;
        int					nrLayersFotos3d;

    	int 				nrRows;
    	int 				nrCols;
    	int 				nrZRows;

        glm::ivec2			fotos3dFboSize;
        glm::ivec2			nrIcons;
        glm::ivec2			nrCircus;
        std::string			vertShdr;
        std::string 		basicVert;

        glm::mat4			netAdjust;
        glm::mat4			drawMvp;
        glm::mat4 			pointRecModMat;
        glm::vec3 			unitCubeSize;
    };
}
