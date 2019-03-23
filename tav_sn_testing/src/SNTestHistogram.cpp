//
// .cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestHistogram.h"

namespace tav
{
    SNTestHistogram::SNTestHistogram(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
    	getChanCols(&chanCols);
        quad = new Quad(-1.f, -1.f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                       chanCols[0].r, chanCols[0].g, chanCols[0].b, chanCols[0].a);
    }

    //---------------------------------------------------------

    void SNTestHistogram::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            _tfo->setSceneNodeColors(chanCols);
        }

        sendStdShaderInit(_shader);
        sendPvmMatrix(_shader, cp);

        //quad->setAlpha(alpha);
        quad->draw(_tfo);
    }

    //---------------------------------------------------------

    void SNTestHistogram::update(double time, double dt)
    {}

    //---------------------------------------------------------

    SNTestHistogram::~SNTestHistogram()
    {
        delete quad;
    }
}
