//
// SNTestGui.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/GWindowManager.h>
#include <GUI/GUIImgSlider.h>
#include <GUI/GUIButton.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>
#include <GUI/Widget.h>

namespace tav
{
    class SNTestGui : public SceneNode
    {
    public:
        SNTestGui(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestGui();

        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);

        void clickBut();
        void swipeImg();
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};

    private:
        Quad*                       quad;
        ShaderCollector*            shCol;
        GWindowManager*             winMan;
        Widget*                     widget;
        unsigned int                widgetWinInd;

        std::string*                dataPath;
        std::vector<std::string>    imgSlidTexs;
    };
}
