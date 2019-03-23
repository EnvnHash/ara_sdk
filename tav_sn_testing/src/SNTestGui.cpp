//
// SNTestGui.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestGui.h"

using namespace std;

namespace tav
{
    SNTestGui::SNTestGui(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    	winMan = static_cast<GWindowManager*>(_scd->winMan);

        widgetWinInd = 0;
        
        // build widget
        widget = new Widget();
        widget->setPos(0.f, 0.f);
        widget->setSize(2.f, 2.f);
        widget->setBackColor(1.f, 1.f, 1.f, 1.f);
        widget->setBackTex((*(_scd->dataPath))+"textures/blue-abstract-background.jpg");
        
        // add image slider
        imgSlidTexs.push_back((*(_scd->dataPath))+"textures/landscape0.jpg");
        imgSlidTexs.push_back((*(_scd->dataPath))+"textures/landscape1.jpg");
        imgSlidTexs.push_back((*(_scd->dataPath))+"textures/landscape2.jpg");
        imgSlidTexs.push_back((*(_scd->dataPath))+"textures/landscape3.jpg");
        
        widget->add( new GUIImgSlider(glm::vec2(0.f, 0.1f), glm::vec2(1.1f, 1.1f) ) );
        widget->getLastGuiObj()->setBackColor(0.f, 0.f, 0.f, 1.f);
        widget->getLastGuiObj()->setColor(0.f, 0.f, 0.f, 1.f);
        widget->getLastGuiObj()->setTextures(imgSlidTexs);
        widget->getLastGuiObj()->setAction(SWIPE_RIGHT, [this]() { return this->swipeImg(); });
        widget->getLastGuiObj()->setActionAnim(SWIPE_RIGHT, GUI_MOVE_RIGHT);
        widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_RIGHT, 1.0);
        
        
        // build buttons
        int nrButton = 4;
        float totWidth = 0.8f;
        float singWidth = (totWidth * 2.f) / static_cast<float>(nrButton);
        singWidth *= 0.6f;

        for (int i=0;i<nrButton;i++)
        {
            float fCol = static_cast<float>(i) / static_cast<float>(nrButton);
            fCol = fCol * 0.4f + 0.6f;
            
            float xPos = static_cast<float>(i) / static_cast<float>(nrButton);
            xPos = (xPos * 2.f - 1.f) * 0.5f + 1.f / static_cast<float>(nrButton);
            xPos *= totWidth;
            xPos -= (1.f - totWidth) * 0.5f;
            
            widget->add(new GUIButton());
            
            widget->getLastGuiObj()->setPos(xPos, -0.2f);
            widget->getLastGuiObj()->setSize(singWidth, 0.4f);
            widget->getLastGuiObj()->setBackColor(std::fmin(fCol * 1.3f, 1.f), fCol, 0.2f, 1.f);
            widget->getLastGuiObj()->setFont(((*(scd->dataPath))+"fonts/Arial.ttf").c_str(), 16);
            widget->getLastGuiObj()->setLabel("gTtach", CENTER_X, CENTER_Y);
            widget->getLastGuiObj()->setColor(0.f, 0.f, 0.f, 1.f);
            widget->getLastGuiObj()->setAction(LEFT_CLICK, [this]() { return this->clickBut(); });
            widget->getLastGuiObj()->setActionAnim(LEFT_CLICK, GUI_BLINK_ONCE);
            widget->getLastGuiObj()->setActionAnimDur(GUI_BLINK_ONCE, 0.4);
            widget->getLastGuiObj()->setHighLightColor(LEFT_CLICK, 0.8f, 0.4f, 0.4f, 1.f);
        }

        winMan->addWidget(0, widget, shCol);
    }

    
    SNTestGui::~SNTestGui()
    {
    }
    
    
    void SNTestGui::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        sendStdShaderInit(_shader);
        
        //std::cout << "------------------- drawing widget ----------------------- " << std::endl;
        winMan->drawWidget(widgetWinInd);
    }

    
    void SNTestGui::clickBut()
    {
    }

    
    void SNTestGui::swipeImg()
    {
    }
    
    
    void SNTestGui::update(double time, double dt)
    {}
    
}
