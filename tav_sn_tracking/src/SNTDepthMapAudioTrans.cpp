//
//  SNTDepthMapAudioTrans.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  funktioniert mit dem rohen tiefenbild der kinect (ohne normalisierung) 16bit integer: 0 - 65536
//
//  Hintergrund und Vordergrund werden über einen einfachen Tiefen-Schwellenwert getrennt
//  Die Idee, das Tiefenbild mit einem Referenztiefenbild zu "subtrahiert" geht nicht, weil das Tiefenbild
//  zu sehr rauscht...
//  Der Tiefenschwellenwert wird
//

#include "SNTDepthMapAudioTrans.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
    SNTDepthMapAudioTrans::SNTDepthMapAudioTrans(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), inited(false),
    flWidth(200), flHeight(200), thinDownSampleFact(8),
    fblurSize(512), captureIntv(120.0), inActAddNoiseInt(40.0),  // wenn nix los emit noise
    actDrawMode(DRAW)  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
    {
    	winMan = static_cast<GWindowManager*>(scd->winMan);
		kin = static_cast<KinectInput*>(scd->kin);
    	pa = (PAudio*) scd->pa;
    	osc = static_cast<OSCData*>(scd->osc);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        // get onKey function
        winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
            return this->onKey(key, scancode, action, mods); });                

        // - add OSC Parameter --

        osc->addPar("depthTresh", 10.f, 5000.f, 1.f, 944.f, OSCData::LIN);
        osc->addPar("fastBlurAlpha", 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
        osc->addPar("optFlowBlurAlpha", 0.f, 1.f, 0.0001f, 0.76f, OSCData::LIN);
        osc->addPar("shapeHeight", 0.0f, 8.f, 0.001f, 1.61f, OSCData::LIN);
        osc->addPar("shapeAddFluidHeight", 0.0f, 2.f, 0.001f, 0.1f, OSCData::LIN);
        osc->addPar("shapeHeightAlpha", 0.0f, 1.f, 0.001f, 0.1f, OSCData::LIN);
        osc->addPar("rotScene", 0.0f, 1.f, 0.001f, 0.f, OSCData::LIN);
        osc->addPar("scaleXY", 0.0f, 10.f, 0.001f, 6.f, OSCData::LIN);
        osc->addPar("heightOffs", -10.f, 0.f, 0.001f, -5.8f, OSCData::LIN);
        osc->addPar("normHeightAdj", 0.0f, 1.f, 0.001f, 0.26f, OSCData::LIN);
        osc->addPar("ampScale", 0.0f, 10.f, 0.001f, 0.2f, OSCData::LIN);
        osc->addPar("waveScale", 0.0f, 2.f, 0.001f, 1.0f, OSCData::LIN);


        // --- Fbo zum generieren der normalen karte  ---
        
        normFbo = new FBO(shCol, flWidth, flHeight,
                          GL_RGB16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
        normFbo->clear();
        
        fluidAndShape = new FBO(shCol, flWidth, flHeight,
                                GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
        normFbo->clear();
        
        hmGrid = new QuadArray(256, 256);
        
        // --- Texturen fuer den Litsphere Shader  ---

        litsphereTex = new TextureManager();
        litsphereTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/Unknown-37.jpeg");
        
        bumpMap = new TextureManager();
        bumpMap->loadTexture2D(*scd->dataPath+"/textures/bump_maps/Unknown-2.jpeg");

        // --- Geo Primitives ---
        
        rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on
        
        // --- Shaders ---
        
        colShader = shCol->getStdCol();
        texShader = shCol->getStdTex();
        normShader = shCol->getStdHeightMapSobel();

        initDepthThresShader();
        initAddShapeShader();
        initFluidHeightShader();
    }

    
    
    void SNTDepthMapAudioTrans::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if(inited)
        {
            if (_tfo)
            {
                _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            switch(actDrawMode)
            {
                case RAW_DEPTH :
                    texShader->begin();
                    texShader->setIdentMatrix4fv("m_pvm");
                    texShader->setUniform1i("tex", 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
                    rawQuad->draw();
                    break;
                    
                case DEPTH_THRESH:
                    texShader->begin();
                    texShader->setIdentMatrix4fv("m_pvm");
                    texShader->setUniform1i("tex", 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg(1));
//                    glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());
                    rawQuad->draw();
                    break;
                    
                case DEPTH_BLUR:
                    texShader->begin();
                    texShader->setIdentMatrix4fv("m_pvm");
                    texShader->setUniform1i("tex", 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
                    rawQuad->draw();
                    break;
                    
                case OPT_FLOW:
                    texShader->begin();
                    texShader->setIdentMatrix4fv("m_pvm");
                    texShader->setUniform1i("tex", 0);
                    glActiveTexture(GL_TEXTURE0);
//                    glBindTexture(GL_TEXTURE_2D, histoBlur->getResult());
                    //glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
                    glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
                    rawQuad->draw();
                    break;
                    
                case OPT_FLOW_BLUR:
                    texShader->begin();
                    texShader->setIdentMatrix4fv("m_pvm");
                    texShader->setUniform1i("tex", 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());
                    rawQuad->draw();
                    break;
                    
                case DRAW:
                {
                    glEnable(GL_DEPTH_TEST);

                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    // translate model back to origin
                    glm::mat4 rotMatr = glm::translate(cp->model_matrix_mat4, glm::vec3(0.f, 0.f, osc->getPar("heightOffs")));
                    // rotate
                    rotMatr = glm::rotate(rotMatr, float(M_PI) * 2.f * osc->getPar("rotScene"), glm::vec3(0.f, 1.f, 0.f));
//                   rotMatr = glm::rotate(rotMatr, float(M_PI) * 2.f * osc->getPar("rotScene"), glm::vec3(1.f, 0.f, 0.f));
                    // translate back into visible area
                    rotMatr = glm::translate( rotMatr, glm::vec3(0.f, -0.1f, -osc->getPar("heightOffs")));
                    
                    glm::mat3 normalMat = glm::mat3( glm::transpose( glm::inverse( rotMatr ) ) );
                    rotMatr = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * rotMatr;

                    fluidHeightShdr->begin();
                    fluidHeightShdr->setUniformMatrix4fv("m_pvm", &rotMatr[0][0]);
                    fluidHeightShdr->setUniformMatrix3fv("m_normal", &normalMat[0][0]);
                    fluidHeightShdr->setUniform1f("scaleXY", osc->getPar("scaleXY"));
                    fluidHeightShdr->setUniform1f("heightOffs", osc->getPar("heightOffs"));
                    fluidHeightShdr->setUniform1f("bumpAmt", 0.12f);
                    fluidHeightShdr->setUniform1f("bumpScale", 3.4f);
                    fluidHeightShdr->setUniform1i("rgbHeightMap", 0);
                    fluidHeightShdr->setUniform1f("waveScale", osc->getPar("waveScale"));
					fluidHeightShdr->setUniform1f("ampScale", osc->getPar("ampScale"));
					fluidHeightShdr->setUniform1f("time", time * 0.2);

                    fluidHeightShdr->setUniform1i("normMap", 1);
                    fluidHeightShdr->setUniform1i("litSphereTex", 2);
                    fluidHeightShdr->setUniform1i("bumpMap", 3);
                    fluidHeightShdr->setUniform1i("kinColor", 4);
                    fluidHeightShdr->setUniform1i("audioTex", 5);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
//                    glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
                    
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, normFbo->getColorImg());

                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());
                    
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

                    litsphereTex->bind(2);
                    bumpMap->bind(3);
                    
                    hmGrid->draw();

                    break;
                }
                default:
                    break;
            }

            //if (doCapture) startCaptureFrame(renderMode, time);
        }
    }
    
    
    
    void SNTDepthMapAudioTrans::update(double time, double dt)
    {
        scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
        scd->audioTex->mergeTo2D( pa->waveData.blockCounter);

        if (kin && kin->isReady())
        {
            if (!inited)
            {
                kin->setCloseRange(true);
                kin->setDepthVMirror(true);
                
                // threshold FBO mit 2 attachments, 0: raw Depth Image, 1: normalized Depth Image
                threshFbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
                                    GL_RGBA16F, GL_TEXTURE_2D, false, 2, 1, 1, GL_CLAMP_TO_BORDER, false);
                threshFbo->clear();
                
                histo = new GLSLHistogram(shCol, kin->getDepthWidth(),
                                          kin->getDepthHeight(), GL_RGBA16F,
                                          kin->getDepthWidth() >= 640 ? 1 : 1); // downsampling bei hoher aufloesung

                // - Optical Flow --
                
                optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(),
                                              kin->getDepthHeight());
                optFlow->setMedian(0.8f);
                optFlow->setBright(0.3f);
                
                // - FastBlurs --
                
                fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
                fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
                optFlowBlur = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(),
                                              kin->getDepthHeight(), GL_RGBA16F);
                histoBlur = new FastBlurMem(0.65f, shCol, kin->getDepthWidth(),
                                            kin->getDepthHeight(), GL_RGBA16F);
                
                
                inited = true;
            } else
            {
                kin->uploadColorImg();

                //--- Proc Depth Image and Update the Fluid Simulator --
                
                // upload mit histogram normalisierung fuer heightmap
                kin->uploadDepthImg(true);

                // upload depth image fuer die maske, etc., wird intern gecheckt, ob neues Bild erforderlich oder nicht
                if (kin->uploadDepthImg(false))
                {
                	frameNr = kin->getDepthUplFrameNr(false);
                    
                    // -- threshold the depth image --

                	glDisable(GL_BLEND);

                    threshFbo->bind();
                    threshFbo->clear();

                    depthThres->begin();
                    depthThres->setIdentMatrix4fv("m_pvm");
                    depthThres->setUniform1i("kinDepthTex", 0);
                    depthThres->setUniform1i("kinNormDepthTex", 1);
                    depthThres->setUniform1f("depthThres", osc->getPar("depthTresh"));

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true));

                    rawQuad->draw();
                    threshFbo->unbind();


                    // -- get min and max of the normalized, thresholded depth Image -

                    histo->procMinMax(threshFbo->getColorImg(1));
                   // printf("%f %f \n", histo->getMaximum(), histo->getMinimum());
                    
                    // -- apply blur on silhoutte--

                    fblur->setAlpha(osc->getPar("fastBlurAlpha"));
                    fblur->proc(threshFbo->getColorImg());

                    fblur2nd->setAlpha(osc->getPar("fastBlurAlpha"));
                    fblur2nd->proc(fblur->getResult());

                }

                // --- add the silhoutte to the fluid 
                
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                
                fluidAndShape->bind();
                fluidAndShape->clearAlpha(1.f - osc->getPar("shapeHeightAlpha"));
                
                addShapeShdr->begin();
//                addShapeShdr->setUniform1i("fluid", 0);
                addShapeShdr->setUniform1i("depthMap", 1);
                addShapeShdr->setUniform1f("minVal", histo->getMinimum());
                addShapeShdr->setUniform1f("shapeHeightScale", osc->getPar("shapeHeight"));
                addShapeShdr->setUniform1f("shapeFluidcale", osc->getPar("shapeAddFluidHeight"));
                addShapeShdr->setUniform1f("alpha", osc->getPar("shapeHeightAlpha"));

//                glActiveTexture(GL_TEXTURE0);
//                glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
//
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg(1));
                
                rawQuad->draw();
                fluidAndShape->unbind();
            
                
                // generate normals
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                normFbo->bind();
                normShader->begin();
                normShader->setUniform1i("heightMap", 0);
                normShader->setUniform2f("texGridStep", 1.f / float(flWidth), 1.f / float(flHeight));
                normShader->setUniform1f("heightFact", osc->getPar("normHeightAdj"));
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
                //glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
                
                rawQuad->draw();
                normFbo->unbind();
            }
        }
    }
    
    
    
    void SNTDepthMapAudioTrans::startCaptureFrame(int renderMode, double time)
    {
        lastCaptureTime = time;
        
        // start write thread
        if(capture_Thread)
        {
            delete capture_Thread;
            capture_Thread = 0;
        }
        
        capture_Thread = new boost::thread(&SNTDepthMapAudioTrans::captureFrame, this, renderMode, time);
    }
    
    
    
    void SNTDepthMapAudioTrans::captureFrame(int renderMode, double time)
    {
        // start SaveThread
        char fileName [100];
        sprintf(fileName,
                ((*scd->dataPath)+"capture/lenovo%d%d%f.jpg").c_str(),
                static_cast<int>( ( static_cast<double>(savedNr) / 10.0 ) ),
                savedNr % 10,
                time);
        
//        cv::flip(capturePic1Cam, fCapturePic1Cam, 0);
//        cv::imwrite(fileName, fCapturePic1Cam);
        
        // start scp thread
        if (!inactivityEmit)
        {
            int i = system(("cp "+std::string(fileName)+" /media/ingeneria1-pc").c_str());
            if (i != 0) printf ("Image Capture Error!!!! scp upload failed\n");
            
            printf("image written \n");
        }
    }

    

    void SNTDepthMapAudioTrans::initDepthThresShader()
    {
        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                     layout( location = 1 ) in vec4 normal;
                                     layout( location = 2 ) in vec2 texCoord;
                                     layout( location = 3 ) in vec4 color;
                                     uniform mat4 m_pvm;
                                     out vec2 tex_coord;
                                     void main()
                                     {
                                         tex_coord = texCoord;
                                         gl_Position = m_pvm * position;
                                    });
        
        stdVert = "// SNTDepthMapAudioTrans depth threshold vertex shader\n" +shdr_Header +stdVert;
        std::string frag = STRINGIFY(uniform sampler2D kinDepthTex; // 16 bit -> float
                                     uniform sampler2D kinNormDepthTex; // 0 - 1 -> float
                                     uniform float depthThres;
                                     in vec2 tex_coord;
                                     layout (location=0) out vec4 rawThres;
                                     layout (location=1) out vec4 normThres;
                                     float outVal;
                                     void main()
                                     {
                                         outVal = texture(kinDepthTex, tex_coord).r;
                                         outVal = outVal > 50.0 ? (outVal < depthThres ? 1.0 : 0.0) : 0.0;
                                         rawThres = vec4(outVal);
                                         normThres = vec4(outVal * texture(kinNormDepthTex, tex_coord).r);
                                         normThres.a = 1.0;
                                     });
        
        frag = "// SNTDepthMapAudioTrans depth threshold fragment shader\n"+shdr_Header+frag;
        
        depthThres = shCol->addCheckShaderText("SNTDepthMapAudioTrans_thres", stdVert.c_str(), frag.c_str());
    }

    
    
    void SNTDepthMapAudioTrans::initFluidHeightShader()
    {
        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                        layout( location = 1 ) in vec4 normal;
                                        layout( location = 2 ) in vec2 texCoord;
                                        layout( location = 3 ) in vec4 color;
                                        
                                        uniform sampler2D rgbHeightMap;
                                        uniform sampler2D audioTex;

                                        uniform mat4 m_pvm;
                                        uniform float scaleXY;
                                        uniform float heightOffs;
                                        uniform float ampScale;
                                        uniform float waveScale;
                                        uniform float time;

                                        vec4 rgbHmPix;
                                        float height;
                                        float amp;

                                        out vec2 tex_coord;
                                        out float out_height;
                                        
                                        void main()
                                        {
                                        	// take only first channel
                                        	amp = texture(audioTex, vec2(texCoord.y * waveScale + time, 0)).r * ampScale + 1.0;

                                            rgbHmPix = texture(rgbHeightMap, texCoord);
                                            out_height = (rgbHmPix.r + rgbHmPix.g + rgbHmPix.b);
                                            height = out_height + heightOffs;
                                            tex_coord = texCoord;

                                            vec2 scalePos = position.xy * scaleXY;
                                            gl_Position = m_pvm * vec4(scalePos.x * amp, scalePos.y, height, 1.0);
                                        });
        
        stdVert = "// SNTDepthMapAudioTransfluid Heightmap vertex shader\n" +shdr_Header +stdVert;
        
        std::string frag = STRINGIFY(in vec2 tex_coord;
        							 in float out_height;

                                     uniform sampler2D normMap;
                                     uniform sampler2D litSphereTex;
                                     uniform sampler2D bumpMap;
                                     uniform sampler2D rgbHeightMap;
                                     uniform sampler2D kinColor;
                                     uniform mat3 m_normal;
                                     uniform float bumpAmt;
                                     uniform float bumpScale;
                                     uniform float colMix;
                                     vec3 texNorm;
                                     vec3 bumpNorm;
                                     vec4 orgCol;
                                     vec4 litColor;
                                     vec4 kinectColor;
                                     float outVal;
                                     layout (location = 0) out vec4 color;
                                     
                                     void main()
                                     {
                                    	 kinectColor = texture(kinColor, vec2(tex_coord.x, 1.0 - tex_coord.y));
                                         orgCol = texture(rgbHeightMap, tex_coord) * 1.5;
                                         texNorm = m_normal * texture(normMap, tex_coord).xyz;
                                         bumpNorm = m_normal * texture(bumpMap, tex_coord * bumpScale).xyz;
                                         litColor = texture(litSphereTex, mix(texNorm.xy, bumpNorm.xy, bumpAmt) * 0.5 + vec2(0.5));
                                         color = kinectColor * float(out_height > 0.1);
                                         //color = mix(orgCol * (litColor.r + litColor.g + litColor.b), litColor, colMix);
                                     });
        
        frag = "// SNTDepthMapAudioTrans fluid Heightmap fragment shader\n"+shdr_Header+frag;
        
        fluidHeightShdr = shCol->addCheckShaderText("SNTDepthMapAudioTrans_hm", stdVert.c_str(), frag.c_str());
    }
    
    

    void SNTDepthMapAudioTrans::initAddShapeShader()
    {
        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        
        std::string vert = STRINGIFY(layout (location=0) in vec4 position;
                                     layout (location=1) in vec3 normal;
                                     layout (location=2) in vec2 texCoord;
                                     layout (location=3) in vec4 color;
                                     out vec2 tex_coord;
                                     void main(void) {
                                         tex_coord = texCoord;
                                         gl_Position = position;
                                     });
        vert = "//SNTDepthMapAudioTrans add shape Vert\n" +shdr_Header +vert;
        
        std::string frag = STRINGIFY(layout(location = 0) out vec4 color;
                                     in vec2 tex_coord;
                                     uniform sampler2D depthMap;
                                     uniform float minVal;
                                     uniform float alpha;
                                     uniform float shapeHeightScale;
                                     uniform float shapeFluidcale;
                                     uniform vec2 kinFov;

                                     float height;
                                     vec4 depthMapCol;
                                     
                                     void main()
                                     {
                                         depthMapCol = texture(depthMap, tex_coord);
                                         height = (depthMapCol.r + depthMapCol.g + depthMapCol.b);
                                         height = max(height - minVal, 0.0) * shapeHeightScale;
                                         if (height <= 0.0) discard;
                                         
                                         color = vec4(height, height, height, alpha);
                                     });
        frag = "// SNTDepthMapAudioTrans add shape frag\n"+shdr_Header+frag;
        
        addShapeShdr = shCol->addCheckShaderText("SNTDepthMapAudioTrans_add_shape", vert.c_str(), frag.c_str());
    }
    
    

    void SNTDepthMapAudioTrans::onKey(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_1 : actDrawMode = RAW_DEPTH;
                    printf("actDrawMode = RAW_DEPTH \n");
                    break;
                case GLFW_KEY_2 : actDrawMode = DEPTH_THRESH;
                    printf("actDrawMode = DEPTH_THRESH \n");
                    break;
                case GLFW_KEY_3 : actDrawMode = DEPTH_BLUR;
                    printf("actDrawMode = DEPTH_BLUR \n");
                    break;
                case GLFW_KEY_4 : actDrawMode = OPT_FLOW;
                    printf("actDrawMode = OPT_FLOW \n");
                    break;
                case GLFW_KEY_5 : actDrawMode = OPT_FLOW_BLUR;
                    printf("actDrawMode = OPT_FLOW_BLUR \n");
                    break;
                case GLFW_KEY_6 : actDrawMode = FLUID_VEL;
                    printf("actDrawMode = FLUID_VEL \n");
                    break;
                case GLFW_KEY_7 : actDrawMode = DRAW;
                    printf("actDrawMode = DRAW \n");
                    break;
            }
        }
    }
    
    
    
    void SNTDepthMapAudioTrans::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        mouseX = xpos / scd->screenWidth;
        mouseY = (ypos / scd->screenHeight);
    }
    
    
    
    SNTDepthMapAudioTrans::~SNTDepthMapAudioTrans()
    {
        delete userMapConv;
        delete userMapRGBA;
        delete fluidAddCol;
        delete partEmitCol;
        delete rawQuad;
        delete optFlow;
    }
}
