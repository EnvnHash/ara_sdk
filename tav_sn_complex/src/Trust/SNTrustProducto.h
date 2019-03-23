//
// SNCamchSimpTextur.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/ImageContours.h>
#include <Shaders/Shaders.h>
#include <SceneNode.h>

namespace tav
{
    class SNTrustProducto : public SceneNode
    {
    public:
    	SNTrustProducto(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTrustProducto();

        void initShdr();
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        Quad*   			quad;
        ImageContours*		imgContour;

        ShaderCollector*				shCol;
        Shaders* 			stdTexAlpha;
        Shaders* 			stdColAlpha;
        Shaders* 			prodShdr;

        TextureManager* 	texProd;
        TextureManager* 	texBack;
        TextureManager* 	texDraw;

        glm::mat4 			scalePropoImg;
        glm::mat4 			prodMat;

        double 				lastUpdt;
        int					texNr;

        float				alpha=1.f; // WIEDER AENDERN!!!
        float				morph=0.f;
        float				prodScale=1.f;
        float				prodXPos=0.f;
        float				prodZPos=0.f;
        float				prodYPos =0.f;
    };
}
