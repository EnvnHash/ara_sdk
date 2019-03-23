//
//  SNTNiteSilhoutte.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTNiteSilhoutte.h"

namespace tav
{
    SNTNiteSilhoutte::SNTNiteSilhoutte(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
        shdr = shCol->getStdTex();
        kin = static_cast<KinectInput*>(scd->kin);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        quad = scd->stdQuad;

        userMapTex = new TextureManager();
        edgeDetect = new Shaders("shaders/edgeDetect.vert", "shaders/edgeDetect.frag", true);
        edgeDetect->link();

        nis = kin->getNis();
    }
    
    
    SNTNiteSilhoutte::~SNTNiteSilhoutte()
    {
        delete quad;
        delete userMapTex;
        delete edgeDetect;
    }
    
    
    void SNTNiteSilhoutte::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
            _tfo->end();
            glDisable(GL_RASTERIZER_DISCARD);
        }
        
        if (!inited)
        {
            userMapTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                                 GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMap
            userMapConv = new uint8_t[kin->getDepthWidth() * kin->getDepthHeight()];
            memset(userMapConv, 0, kin->getDepthWidth() * kin->getDepthHeight());
            edgeFBO = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight());
            
            inited = true;
            
        } else
        {
            //sendStdShaderInit(_shader);
            //useTextureUnitInd(0, fluidSim->obstaclesFbo->getColorImg(), _shader, _tfo);
            //useTextureUnitInd(0, fluidSim->getResTex(), _shader, _tfo);

            edgeFBO->bind();
            edgeFBO->clear();

            edgeDetect->begin();
            edgeDetect->setIdentMatrix4fv("m_pvm");
            edgeDetect->setUniform1i("tex", 0);
            edgeDetect->setUniform1f("stepX", 1.f / static_cast<float>(scd->screenWidth));
            edgeDetect->setUniform1f("stepY", 1.f / static_cast<float>(scd->screenHeight));
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, userMapTex->getId());
            
            quad->draw();

            edgeFBO->unbind();
            
            // render result
            shdr->begin();
            shdr->setUniform1i("tex", 0);
            shdr->setIdentMatrix4fv("m_pvm");
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, edgeFBO->getColorImg());
            
            quad->draw(_tfo);
            
            //        fluidSim->draw();
        }
 
        
        if (_tfo)
        {
            glEnable(GL_RASTERIZER_DISCARD);
            _shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
            _tfo->begin(_tfo->getLastMode());
        }
    }
    
    
    void SNTNiteSilhoutte::update(double time, double dt)
    {
        // add users
        if (kin->isNisInited() && inited && frameNr != kin->getNisFrameNr())
        {
            frameNr = kin->getNisFrameNr();

            const nite::UserMap& userLabels = nis->getUserMap();
            const nite::UserId* pLabels = userLabels.getPixels();
            
            // convert userLabels
            float factor = 1;
            uint8_t* pTexRow = userMapConv;
            for (int y=0; y < kin->getDepthHeight() ; ++y)
            {
                uint8_t* pTex = pTexRow; // get the pointer to the first pixel in the actual row
                for (int x=0; x < kin->getDepthWidth() ; ++x, ++pLabels)
                {
                    if (*pLabels == 0) factor = 0; else factor = 255.f;
                    *(pTex++) = factor;
                }
                pTexRow += kin->getDepthWidth();
            }
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, userMapTex->getId());
            glTexSubImage2D(GL_TEXTURE_2D,             // target
                            0,                          // First mipmap level
                            0, 0,                       // x and y offset
                            kin->getDepthWidth(),
                            kin->getDepthHeight(),
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            userMapConv);
            
        }
    }
}
