//
// SNCamchBrightBack.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNCamchBrightBack.h"

#define STRINGIFY(A) #A

namespace tav
{
    SNCamchBrightBack::SNCamchBrightBack(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
    {
    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[ static_cast<GLuint>(_sceneArgs->at("tex0")) ];

    	quad = scd->stdQuad;

        addPar("alpha", &alpha);

        initShader();
    }

    
    SNCamchBrightBack::~SNCamchBrightBack()
    {
        delete quad;
    }

    
    void SNCamchBrightBack::initShader()
    {
    	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

    	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;\n
        layout( location = 1 ) in vec4 normal;\n
        layout( location = 2 ) in vec2 texCoord;\n
        layout( location = 3 ) in vec4 color;\n
        uniform mat4 m_pvm;\n

        out TO_FS {
        	vec2 tex_coord;\n
        } vertex_out;

    	void main() {
    		vertex_out.tex_coord = texCoord;
    	   gl_Position = m_pvm * position;
    	});

        vert = "// basic texture shader, vert\n" + shdr_Header +vert;

        

    	std::string frag = STRINGIFY(
    	in TO_FS {
    	  	vec2 tex_coord;\n
    	} vertex_in;

        layout (location = 0) out vec4 color;\n

    	uniform sampler2D tex;
    	uniform float alpha;

    	void main() {
    		color = vec4(texture(tex, vertex_in.tex_coord).rgb, alpha);
    	});

        frag = "// basic texture shader, frag\n" +shdr_Header +frag;

    	texShdr = shCol->addCheckShaderText("SNCamchLiquid_tex", vert.c_str(), frag.c_str());
    }

    
    void SNCamchBrightBack::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        texShdr->begin();
        texShdr->setIdentMatrix4fv("m_pvm");
        texShdr->setUniform1i("tex", 0);
        texShdr->setUniform1f("alpha", alpha);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, scd->tex[0] );

        quad->draw(_tfo);
    }

    
    void SNCamchBrightBack::update(double time, double dt)
    {}
}
