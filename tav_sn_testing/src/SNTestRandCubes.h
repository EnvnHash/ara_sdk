//
// SNTestRandCubes.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GeoPrimitives/Cube.h>

#ifdef HAVE_AUDIO
#include <PAudio.h>
#endif

namespace tav
{
    class SNTestRandCubes : public SceneNode
    {
    public:
        SNTestRandCubes(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNTestRandCubes();
    
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        void onKey(int key, int scancode, int action, int mods){};
    private:
        int         nrCubes;
        Cube*       cubes;

#ifdef HAVE_AUDIO
        PAudio*     pa;
#endif

        glm::mat4*  mod_matr;
    };
}
