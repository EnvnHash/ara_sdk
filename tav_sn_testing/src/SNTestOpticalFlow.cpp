//
//  TestOpticalFlow.cpp
//  tav_scene
//
//  Created by Sven Hahne on 13/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNTestOpticalFlow.h"

using namespace std;
namespace tav
{
    SNTestOpticalFlow::SNTestOpticalFlow(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), bInit(false), texPtr(0)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[ static_cast<GLuint>(_sceneArgs->at("tex0")) ];
    	tex1 = texs[ static_cast<GLuint>(_sceneArgs->at("tex1")) ];

        quad = scd->stdQuad;

        texShader = shCol->getStdTex();
        flow = new GLSLOpticalFlow(shCol, tex0->getWidth(), tex0->getHeight());
        
        kinImgs = new TextureManager*[2];
        for (int i=0;i<2;i++) kinImgs[i] = new TextureManager();
    }
    
    
    SNTestOpticalFlow::~SNTestOpticalFlow()
    {
        delete quad;
    }
    

    void SNTestOpticalFlow::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if(!bInit)
        {
            for (int i=0;i<2;i++)
                kinImgs[i]->allocate(tex0->getWidth(), tex0->getHeight(),
                                     GL_RGB8, GL_RGB, GL_TEXTURE_2D);
            bInit = true;
        } else
        {
            flow->update(tex0->getId(), tex1->getId());

            texShader->begin();
            texShader->setIdentMatrix4fv("m_pvm");
            texShader->setUniform1i("tex", 0);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, flow->getResTexId());
            
            quad->draw(_tfo);
        }
    }
    

    void SNTestOpticalFlow::update(double time, double dt)
    {}
    
}
