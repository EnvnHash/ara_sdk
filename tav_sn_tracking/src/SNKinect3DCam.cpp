//
// SNKinect3DCam.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Funktioniert nur mit blending = true
//

#include "SNKinect3DCam.h"

#define STRINGIFY(A) #A

namespace tav
{
    SNKinect3DCam::SNKinect3DCam(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs,"DirLightNoTex"), texGridSize(256)
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    	osc = static_cast<OSCData*>(scd->osc);
    	kin = static_cast<KinectInput*>(scd->kin);

    	TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

        quadAr = new QuadArray(125, 125, -1.f, -1.f, 2.f, 2.f,
                               1.f, 1.f, 1.f, 1.f);

        quad = scd->stdQuad;

        modelMat = glm::mat4(1.f);
        normalMat = glm::mat3( glm::transpose( glm::inverse( modelMat ) ) );
        
        posTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F,
                         GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
        normTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F,
                          GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
        
        initPreCalcShdr();
        
        stdTexShader = shCol->getStdTex();
    }
    
    
    void SNKinect3DCam::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if(!inited)
        {
            initWaveShdr(_tfo);
            inited = true;
        }
        
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            _tfo->setSceneProtoName(&protoName);
            _tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
            waveShdr->begin();
            _tfo->begin(GL_TRIANGLES);
        }
        
        
        glDisable(GL_CULL_FACE);
        
        sendStdShaderInit(waveShdr);
        useTextureUnitInd(0, tex0->getId(), waveShdr, _tfo);
        
        //modelMat = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 1.f) );
        modelMat = glm::rotate( glm::mat4(1.f), float(osc->speed * M_PI * 0.25), glm::vec3(0.f, 1.f, 0.f) );
//        modelMat = glm::rotate( glm::mat4(1.f), float(std::sin(time * 0.5) * M_PI * 0.25), glm::vec3(0.f, 1.f, 0.f) );

        normalMat = glm::mat3( glm::transpose( glm::inverse( modelMat ) ) );
        
        waveShdr->setUniformMatrix4fv("modelMatrix", &modelMat[0][0]);
        waveShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
        waveShdr->setUniformMatrix4fv("projectionMatrix", cp->multicam_projection_matrix);
        waveShdr->setUniform1i("posTex", 0);
        waveShdr->setUniform1i("normTex", 1);
        waveShdr->setUniform1i("colorTex", 2);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normTex->getColorImg());

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, kin->getColorTexId() );
        
        quadAr->draw(_tfo);

        if (_tfo)
        {
            _tfo->end(); // tfo muss angehalten werden, damit der shader gewechselt werden kann
            _shader->begin();
            _tfo->begin(GL_TRIANGLES);
        }
    }
    
    
    void SNKinect3DCam::update(double time, double dt)
    {
        if ( lastBlock != kin->getDepthFrameNr(false) )
        {
            lastBlock = kin->getDepthFrameNr(false);

            kin->uploadDepthImg(false);
            kin->uploadColorImg();
            
            // - pre calculate positions from kinect depth texture ----
            
            posTex->bind();
            posTex->clear();
            
            posTexShdr->begin();
            posTexShdr->setUniform1i("depthTex", 0);
            posTexShdr->setUniform1f("maxDepth", 4500.f); // in mm
            posTexShdr->setUniform1f("thresMin", 800.f); // in mm
            posTexShdr->setUniform1f("thresMax", 600.f); // in mm
            posTexShdr->setUniform1i("texGridSize", texGridSize);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
            
            quad->draw();
            posTex->unbind();
            
            // - pre calculate normals from precalculated positions -
            
            normTex->bind();
            normTex->clear();
            
            normTexShdr->begin();
            normTexShdr->setUniform1i("posTex", 0);
            normTexShdr->setUniform1i("texGridSize", texGridSize);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, posTex->getColorImg());
            
            quad->draw();
            normTex->unbind();
        }
    }
    
    
    void SNKinect3DCam::initPreCalcShdr()
    {
        //- Position Shader ---

        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        std::string vert = STRINGIFY(layout (location=0) in vec4 position;
                                     layout (location=1) in vec3 normal;
                                     layout (location=2) in vec2 texCoord;
                                     layout (location=3) in vec4 color;
                                     out vec4 pos;
                                     out vec2 tex_coord;
                                     void main(void) {
                                         pos = position;
                                         tex_coord = texCoord;
                                         gl_Position = position;
                                     });
        vert = "// Kinect3DCam pos tex vertex shader\n" +shdr_Header +vert;
        
        std::string frag = STRINGIFY(layout(location = 0) out vec4 pos_tex;
                                     in vec4 pos;
                                     in vec2 tex_coord;
                                     uniform sampler2D depthTex;
                                     uniform float maxDepth;
                                     uniform float thresMin;
                                     uniform float thresMax;
                                     float depth;
                                     void main()
                                     {
                                         // max depth = 4,5m
                                         // gemappt auf 0 bis -1.0
                                         depth = max(texture(depthTex, tex_coord).r - thresMin, 0.0) / thresMax;
                                         pos_tex = vec4(pos.x, pos.y, -depth, 1.0);
                                     });
        frag = "// Kinect3DCam pos tex shader\n"+shdr_Header+frag;
        
        posTexShdr = shCol->addCheckShaderText("kinectPlanePosTex", vert.c_str(), frag.c_str());
        
        //- Normal Shader ---

        vert = STRINGIFY(layout (location=0) in vec4 position;
                         layout (location=1) in vec3 normal;
                         layout (location=2) in vec2 texCoord;
                         layout (location=3) in vec4 color;
                         out vec2 toTexPos;
                         void main(void) {
                             toTexPos = vec2(position) * 0.5 + 0.5;
                             gl_Position = position;
                         });
        vert = "// Kinect3DCam norm tex vertex shader\n" +shdr_Header +vert;
        
        frag = STRINGIFY(layout(location = 0) out vec4 norm_tex;
                         in vec2 toTexPos;
                         uniform sampler2D posTex;
                         uniform int texGridSize;

                         vec3 posTop;
                         vec3 posBottom;
                         vec3 posCenter;
                         vec3 posLeft;
                         vec3 posRight;
                         
                         vec3 norms[2];
                         
                         float gridOffs;
                         void main()
                         {
                             gridOffs = 1.0 / float(texGridSize);
                             
                             // read neighbour positions left, right, top, bottom
                             posTop = texture(posTex, vec2(toTexPos.x, toTexPos.y + gridOffs) ).xyz;
                             posBottom = texture(posTex, vec2(toTexPos.x, toTexPos.y - gridOffs)).xyz;
                             posCenter = texture(posTex, toTexPos).xyz;
                             posLeft = texture(posTex, vec2(toTexPos.x - gridOffs, toTexPos.y)).xyz;
                             posRight = texture(posTex, vec2(toTexPos.x + gridOffs, toTexPos.y)).xyz;
                             
                             norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter)));
                             norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));
                             
                             for(int i=0;i<2;i++)
                                 norms[i] = norms[i].z > 0.0 ? norms[i] : norms[i] * -1.0;
                             
                             norms[0] = normalize((norms[0] + norms[1]) * 0.5);
                             norm_tex = vec4(norms[0], 1.0);
                         });
        frag = "// Kinect3DCam norm tex shader\n"+shdr_Header+frag;
        
        normTexShdr = shCol->addCheckShaderText("kinect3DCamNormTex", vert.c_str(), frag.c_str());
        
        
        //- test Shader ---
        
        vert = STRINGIFY(layout (location=0) in vec4 position;
                         layout (location=1) in vec3 normal;
                         layout (location=2) in vec2 texCoord;
                         layout (location=3) in vec4 color;
                         out vec2 toTexPos;
                         void main(void) {
                             toTexPos = texCoord;
                             gl_Position = position;
                         });
        vert = "// Kinect3DCam norm tex vertex shader\n" +shdr_Header +vert;
        
        frag = STRINGIFY(layout(location = 0) out vec4 color;
                         in vec2 toTexPos;
                         uniform sampler2D tex;
                         void main()
                         {
                             color = vec4(texture(tex, toTexPos).r * 0.000222);
                             color.a = 1.0;
                         });
        frag = "// Kinect3DCam norm tex shader\n"+shdr_Header+frag;
        
        
        testDepthShdr = shCol->addCheckShaderText("kinect3DCamTestShdr", vert.c_str(), frag.c_str());
    }
    

    void SNKinect3DCam::initWaveShdr(TFO* _tfo)
    {
        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        std::string vert = STRINGIFY(layout (location=0) in vec4 position;
                                     layout (location=1) in vec3 normal;
                                     layout (location=2) in vec2 texCoord;
                                     layout (location=3) in vec4 color;

                                     uniform int useInstancing;
                                     
                                     out VS_GS_VERTEX {
                                         vec2 texCoord;
                                     } vertex_out;
                                     
                                     void main(void) {
                                         vertex_out.texCoord = texCoord;
                                         gl_Position = position;
                                     });
        vert = "// Kinect3DCam plane shader\n" +shdr_Header +vert;
        
        std::string geom = STRINGIFY(layout (triangles) in;
                                     layout (triangle_strip, max_vertices=3) out;

                                     in VS_GS_VERTEX {
                                         vec2 texCoord;
                                     } vertex_in[];
                                     
                                     uniform int texNr;

                                     uniform sampler2D posTex;
                                     uniform sampler2D normTex;
                                     uniform sampler2D colorTex;
                                     
                                     uniform mat4 modelMatrix;
                                     uniform mat3 normalMatrix;
                                     uniform mat4 projectionMatrix;
                                     
                                     out vec4 rec_position;
                                     out vec3 rec_normal;
                                     out vec4 rec_texCoord;
                                     out vec4 rec_color;
                                     vec4 rawPos;
                                     float draw = 0.0;
                                     void main()
                                     {
                                         for(int i=0; i<gl_in.length(); ++i)
                                         {
                                             rawPos = texture(posTex, vertex_in[i].texCoord);
                                             draw = rec_position.z > -0.3 ? 1.0 : 0.0;
                                             draw = rec_position.x > -0.8 ? draw : 0.0;
                                             draw = rec_position.x < 0.8 ? draw : 0.0;
                                             draw = rec_position.y < 0.9 ? draw : 0.0;
                                             
                                             if (draw > 0.0)
                                             {
//                                                 rec_position = modelMatrix * vec4(gl_in[i].gl_Position.x,
//                                                                                   gl_in[i].gl_Position.y,
//                                                                                   rawPos.r,
//                                                                                   1.0);
                                                 rec_position = modelMatrix * vec4(rawPos.x, -rawPos.y, rawPos.z, 1.0);
                                                 rec_normal = normalize(normalMatrix * texture(normTex, vertex_in[i].texCoord).xyz);
                                                 rec_color = texture(colorTex, vertex_in[i].texCoord);
                                                 rec_texCoord = vec4(vertex_in[i].texCoord, float(texNr), 0.0);
                                             
                                                 gl_Position = gl_in[i].gl_Position;
                                                 EmitVertex();
                                                 if (i==2) EndPrimitive();
                                             }
                                         }
                                     });
        geom = "// Kinect3DCam plane shader\n"+shdr_Header+geom;
        
        std::string frag = STRINGIFY(layout(location = 0) out vec4 color;
                                     in vec4 rec_color;
                                     void main() {
                                         color = rec_color;
                                     });
        frag = "// Kinect3DCam plane shader\n"+shdr_Header+frag;
        
        waveShdr = shCol->addCheckShaderTextNoLink("kinect3DCamFinal", vert.c_str(), geom.c_str(), frag.c_str());


        //- Setup TFO ---
        
        std::vector<std::string> names;
        // copy the standard record attribute names, number depending on hardware
        for (auto i=0;i<MAX_SEP_REC_BUFS;i++)
            names.push_back(stdRecAttribNames[i]);
        _tfo->setVaryingsToRecord(&names, waveShdr->getProgram());
        
        waveShdr->link();
    }
    
    
    SNKinect3DCam::~SNKinect3DCam()
    {
        delete posTex;
        delete normTex;
        delete quadAr;
        delete normTexShdr;
        delete posTexShdr;
        delete waveShdr;
    }
}
