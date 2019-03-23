//
//  SNTDepthMapSelfie.cpp
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

#include "SNTDepthMapSelfie.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
    SNTDepthMapSelfie::SNTDepthMapSelfie(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs), inited(false), flWidth(200), flHeight(200), thinDownSampleFact(8),
    fblurSize(512), captureIntv(120.0), inActAddNoiseInt(40.0),  // wenn nix los emit noise
    actDrawMode(DRAW)  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
    {
    	kin = static_cast<KinectInput*>(scd->kin);
    	osc = static_cast<OSCData*>(scd->osc);
    	winMan = static_cast<GWindowManager*>(scd->winMan);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

        // get onKey function
        winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
            return this->onKey(key, scancode, action, mods); });                

        // - add OSC Parameter --

        osc->addPar("depthTresh", 10.f, 5000.f, 1.f, 1214.f, OSCData::LIN);
//        _osc->addPar("fastBlurAlpha", 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
//        _osc->addPar("optFlowBlurAlpha", 0.f, 1.f, 0.0001f, 0.76f, OSCData::LIN);
        osc->addPar("modelPos", 0.0f, 5000.f, 0.001f, 900.f, OSCData::LIN);

        // --- Fbo zum generieren der normalen karte  ---
        
        fluidAndShape = new FBO(shCol, flWidth, flHeight,
                                GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
        
        // --- Texturen fuer den Litsphere Shader  ---

        backTex = new TextureManager();
        backTex->loadTexture2D(*scd->dataPath+"/textures/bravo_back.jpg");

        bravoTex = new TextureManager();
        bravoTex->loadTexture2D(*scd->dataPath+"/textures/bravo-front.png");

        // --- Geo Primitives ---

        rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on
        
        // --- Shaders ---
        
        colShader = shCol->getStdCol();
        texShader = shCol->getStdTex();

        initCutBackShader();
        initDepthThresShader();
//        initAddShapeShader();
//        initFluidHeightShader();
    }
    
    
    
    void SNTDepthMapSelfie::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
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
                    glDisable(GL_DEPTH_TEST);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    // draw background
                    texShader->begin();
                    texShader->setIdentMatrix4fv("m_pvm");
                    texShader->setUniform1i("tex", 0);

                    backTex->bind(0);
                    rawQuad->draw();


                    // translate model back to origin
                    glm::mat4 rotMatr = glm::mat4(1.f);
                    rotMatr = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * rotMatr;

                    kinCutBackShdr->begin();
                    kinCutBackShdr->setUniformMatrix4fv("m_pvm", &rotMatr[0][0]);
                    kinCutBackShdr->setUniform1i("kinColor", 0);
                    kinCutBackShdr->setUniform1i("kinDepthThresh", 1);
                    kinCutBackShdr->setUniform1i("kinDepth", 2);
                    kinCutBackShdr->setUniform1i("modelTex", 3);
                    kinCutBackShdr->setUniform1f("modelPos", osc->getPar("modelPos"));

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

                    glActiveTexture(GL_TEXTURE3);
                    bravoTex->bind(3);

                    rawQuad->draw();


                    // draw foreground
//                    texShader->begin();
//                    texShader->setIdentMatrix4fv("m_pvm");
//                    texShader->setUniform1i("tex", 0);
//
//                    bravoTex->bind(0);
//                    rawQuad->draw();

                    break;
                }
                default:
                    break;
            }

            //if (doCapture) startCaptureFrame(renderMode, time);
        }
    }
    
    
    
    void SNTDepthMapSelfie::update(double time, double dt)
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
                /*
                histo = new GLSLHistogram(shCol, kin->getDepthWidth(),
                                          kin->getDepthHeight(), GL_RGBA16F,
                                          kin->getDepthWidth() >= 640 ? 1 : 1); // downsampling bei hoher aufloesung

                // - FastBlurs --
                
                fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
                fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
                histoBlur = new FastBlurMem(0.65f, shCol, kin->getDepthWidth(),
                                            kin->getDepthHeight(), GL_RGBA16F);
                
                */
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

                    /*
                    // -- get min and max of the normalized, thresholded depth Image -

                    histo->procMinMax(threshFbo->getColorImg(1));
                   // printf("%f %f \n", histo->getMaximum(), histo->getMinimum());
                    
                    // -- apply blur on silhoutte--

                    fblur->setAlpha(osc->getPar("fastBlurAlpha"));
                    fblur->proc(threshFbo->getColorImg());

                    fblur2nd->setAlpha(osc->getPar("fastBlurAlpha"));
                    fblur2nd->proc(fblur->getResult());
                */
                }
            }
        }
    }
    
    
    
    void SNTDepthMapSelfie::startCaptureFrame(int renderMode, double time)
    {
        lastCaptureTime = time;
        
        // start write thread
        if(capture_Thread)
        {
            delete capture_Thread;
            capture_Thread = 0;
        }
        
        capture_Thread = new boost::thread(&SNTDepthMapSelfie::captureFrame, this, renderMode, time);
    }
    
    
    
    void SNTDepthMapSelfie::captureFrame(int renderMode, double time)
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

    

    void SNTDepthMapSelfie::initDepthThresShader()
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
        
        stdVert = "// SNTDepthMapSelfie depth threshold vertex shader\n" +shdr_Header +stdVert;
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
        
        frag = "// SNTDepthMapSelfie depth threshold fragment shader\n"+shdr_Header+frag;
        
        depthThres = shCol->addCheckShaderText("SNTDepthMapSelfie_thres", stdVert.c_str(), frag.c_str());
    }

    
    
    void SNTDepthMapSelfie::initCutBackShader()
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
        
        stdVert = "// SNTDepthMapSelfie vertex shader\n" +shdr_Header +stdVert;
        
        std::string frag = STRINGIFY(in vec2 tex_coord;

                                     uniform sampler2D kinColor;
                                     uniform sampler2D kinDepthThresh;
                                     uniform sampler2D kinDepth;
                                     uniform sampler2D modelTex;

                                     uniform float modelPos;

                                     vec2 scaleTexCoord;
                                     float depthAtPix;
                                     float cutCol;
                                     vec4 modelCol;
                                     vec4 visitorCol;

                                     layout (location = 0) out vec4 color;
                                     
                                     void main()
                                     {
                                    	 scaleTexCoord = vec2(tex_coord.x * (5.0 / 4.0), tex_coord.y + 0.2) * 0.8;

                                    	 cutCol = texture(kinDepthThresh, scaleTexCoord).r;
                                    	 depthAtPix = texture(kinDepth, scaleTexCoord).r; // in mm
                                    	 modelCol = texture(modelTex, tex_coord); // in mm
                                    	 visitorCol = texture(kinColor, vec2(scaleTexCoord.x, 1.0 - scaleTexCoord.y)) * cutCol;

                                    	 color = (modelCol.a > 0.7) ? modelCol : visitorCol;
                                    	// color = (depthAtPix < modelPos && depthAtPix > 200 && depthAtPix < 2000) ? visitorCol : color;
                                     });
        
        frag = "// SNTDepthMapSelfie fragment shader\n"+shdr_Header+frag;
        
        kinCutBackShdr = shCol->addCheckShaderText("SNTDepthMapSelfie", stdVert.c_str(), frag.c_str());
    }
    
    

    void SNTDepthMapSelfie::initAddShapeShader()
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
        vert = "//SNTDepthMapSelfie add shape Vert\n" +shdr_Header +vert;
        
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
        frag = "// SNTDepthMapSelfie add shape frag\n"+shdr_Header+frag;
        
        addShapeShdr = shCol->addCheckShaderText("SNTDepthMapSelfie_add_shape", vert.c_str(), frag.c_str());
    }
    

    
    void SNTDepthMapSelfie::onKey(int key, int scancode, int action, int mods)
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
                case GLFW_KEY_7 : actDrawMode = DRAW;
                    printf("actDrawMode = DRAW \n");
                    break;
            }
        }
    }
    
    
    
    void SNTDepthMapSelfie::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        mouseX = xpos / scd->screenWidth;
        mouseY = (ypos / scd->screenHeight);
    }
    
    
    
    SNTDepthMapSelfie::~SNTDepthMapSelfie()
    {
        delete userMapConv;
        delete userMapRGBA;
        delete fluidAddCol;
        delete partEmitCol;
        delete rawQuad;
    }
}
