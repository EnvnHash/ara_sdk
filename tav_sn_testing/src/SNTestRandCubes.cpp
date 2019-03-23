//
// SNTestRandCubes.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestRandCubes.h"

namespace tav
{
    SNTestRandCubes::SNTestRandCubes(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
#ifdef HAVE_AUDIO
    	pa = (PAudio*) scd->pa;
#endif
        nrCubes = 300;
        
        mod_matr = new glm::mat4[nrCubes];
        
        // prepare for instancing, MOD_MATR will change every instance
        std::vector<coordType> instAttribs;
        instAttribs.push_back(MODMATR);

        cubes = new Cube();
//        cubes = new Cube(&instAttribs, nrCubes);
        
//        GLfloat* matrices = (GLfloat*) cubes->getMapBuffer(MODMATR);
        
        // Set model matrices for each instance
        for (int n=0; n<nrCubes; n++)
        {
            glm::mat4 thisMatr = glm::translate(glm::vec3(getRandF(-1.f, 1.f),
                                                          getRandF(-1.f, 1.f),
                                                          getRandF(-1.0f, 0.f)));
            glm::mat4 scaleMatr = glm::scale(glm::vec3(getRandF(0.001f, 0.1f),
                                                       getRandF(0.001f, 0.1f),
                                                       getRandF(0.01f, 0.1f)));
            mod_matr[n] = thisMatr * scaleMatr;
            
//            for (int i=0;i<4;i++)
//                for (int j=0;j<4;j++)
//                    matrices[n*16+i*4+j] = thisMatr[i][j];
        }
        
        //cubes->unMapBuffer();
    }

    //----------------------------------------------------

    SNTestRandCubes::~SNTestRandCubes()
    {
        cubes->remove();
        delete cubes;
    }

    //----------------------------------------------------

    void SNTestRandCubes::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        sendStdShaderInit(_shader);

        for (short i=0;i<nrCubes;i++)
        {
            _shader->setUniformMatrix4fv("modelMatrix", &mod_matr[i][0][0]);
            cubes->draw();
        }
        
//        _shader->setUniform1i("useInstancing", nrCubes);
//        cubes->drawInstanced(nrCubes, _tfo);

    }
    
    //----------------------------------------------------
    
    void SNTestRandCubes::update(double time, double dt)
    {}
    
 }
