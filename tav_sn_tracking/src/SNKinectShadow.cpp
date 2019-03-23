//
//  SNKinectShadow.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNKinectShadow.h"

namespace tav
{
    SNKinectShadow::SNKinectShadow(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
  	kin = static_cast<KinectInput*>(scd->kin);

        quadAr = new QuadArray(38, 38, -1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f));
        quadAr->rotate(float(M_PI) * 0.5f, 1.f, 0.f, 0.f);
        //quadAr->setColor(1.f, 0.f, 0.f, 1.f);

        blur = new FastBlur(static_cast<OSCData*>(scd->osc), shCol, 512, 512);
        kin->setUpdateShadow(true);
    }
    
    
    SNKinectShadow::~SNKinectShadow()
    {
        delete blur;
        delete quadAr;
    }
    

    void SNKinectShadow::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        glDisable(GL_DEPTH_TEST);
        
        sendStdShaderInit(_shader);
        useTextureUnitInd(0, blur->getResult(), _shader, _tfo);
        quadAr->draw(_tfo);
    }
    

    void SNKinectShadow::update(double time, double dt)
    {
        kin->uploadShadowImg();
        blur->proc(kin->getShdwTexId());
    }
    

    void SNKinectShadow::onKey(int key, int scancode, int action, int mods)
    {}}
