//
//  SNTSilhouetteRGB.cpp
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

#include "SNTSilhouetteRGB.h"

using namespace std;

namespace tav
{
    SNTSilhouetteRGB::SNTSilhouetteRGB(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs),  inited(false), flWidth(200), flHeight(200), thinDownSampleFact(8),
    fblurSize(512), captureIntv(120.0), inActAddNoiseInt(40.0),  // wenn nix los emit noise
    actDrawMode(DRAW)  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	winMan = static_cast<GWindowManager*>(scd->winMan);
		osc = static_cast<OSCData*>(scd->osc);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        // get onKey function
        winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
            return this->onKey(key, scancode, action, mods); });                

        // - add OSC Parameter --

        addPar("depthTresh", &depthTresh);
        addPar("fastBlurAlpha", &fastBlurAlpha);
        addPar("shapeHeight", &shapeHeight);
        addPar("shapeHeightAlpha", &shapeHeightAlpha);
        
        // - Fluid System --
        
        fluidAndShape = new FBO(shCol, flWidth, flHeight,
                                GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
        hmGrid = new QuadArray(256, 256);

        // --- Geo Primitives ---
        
        rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on
        
        // --- Shaders ---
        
        colShader = shCol->getStdCol();
        texShader = shCol->getStdTex();

        initDepthThresShader();
        initRGBOffsetShader();
    }
    
    
    
    void SNTSilhouetteRGB::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
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
                    
                case DRAW:
                {

                	glClearColor(1.0, 1.0, 1.0, 1.0);
                	glClear(GL_COLOR_BUFFER_BIT);

              //  	glBlendEquation(GL_FUNC_SUBTRACT);

                    glDisable(GL_DEPTH_TEST);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    
                    rgbOffsetShdr->begin();
                    rgbOffsetShdr->setIdentMatrix4fv("m_pvm");
                    rgbOffsetShdr->setUniform1i("tex", 0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
                    
                    rawQuad->draw();

                //	glBlendEquation(GL_FUNC_ADD);

                    break;
                }
                default:
                    break;
            }

            //if (doCapture) startCaptureFrame(renderMode, time);
        }
    }
    
    
    void SNTSilhouetteRGB::update(double time, double dt)
    {
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

                // - FastBlurs --
                
                fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
                fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
                histoBlur = new FastBlurMem(0.65f, shCol, kin->getDepthWidth(),
                                            kin->getDepthHeight(), GL_RGBA16F);
                
                
                inited = true;
            } else
            {
                // wenn nix los, emit particles
                //if(inActEmitTime > inActAddNoiseInt) inactivityEmit = true; else inactivityEmit = false;

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
                    depthThres->setUniform1f("depthThres", depthTresh);

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

                    fblur->setAlpha(fastBlurAlpha);
                    fblur->proc(threshFbo->getColorImg());

                    fblur2nd->setAlpha(fastBlurAlpha);
                    fblur2nd->proc(fblur->getResult());
                }


            }
        }
    }
    
    
    void SNTSilhouetteRGB::startCaptureFrame(int renderMode, double time)
    {
        lastCaptureTime = time;
        
        // start write thread
        if(capture_Thread)
        {
            delete capture_Thread;
            capture_Thread = 0;
        }
        
        capture_Thread = new boost::thread(&SNTSilhouetteRGB::captureFrame, this, renderMode, time);
    }
    
    
    void SNTSilhouetteRGB::captureFrame(int renderMode, double time)
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

    
    void SNTSilhouetteRGB::initDepthThresShader()
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
        
        stdVert = "// SNTSilhouetteRGB depth threshold vertex shader\n" +shdr_Header +stdVert;
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
        
        frag = "// SNTSilhouetteRGB depth threshold fragment shader\n"+shdr_Header+frag;
        
        depthThres = shCol->addCheckShaderText("SNTSilhouetteRGB_thres", stdVert.c_str(), frag.c_str());
    }

    
    void SNTSilhouetteRGB::initRGBOffsetShader()
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
        
        stdVert = "// SNTSilhouetteRGBfluid rgb offset vertex shader\n" +shdr_Header +stdVert;
        
        std::string frag = STRINGIFY(in vec2 tex_coord;
                                     uniform sampler2D tex;
                                     layout (location = 0) out vec4 color;
                                     
                                     void main()
                                     {
                                         vec4 red = texture(tex, vec2(tex_coord.x + 0.3, tex_coord.y));
                                         vec4 green = texture(tex, vec2(tex_coord.x + 0.2, tex_coord.y + 0.1));
                                         vec4 blue = texture(tex, vec2(tex_coord.x + 0.1, tex_coord.y));
                                         float bright = min(red.r + green.r + blue.r, 1.0);

                                         color = vec4(
                                        		 max( vec3(0.3), vec3(1.0) - vec3(
                                        					 red.r * vec3(0.16, 0.94, 0.09) * 0.8 // lila
                                        					 + green.r * vec3(0.0, 0.0, 1.0) * 0.8 // gelb
                                         	 	 	 	 	 + blue.r * vec3(1.0, 0.0, 0.0) * 0.8)
												 ),
												 bright
                                         );
                                     });
        
        frag = "// SNTSilhouetteRGB rgb offset fragment shader\n"+shdr_Header+frag;
        
        rgbOffsetShdr = shCol->addCheckShaderText("SNTSilhouetteRGB_offset", stdVert.c_str(), frag.c_str());
    }
    

    void SNTSilhouetteRGB::onKey(int key, int scancode, int action, int mods)
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
                case GLFW_KEY_4 : actDrawMode = DRAW;
                    printf("actDrawMode = DRAW \n");
                    break;
            }
        }
    }
    
    
    SNTSilhouetteRGB::~SNTSilhouetteRGB()
    {
        delete userMapConv;
        delete userMapRGBA;
        delete rawQuad;
    }
}
