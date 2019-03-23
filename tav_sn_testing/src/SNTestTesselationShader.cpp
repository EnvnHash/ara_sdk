//
// SNTestTesselationShader.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  most basic vao test
//
#define STRINGIFY(A) #A

#include "SNTestTesselationShader.h"

namespace tav
{
    SNTestTesselationShader::SNTestTesselationShader(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        // vao to init the particle system
        part = new VAO("position:3f", GL_STATIC_DRAW);

        GLfloat vertex_positions[] =
        {
            -0.75f, -0.75f, 0.f,
             0.75f, -0.75f, 0.f,
             -0.75f,  0.75f, 0.f,
             0.75f,  0.75f, 0.f
//
//             0.75f, -0.75f,  0.25f,
//             0.75f,  0.75f,  0.25f,
//            -0.75f, -0.75f,  0.25f,
//            -0.75f,  0.75f,  0.25f
        };

        part->upload(tav::POSITION, &vertex_positions[0], 4);

        fullScrCam = new GLMCamera();        // standard frustrum camera

        initTessShdr();
    }

    
    void SNTestTesselationShader::initTessShdr()
    {
         //- Position Shader ---

         std::string shdr_Header = "#version 420 core\n";

         std::string vert = STRINGIFY(layout (location=0) in vec4 position;
                                      void main() {
                                          gl_Position = position;
                                      });
         vert = shdr_Header +vert;


         std::string tessContr = STRINGIFY(
         layout (vertices=4) out;
         void main(void)
         {
             if (gl_InvocationID == 0)
             {
                 gl_TessLevelInner[0] = 4.0;
                 gl_TessLevelInner[1] = 4.0;
                 gl_TessLevelOuter[0] = 4.0;
                 gl_TessLevelOuter[1] = 4.0;
                 gl_TessLevelOuter[2] = 4.0;
                 gl_TessLevelOuter[3] = 4.0;
             }
             gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
         });
         tessContr = shdr_Header +tessContr;


         std::string tessEval = STRINGIFY(
        	            layout (quads, fractional_odd_spacing, ccw) in;

        	            uniform mat4 mv_matrix;
        	            uniform mat4 proj_matrix;

        	            void main(void)
        	            {
        	                vec4 mid1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
        	                vec4 mid2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
        	                vec4 pos = mix(mid1, mid2, gl_TessCoord.y);
        	                pos.xyz = (pos.xyz) * 0.25;
        	                gl_Position = proj_matrix * mv_matrix * pos;
        	            });
         tessEval = shdr_Header+tessEval;



         std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                      void main()\n
                                      {\n
                                          fragColor = vec4(1.0);\n
                                      });
         frag = shdr_Header+frag;

         tessShader = shCol->addCheckShaderText("SNTestTesselationShader",
                                                             vert.c_str(), tessContr.c_str(), tessEval.c_str(), nullptr, frag.c_str());
    }


    void SNTestTesselationShader::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        int i;
        static const GLfloat green[] = { 0.0f, 0.25f, 0.0f, 1.0f };
        static const GLfloat one = 1.0f;

        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glm::mat4 m_pv = cp->projection_matrix_mat4 * cp->view_matrix_mat4;

        tessShader->begin();
        tessShader->setUniformMatrix4fv("proj_matrix", &m_pv[0][0]);
        tessShader->setUniformMatrix4fv("mv_matrix", &_modelMat[0][0]);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        part->bind();
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        //part->drawElements(GL_PATCHES);
        glDrawArrays(GL_PATCHES, 0, 4);
    }
    

    void SNTestTesselationShader::update(double time, double dt)
    {}
    

    SNTestTesselationShader::~SNTestTesselationShader()
    { }

}
