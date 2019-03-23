//
// SNTestVao.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  most basic vao test
//

#include "SNTestVao.h"

namespace tav
{
    SNTestVao::SNTestVao(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
        // vao to init the particle system
        int nrPnts = 4;
        
        part = new VAO("position:3f,normal:3f,texCoord:2f,color:4f", GL_STATIC_DRAW);
        part->initData(nrPnts);
        
        float* tp = new float[nrPnts*3];
        for (auto i=0;i<nrPnts;i++)
        {
            tp[i*3 ] = (((float)i + .5f) / (float)nrPnts) * 2.f -1.f;
            tp[i*3 +1] = 0.f;
            tp[i*3 +2] = 0.f;
        }
        
        part->upload(tav::POSITION, tp, nrPnts);
        
        float* tc = new float[nrPnts*4];
        for (auto i=0;i<nrPnts;i++)
        {
            tc[i*4 ] = 1.f;
            tc[i*4 +1] = 1.f;
            tc[i*4 +2] = 1.f;
            tc[i*4 +3] = 1.f;
        }
        
        part->upload(tav::COLOR, tc, nrPnts);
        
        
        colShader = new Shaders("shaders/basic_col.vert", "shaders/basic_col.frag", true);
        glLinkProgram(colShader->getProgram());

        fullScrCam = new GLMCamera();        // standard frustrum camera
        
    }
    
    
    SNTestVao::~SNTestVao()
    {
    	delete part;
    	delete fullScrCam;
    	delete colShader;
    }
    
    void SNTestVao::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        colShader->begin();
        colShader->setIdentMatrix4fv("m_pvm");
        
        part->draw(GL_POINTS);

        colShader->end();        
    }

    void SNTestVao::update(double time, double dt)
    {}

}
