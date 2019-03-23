//
// SNTThoughtBubble.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTThoughtBubble.h"

namespace tav
{
    SNTThoughtBubble::SNTThoughtBubble(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        nis = kin->getNis();
        nis->setUpdateSkel(true);

        texShader = shCol->getStdTex();
        
        quad = new Quad(-1.f, -1.f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on

        bubbleTex = new TextureManager();
        bubbleTex->loadTexture2D((*scd->dataPath)+"textures/thought_bubble.tif");
        
        kinDebugMatr = glm::mat4(1.f);
        kinDebugMatr = glm::rotate(kinDebugMatr, (float)M_PI, glm::vec3(1.f, 0.f, 0.f));

        texScale = 0.15f;
        
        modMatr = glm::mat4(1.f);
        modMatr = glm::scale(modMatr, glm::vec3(texScale, texScale, 1.f));
        
        headOffset = glm::vec3(0.2f, 0.2f, 0.f);
    }
    
    
    
    SNTThoughtBubble::~SNTThoughtBubble()
    {
        delete quad;
    }
    
    
    
    void SNTThoughtBubble::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
//            _tfo->disableDepthTest();
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
            _tfo->end();
            glDisable(GL_RASTERIZER_DISCARD);
        }
        
//        sendStdShaderInit(_shader);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        
        kin->uploadColorImg();
        
        texShader->begin();
        texShader->setUniform1i("tex", 0);
        texShader->setUniformMatrix4fv("m_pvm", &kinDebugMatr[0][0]);
        
        //useTextureUnitInd(0, kin->getColorTexId(), _shader, _tfo); // zum tfo aufnehmen der texturen
        //        _shader->setUniformMatrix4fv("modelMatrix", &kinDebugMatr[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());
        quad->draw();

//        useTextureUnitInd(0, bubbleTex->getId(), _shader, _tfo); // zum tfo aufnehmen der texturen
//        _shader->setUniformMatrix4fv("modelMatrix", &modMatr[0][0]);
        texShader->setUniformMatrix4fv("m_pvm", &modMatr[0][0]);
        glBindTexture(GL_TEXTURE_2D, bubbleTex->getId());
        quad->draw();

       // _shader->setIdentMatrix4fv("modelMatrix");
        
        if (_tfo) {
            glEnable(GL_RASTERIZER_DISCARD);
            _shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
            _tfo->begin(_tfo->getLastMode());
        }
    }
    
    
    
    void SNTThoughtBubble::update(double time, double dt)
    {
        // get new position
        if (kin->isNisInited() && frameNr != kin->getNisFrameNr())
        {
            frameNr = kin->getNisFrameNr();

            short usr = nis->getNearestUser();

            if (usr >= 0)
            {
                head = nis->getJoint(nis->getNearestUser(), nite::JOINT_HEAD);

                head += headOffset;
                
                modMatr = glm::mat4(1.f);
                modMatr = glm::translate(modMatr, glm::vec3(head.x, head.y, 0.f));
                modMatr = glm::scale(modMatr, glm::vec3(texScale, texScale, 1.f));
            }
        }
    }
    
}
