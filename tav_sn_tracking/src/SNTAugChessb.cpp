//
// SNTAugChessb.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  mirroring has to be off!
//  kinect v1 gets more and more unprecise with increasing distance
//  the distance is rastered, so the error varyies
//

#include "SNTAugChessb.h"

#define STRINGIFY(A) #A

using namespace std;
using namespace cv;

namespace tav
{
    SNTAugChessb::SNTAugChessb(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs),
    actDrawMode(CHESS_TO_CAM),
    kinFovY(47.8f),                     // gemessen  47,8 Handbuch sagt 45 Grad, onistream sagt 45,642
    kinFovX(61.66f),                    // gemessen 62.2, vielleicht von Kinect zu Kinect unterschiedlich
                                        // Handbuch sagt 58 Grad, onistream sagt 58,59
    getNrCheckSamples(50),              // anzahl von samples bis die werte als stabil angesehen werden
    invertCamBeamerMatr(true),
    beamerToObjIntv(5),
    realBoardWidth(24.3f),              // reale Breite des Schachbretts (alle kästchen)
    coordNormFact(0.1f),                 // zur besseren Handhabung der Kinect Realworld Koordinaten
    nrMeasureSamples(7)
    {
    	osc = static_cast<OSCData*>(scd->osc);
    	kin = static_cast<KinectInput*>(scd->kin);
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
        

    	aModelNode = this->addChild();
    	aModelNode->setName(name);
    	aModelNode->setActive(true);

    	aImport = new AssimpImport(scd, true);
    	aImport->load(((*scd->dataPath)+"models/realidad_aumentada.obj").c_str(), aModelNode, [this](){
    		printf("SNGam_Libro, libro loaded! \n");
    	});

    	//aImport->loadModel((*scd->dataPath)+"models/realidad_aumentada.obj");
//    	aImport->loadModel((*scd->dataPath)+"models/trust_logo_extrude2.obj");
    	//        aImport->loadModel((*scd->dataPath)+"models/Vw_bus_pancho/MeshCorrectionTextures.obj");
    	//        aImport->loadModel((*scd->dataPath)+"models/Vw_bus_pancho/CombiTestSven.obj");



        kpCalib.camFovX = (kinFovX / 360.f) * 2.f * M_PI;
        kpCalib.camFovY = (kinFovY * 1.03f / 360.f) * 2.f * M_PI;

// benq mw820std
//        kpCalib.beamModel = ULTRA_WIDE;
//        kpCalib.beamerThrowRatio = 0.605f;           // benq mw820std
//        kpCalib.beamerFovX = 1.384f;                  // benq mw820std
//        kpCalib.beamerFovY = 1.11f;                   // benq mw820std
//        beamerLowEdgeAngle(2.94f),           // wenn die linse des Beamers nicht zentriert ist, dieser wert bezieht sich auf 4:3
                                                // bei 16:10 anderer wert, alter wert 1,68
//        kpCalib.beamerLowEdgeAngle = (beamerLowEdgeAngle / 360.f) * 2.f * M_PI;
//        kpCalib.beamerThrowRatio = beamerThrowRatio;
//        kpCalib.beamerLookAngle = -33.46f;
//        kpCalib.camBeamerRealOffs = glm::vec3(0.f, 91.f, 142.f);

// asus bm1
        kpCalib.beamModel = ULTRA_WIDE;
        kpCalib.beamerThrowRatio = 1.888f;           // benq mw820std
        kpCalib.beamerFovX = 0.5159f;
        kpCalib.beamerFovY = 0.3931f;
        //kpCalib.beamerLowEdgeAngle = -0.02552f; // schaut nach unten
        kpCalib.beamerLowEdgeAngle = 0.f; // schaut nach unten
        kpCalib.beamerLookAngle = -11.3f;
        kpCalib.camBeamerRealOffs = glm::vec3(0.f, 67.f, 10.f);

        
        kpCalib.boardSize = cv::Size(8,4);
        kpCalib.virtBoardSize = cv::Size(16,8);
        //        kpCalib.beamerFovX = 2.f * std::atan2(0.5f, beamerThrowRatio);
        //        kpCalib.beamerFovY = kpCalib.beamerFovX / kpCalib.beamerAspectRatio;
        
        // gemessen
        
//        kpCalib.beamerAspectRatio = kpCalib.beamerFovX / kpCalib.beamerFovY;
        kpCalib.beamerAspectRatio = static_cast<float>(scd->screenWidth) / static_cast<float>(scd->screenHeight);

        kpCalib.invertMatr = invertCamBeamerMatr;
        kpCalib.nrSamples = getNrCheckSamples;
        kpCalib.rotations = glm::vec3(0.f);
        kpCalib.camBeamerRots = glm::vec3(0.f);
        
        contrPoints = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
        GLfloat* pos = new GLfloat[kpCalib.boardSize.width * kpCalib.boardSize.height * 3];
        for(int y=0;y<kpCalib.boardSize.height;y++)
            for(int x=0;x<kpCalib.boardSize.width;x++)
            {
                pos[(y * kpCalib.boardSize.width + x) *3] = static_cast<float>(x) / static_cast<float>(kpCalib.boardSize.width-1) - 0.5f;
                pos[(y * kpCalib.boardSize.width + x) *3 +1] = static_cast<float>(y) / static_cast<float>(kpCalib.boardSize.height-1) - 0.5f;
                pos[(y * kpCalib.boardSize.width + x) *3 +2] = 0.f;
            }
        
        contrPoints->upload(POSITION, pos, kpCalib.boardSize.width * kpCalib.boardSize.height * 3);
        contrPoints->setStaticColor(1.f, 0.f, 0.f, 1.f);
        
        testChessFile = cv::imread((*scd->dataPath)+"textures/chess_640.jpg");
        
        camBeamerMatrPath = (*scd->dataPath)+("calib_cam/xtion_to_benq.yml");
        

        quad = new Quad(-1.f, -1.f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        0.f, 0.f, 0.f, 1.f,
                        nullptr, 1, true);
        
        noFlipQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                              glm::vec3(0.f, 0.f, 1.f),
                              0.f, 0.f, 0.f, 1.f);
        
        kinQuad = new Quad(-1.0f, .75f, 0.25f, 0.25f,
                           glm::vec3(0.f, 0.f, 1.f),
                           0.f, 0.f, 0.f, 1.f,
                           nullptr, 1, true);
        
        whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                             glm::vec3(0.f, 0.f, 1.f),
                             1.f, 1.f, 1.f, 1.f);
        
        xLine = new Line(2, 1.f, 0.f, 0.f, 1.f);
        GLfloat* xLinePoints = (GLfloat*) xLine->getMapBuffer(POSITION);
        xLinePoints[0] = -1.f; xLinePoints[1] = 0.f; xLinePoints[2] = 0.f;
        xLinePoints[3] = 1.f; xLinePoints[4] = 0.f; xLinePoints[5] = 0.f;
        xLine->unMapBuffer();
        
        yLine = new Line(2, 1.f, 0.f, 0.f, 1.f);
        GLfloat* yLinePoints = (GLfloat*) yLine->getMapBuffer(POSITION);
        yLinePoints[0] = 0.f; yLinePoints[1] = 1.f; yLinePoints[2] = 0.f;
        yLinePoints[3] = 0.f; yLinePoints[4] = -1.f; yLinePoints[5] = 0.f;
        yLine->unMapBuffer();
        
        kinTex = new TextureManager();
        kinDepthTex = new TextureManager();
        
        // chess muss die proportionen des realen chesss haben
        // proportionen sind in der grafik schon richtig, deshalb keine
        // aspect anpassung
        glChessBoardWidth = 1.3f;
        chessBoard = new PropoImage((*scd->dataPath)+"textures/chessboard.jpg",
                                    scd->screenWidth, scd->screenHeight, glChessBoardWidth);
        glChessBoardHeight = chessBoard->getImgHeight();

        chessBoard_big = new PropoImage((*scd->dataPath)+"textures/chessboard_big.jpg",
                                    scd->screenWidth, scd->screenHeight, glChessBoardWidth);
        
        // 512 x 512 textur, nicht quadratische texture, skalieren nicht richtig...
        // proportion texture ist 1.8
        chessBoard2 = new PropoImage((*scd->dataPath)+"textures/chessboard_512.jpg",
                                     (int)((float)scd->screenWidth / 1.8f),
                                     scd->screenWidth, 4.f);

        // 512 x 512 textur, nicht quadratische texture, skalieren nicht richtig...
        // proportion texture ist 1.8
        chessBoard3 = new PropoImage((*scd->dataPath)+"textures/chessboard_512.jpg",
                                     (int)((float)scd->screenWidth / 1.8f),
                                     scd->screenWidth, realBoardWidth);
        
        chessQA = new QuadArray(20, 20, 0.f, 0.f,
                                realBoardWidth,
                                chessBoard3->getImgHeight(),
                                glm::vec3(0.f, 0.f, 1.f));
        
        cv::FileStorage fs((*scd->dataPath)+"calib_cam/xtion_color_640.yml", cv::FileStorage::READ);
        if ( fs.isOpened() )
        {
            fs["camera_matrix"] >> kpCalib.cameraMatrix;
            fs["distortion_coefficients"] >> kpCalib.distCoeffs;
            fs["image_width"] >> kpCalib.imgSize.width;
            fs["image_height"] >> kpCalib.imgSize.height;
        } else
        {
            printf("couldn´t open file\n");
        }
        
        chessRotMatr = glm::mat4(1.f);
        kalmanRot = new CvKalmanFilter(6, 3, 0.01f);
        kalmanRotZ = new CvKalmanFilter(6, 3, 0.01f);
        kalmanTrans = new CvKalmanFilter(6, 3, 0.01f);
        
        cbNormal = new Median<glm::vec3>(7.f);
        cbMidPoint = new Median<glm::vec3>(7.f);
        cbLeftVec = new Median<glm::vec4>(2.5f);
        kinCamOffset = new Median<glm::vec3>(7.f);
        chessRealWidth = new Median<float>(7.f);
        beamerWidth = new Median<float>(7.f);
        beamerHeight = new Median<float>(7.f);
        beamerWallDist = new Median<float>(7.f);
        rotX = new Median<double>(7.f);

        lastChessWidth = new Median<float>(1.f, nrMeasureSamples);
        lastBeamerDist = new Median<float>(1.f, nrMeasureSamples);
        newChessWidth = new Median<float>(1.f, nrMeasureSamples);
        newBeamerDist = new Median<float>(1.f, nrMeasureSamples);

        rotations = new Median<glm::vec3>(5.f, getNrCheckSamples);

        if(actDrawMode == CHECK_BEAMER_TO_OBJ || actDrawMode == CHECK_KIN_UNWARP)
        {
            cout << "load beamer to cam matrix" << endl;
            fs = cv::FileStorage(camBeamerMatrPath, cv::FileStorage::READ);
            if ( fs.isOpened() ) loadCamToBeamerCalib(fs, kpCalib);
        }
        
        if(actDrawMode == CHESS_TO_CAM || actDrawMode == CHECK_CAM_TO_BEAMER)
            kpCalib.invertMatr = false;
        
        colShader = shCol->getStdCol();
        shdr = shCol->getStdTex();
        blendShader = shCol->getStdTexAlpha();
        initLitShader();

//        blendShader = shCol->addCheckShader("texBlendShdr", "shaders/mapping2.vert",
//                                                             "shaders/mapping2.frag");

        litsphereTex = new TextureManager();
        //litsphereTex->loadTexture2D((*scd->dataPath)+"textures/litspheres/Unknown-30.jpeg");
        litsphereTex->loadTexture2D((*scd->dataPath)+"textures/litspheres/ikin_logo_lit.jpeg");

        cubeTex = new TextureManager();
        cubeTex->loadTextureCube((*scd->dataPath)+"textures/trust/trust_cube.png");

        godRays = new GodRays(shCol, _scd->screenWidth/2, _scd->screenHeight/2);
        lightCol = glm::vec4(1.f);

    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::initKinDependent()
    {
        kin->setImageRegistration(true);
        //kin->setMirror(false); // geht nicht bei depth stream, warum auch immer...
        // vermutlich problem mit verschiedenen auflösungen per stream
        
        kpCalib.imgSize.width = static_cast<float>(kin->getColorWidth());
        kpCalib.imgSize.height = static_cast<float>(kin->getColorHeight());
        
        kpCalib.depthImgSize.width = static_cast<float>(kin->getDepthWidth());
        kpCalib.depthImgSize.height = static_cast<float>(kin->getDepthHeight());
        
//        kpCalib.camAspectRatio = kpCalib.imgSize.width / kpCalib.imgSize.height;
        kpCalib.camAspectRatio = kpCalib.camFovX / kpCalib.camFovY;

        // umrechnungsfaktor zwischen onistream FovY und dem gemessenen FovY
//        cout << "kinFovX: " << kin->getDepthFovX() << endl;
//        cout << "kinFovY: " << kin->getDepthFovY() << endl;
        
        kpCalib.oniFov = glm::vec2(kin->getDepthFovX(), kin->getDepthFovY());
           
        cv::initUndistortRectifyMap(kpCalib.cameraMatrix, kpCalib.distCoeffs, cv::Mat(),
                                    kpCalib.cameraMatrix,
                                    cv::Size(kin->getColorWidth(), kin->getColorHeight()),
                                    CV_16SC2, map1, map2);
        kinTex->allocate(kin->getColorWidth(), kin->getColorHeight(),
                         GL_RGBA8, GL_RGB, GL_TEXTURE_2D);
        kinDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                              GL_R16, GL_RED, GL_TEXTURE_2D);
        
        view = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
        rview = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
        viewGray = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC1);
        checkWarp = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
        checkWarpDepth = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_16UC1);
        
//        viewDepth = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_16U);
//        viewDepthBlur = cv::Mat(kin->getDepthHeight(), kin->getDepthWidth(), CV_16U);
        
//        depthData = new uint16_t[kin->getDepthHeight() * kin->getDepthWidth()];
//        depthDataBlur = new uint16_t[kin->getDepthHeight() * kin->getDepthWidth()];
//
        fastBlur = new FastBlur(shCol, kin->getDepthWidth(),
                                kin->getDepthHeight(), GL_R16);
        kinDepthTex = new TextureManager();
        kinDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(), GL_R16, GL_RED, GL_TEXTURE_2D);
        
        isInited = true;
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::initLitShader()
    {
           std::string shdr_Header = "#version 430 core\n#pragma optimize(on)\n";
           std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                           layout( location = 1 ) in vec4 normal;
                                           layout( location = 2 ) in vec2 texCoord;
                                           layout( location = 3 ) in vec4 color;

                                           out TO_FS {
                                        	   vec3 eye_pos;
                                        	   vec3 normal;
                                           } vertex_out;

                                           uniform mat4 m_p;
                                           uniform mat4 m_v;
                                           uniform mat4 m_m;
                                           uniform mat3 m_norm;
                                           void main()
                                           {
                                        	   vertex_out.normal = normalize( m_norm * normal.xyz );
                                               vertex_out.eye_pos = normalize( vec3( m_v * m_m * position ) );
                                               gl_Position = m_p * m_v * m_m * position;
                                           });

           stdVert = "// SNTrustLogo vertex shader\n" +shdr_Header +stdVert;



           shdr_Header = "#version 430 core\n#pragma optimize(on)\n";

           std::string frag = STRINGIFY(uniform sampler2D normMap;
                                        uniform sampler2D litSphereTex;
                                        uniform samplerCube cubeMap;

                                        uniform float reflAmt;
                                        uniform float brightScale;
                                        uniform float morph;
                                        uniform float alpha;

                                        const float Eta = 0.15; // Water

                                        in TO_FS {
                                        	vec3 eye_pos;
                                        	vec3 normal;
                                        } vertex_in;

                                        vec4 orgCol;
                                        vec4 litColor;
                                        float outVal;

                                        layout (location = 0) out vec4 color;

                                        void main()
                                        {

                                        	// litsphere
                                        	vec3 reflection = reflect( vertex_in.eye_pos, vertex_in.normal );
                                        	vec3 refraction = refract( vertex_in.eye_pos, vertex_in.normal, Eta );

                                        	float m = 2.0 * sqrt(
                                       	        pow( reflection.x, 2.0 ) +
                                       	        pow( reflection.y, 2.0 ) +
                                       	        pow( reflection.z + 1.0, 2.0 )
                                        	);

                                        	vec2 vN = reflection.xy / m + 0.5;

                                            litColor = texture(litSphereTex, vN);

                                            vec4 reflectionCol = texture( cubeMap, reflection );
                                            vec4 refractionCol = texture( cubeMap, refraction );

                                            float fresnel = Eta + (1.0 - Eta) * pow(max(0.0, 1.0 - dot(-vertex_in.eye_pos, vertex_in.normal)), 5.0);
                                            //vec4 reflCol = mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0));
                                            vec4 reflCol = reflAmt * mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0) * 0.75);

                                            color = litColor;
//                                            color = (litColor + reflCol) * brightScale;
                                        });

           frag = "// SNTrustLogo fragment shader\n"+shdr_Header+frag;

           litShdr = shCol->addCheckShaderText("SNTrustLogo", stdVert.c_str(), frag.c_str());
       }

    //------------------------------------------------------------------------------
    
    void SNTAugChessb::update(double time, double dt)
    {
        cv::Mat uploadData, uploadDepth;
        float bd, cw;

//        kpCalib.camFovY = (47.8f * osc->blurOffs / 360.f) * 2.f * M_PI;
//        cout << "kpCalib.camFovY: " <<  kpCalib.camFovY << endl;
        
        if (isInited)
        {
        	bool gotNewColFrame = kin->uploadColorImg();

            // depth Frame Operations
            if (actDrawMode == CHECK_KIN_OVERLAY && (gotNewColFrame || kin->uploadDepthImg(false)))
            {
                glm::vec3 kinMid = glm::vec3((float)kin->getColorWidth() * 0.5f,
                                             (float)kin->getColorHeight() * 0.5f, 0.f);
                
                getKinRealWorldCoord(kinMid);
            }
            
            // color Frame Operations
            if (gotNewColFrame)
            {
                frameNr = kin->getColFrameNr();

                // take undistorted kin Color Image, since for this the Kinect internal
                // calibration works
                view.data = kin->getActColorImg();
                uploadData = view;
                
                // blur depth values
                kin->lockDepthMutex();
                kinDepthTex->bind();
                glTexSubImage2D(GL_TEXTURE_2D,             // target
                                0,                          // First mipmap level
                                0, 0,                       // x and y offset
                                kin->getDepthWidth(),
                                kin->getDepthHeight(),
                                GL_RED,
                                GL_UNSIGNED_SHORT,
                                kin->getDepthFrame()->getData());
                
                fastBlur->proc(kinDepthTex->getId());
                fastBlur->downloadData();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                kin->unlockDepthMutex();
                
                // einfaches mapping auf opengl->kamera-Bild
                if (frameNr > 50 && actDrawMode == CHESS_TO_CAM)
                {
                	if (gotChess)
                	{
                		gotChess=false;
                    	if (m_Thread) delete m_Thread;
                    	m_Thread = new boost::thread(&SNTAugChessb::getChessCorners, this);
                	} else {

                		if (startFirstChessThread)
                		{
                			startFirstChessThread = false;
                        	m_Thread = new boost::thread(&SNTAugChessb::getChessCorners, this);
                		}
                	}

                   	chessRotMatr = getRotRealToGLScreen(kpCalib);
                }

/*
                // detect a chessboard in the color image and get corresponding depth values
                // from the kinect
                if (frameNr > 50
                    && ((actDrawMode == CHECK_CAM_TO_BEAMER && gotCamToBeamer < getNrCheckSamples)
                        || actDrawMode == SAVE_CAM_TO_BEAMER))
                {
                    //cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
                    bool found = cv::findChessboardCorners(view, kpCalib.boardSize, pointbuf,
                                                           cv::CALIB_CB_ADAPTIVE_THRESH |
                                                           cv::CALIB_CB_FAST_CHECK |
                                                           cv::CALIB_CB_NORMALIZE_IMAGE);
                    
                    if (found)
                    {
                        if (gotCamToBeamer <= getNrCheckSamples)
                        {
                            chessRotMatr = getRotRealToGL(kpCalib);
                            
                            // get perspective transformation
                            kpCalib.camBeamerPerspTrans = getPerspTrans(pointbuf, glChessBoardWidth, glChessBoardHeight);
                            
                            if (actDrawMode == SAVE_CAM_TO_BEAMER)
                                getRealWorldKinBeamerOffs(kpCalib);
                            
                            gotCamToBeamer++;
                            
                        } else
                        {
                            if(!saved)
                            {                                
                                // 4 iterations should be enough to minize the error
                                for (int i=0;i<4;i++) reCheckRot(kpCalib);
                                saveCamToBeamerCalib(camBeamerMatrPath, kpCalib);
                                saved = true;
                            }
                        }
                    }
                }
                
  */
                // upload kinect image to gl, for debugging
                if(actDrawMode == CHESS_TO_CAM
                   || actDrawMode == SAVE_CAM_TO_BEAMER
                   || (actDrawMode == CHECK_CAM_TO_BEAMER && gotCamToBeamer < getNrCheckSamples))
                {
                   // cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
                    uploadData = view;

                    glBindTexture(GL_TEXTURE_2D, kinTex->getId());
                    glTexSubImage2D(GL_TEXTURE_2D,             // target
                                    0,                          // First mipmap level
                                    0, 0,                       // x and y offset
                                    kin->getColorWidth(),
                                    kin->getColorHeight(),
                                    GL_RGB,
                                    GL_UNSIGNED_BYTE,
                                    uploadData.data);
                }

                /*
                // unwarping with opencv for debugging reasons
                if(actDrawMode == CHECK_KIN_UNWARP)
                {
                    if (switchBeamerToObj == false && gotBeamerToObj < beamerToObjIntv)
                    {
                        //cv::warpPerspective(view, checkWarp, kpCalib.camBeamerPerspTrans, kpCalib.imgSize);

                        bool found = cv::findChessboardCorners(view, kpCalib.boardSize, pointbuf,
                                                               cv::CALIB_CB_ADAPTIVE_THRESH |
                                                               //cv::CALIB_CB_FAST_CHECK |
                                                               cv::CALIB_CB_NORMALIZE_IMAGE);
                        
                        if (found)
                        {
                            //cout << "found object" << endl;
                            kpCalib.invertMatr = false;
                            objRotMatr = getRotRealToCamUnwrap(kpCalib);
//                            objRotMatr = getRotRealToCamToGL(kpCalib);
                            gotBeamerToObj++;
                        }
                    } else
                    {
                        if(switchBeamerToObj == false)
                        {
                            switchBeamerToObj = !switchBeamerToObj;
                            gotBeamerToObj = 0;
                        }
                    }
                    
                    cv::warpPerspective(view, checkWarp, kpCalib.camBeamerPerspTrans, kpCalib.imgSize);
                    glBindTexture(GL_TEXTURE_2D, kinTex->getId());
                    glTexSubImage2D(GL_TEXTURE_2D,             // target
                                    0,                          // First mipmap level
                                    0, 0,                      // x and y offset
                                    kin->getColorWidth(),
                                    kin->getColorHeight(),
                                    GL_RGB,
                                    GL_UNSIGNED_BYTE,
                                    checkWarp.data);
                }
                */

                
                /*
                // calibrate obj to beamer, result is a rotation and translation matrix
                // how is the object translated and rotated from the perspective of the camera?
                if (actDrawMode == CHECK_BEAMER_TO_OBJ && frameNr > 50)
                {
                    if (switchBeamerToObj == false && gotBeamerToObj < beamerToObjIntv)
                    {
                        bool found = cv::findChessboardCorners(view, kpCalib.boardSize, pointbuf,
                                                               cv::CALIB_CB_ADAPTIVE_THRESH |
                                                               //cv::CALIB_CB_FAST_CHECK |
                                                               cv::CALIB_CB_NORMALIZE_IMAGE);
                        
                        if (found)
                        {
                            //cout << "found object" << endl;
                            kpCalib.invertMatr = false;
                            objRotMatr = getRotRealToCamToGL(kpCalib);
                            gotBeamerToObj++;
                        }
                    } else
                    {
                        if(switchBeamerToObj == false)
                        {
                            switchBeamerToObj = !switchBeamerToObj;
                            gotBeamerToObj = 0;
                        }
                    }
                }
*/
            }
        }
    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::getChessCorners()
    {
        bool found = cv::findChessboardCorners(view, kpCalib.boardSize, pointbuf,
                                               cv::CALIB_CB_ADAPTIVE_THRESH |
                                               cv::CALIB_CB_FAST_CHECK |
                                               cv::CALIB_CB_NORMALIZE_IMAGE);

        if (found) gotNewChess = true;
    	gotChess = true;
    }

    //------------------------------------------------------------------------------
    
    void SNTAugChessb::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        if (!isInited && kin->isReady())
        {
            initKinDependent();
        } else
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            
            switch(actDrawMode)
            {
            	case KIN_RGB:
                    shdr->begin();
                    shdr->setIdentMatrix4fv("m_pvm");
                    shdr->setUniform1i("tex", 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());
                    quad->draw();
                    break;
                case CHECK_KIN_OVERLAY:
                    drawKinOverlay();
                    break;
                case CHESS_TO_CAM :
                    drawChessToCam(time, dt, cp);
                    break;
                case CHECK_CAM_TO_BEAMER:
                    drawCamToBeamer();
                    break;
                case SAVE_CAM_TO_BEAMER:
                    drawCamToBeamerInv();
                    break;
                case CHECK_KIN_UNWARP:
                    drawKinUnwrap();
                    break;
                case CHECK_BEAMER_TO_OBJ:
                    drawBeamerToObj();
                    break;
                default:
                    break;
            }
        }
    }

    //------------------------------------------------------------------------------
    
    void SNTAugChessb::drawKinOverlay()
    {
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());
        quad->draw();
        
        blendShader->begin();
        blendShader->setIdentMatrix4fv("m_pvm");
        blendShader->setUniform1f("alpha", 0.5f);
        blendShader->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
        quad->draw();
        
        // draw a cross centered to the screen for debugging
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        
        colShader->begin();
        colShader->setIdentMatrix4fv("m_pvm");
        yLine->draw(GL_LINES);
        xLine->draw(GL_LINES);
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::drawChessToCam(double time, double dt, camPar* cp)
    {
        float near = 1.0f, far = 500.f;
        glm::mat4 beamerMatr = glm::perspectiveFov(kpCalib.camFovY,
                                                   (float)scd->screenWidth,
                                                   (float)scd->screenHeight,
                                                   near, far);
        
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kinTex->getId());
        quad->draw();
        
//        blendShader->begin();
//        blendShader->setUniform1f("alpha", 0.5f);
//        blendShader->setUniform1i("tex", 0);
        
        glm::mat4 aMat = *aImport->getNormAndCenterMatr();
        aMat = chessRotMatr * glm::scale(glm::vec3(7.f)) * aMat;
     //   blendShader->setUniformMatrix4fv("m_pvm", &beamerMatr[0][0]);
        glm::mat3 normMat = glm::mat3( glm::transpose( glm::inverse( chessRotMatr ) ) );


//        glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());
//        chessBoard2->draw();

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // das Logo
        litShdr->begin();
        litShdr->setUniformMatrix4fv("m_p", &beamerMatr[0][0]);
        litShdr->setUniformMatrix4fv("m_v", cp->view_matrix);
        litShdr->setUniformMatrix4fv("m_m", &aMat[0][0]);
        litShdr->setUniformMatrix3fv("m_norm", &normMat[0][0]);
        litShdr->setUniform1i("litSphereTex", 0);
        litShdr->setUniform1i("cubeMap", 1);
        litShdr->setUniform1f("reflAmt", 0.1f);
        litShdr->setUniform1f("brightScale", 1.f);

        litsphereTex->bind(0);
        cubeTex->bind(1);

        aImport->draw(GL_TRIANGLES, litShdr);

    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::drawCamToBeamer()
    {
        if(gotCamToBeamer < getNrCheckSamples)
        {
            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            shdr->setUniform1i("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            
            whiteQuad->draw();
            chessBoard->draw();
        } else
        {
            float near = 0.1f, far = 400.f;
            glm::mat4 beamerMatr = glm::perspectiveFov(kpCalib.camFovY,
                                                       (float)scd->screenWidth,
                                                       (float)scd->screenHeight,
                                                       near, far);
            
            glActiveTexture(GL_TEXTURE0);

            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            shdr->setUniform1i("tex", 0);
            glBindTexture(GL_TEXTURE_2D, kinTex->getId());
            quad->draw();
            
            blendShader->begin();
            blendShader->setUniform1f("alpha", 0.5f);
            blendShader->setUniform1i("tex", 0);
            
            float scaleFact = (kpCalib.chessRealWidth / realBoardWidth) * coordNormFact;
            glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(scaleFact, scaleFact, 1.f));
            chessRotMatr = kpCalib.chessTrans * kpCalib.chessRotXY * kpCalib.chessRotZ * scale;

            beamerMatr = beamerMatr * chessRotMatr;
            blendShader->setUniformMatrix4fv("m_pvm", &beamerMatr[0][0]);
            
            chessBoard3->draw();
        }
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::drawCamToBeamerInv()
    {
        float near = 0.1f, far = 400.f;
        glm::mat4 camMatr = glm::perspective(kpCalib.camFovY, kpCalib.camAspectRatio, near, far);
        glm::mat4 beamerMatr = glm::perspective(kpCalib.beamerFovY, kpCalib.beamerAspectRatio, near, far);
        
        shdr->begin();
        shdr->setIdentMatrix4fv("m_pvm");
        shdr->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        whiteQuad->draw();

        if (gotCamToBeamer < getNrCheckSamples)
        {
            chessBoard->draw();
        } else
        {
            cout << "kpCalib.camBeamerRealOffs: " << glm::to_string(kpCalib.camBeamerRealOffs) << endl;
            cout << "chessRealWidth: " << kpCalib.chessRealWidth << endl;
            cout << "beamerWallDist: " << kpCalib.beamerWallDist << endl;
            cout << "chessRealMid.z: " << kpCalib.chessRealMid.z << endl;
            
            float scaleFact = (kpCalib.chessRealWidth / realBoardWidth) * coordNormFact;
            glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(scaleFact, scaleFact, 1.f));
            
            camMatr = camMatr * kpCalib.chessTrans * glm::inverse(kpCalib.chessRotXY) * scale;
//            camMatr = camMatr * kpCalib.chessTrans * kpCalib.chessRotXY * kpCalib.chessRotZ * scale;
            
            shdr->setUniformMatrix4fv("m_pvm", &camMatr[0][0]);
            chessBoard3->draw();
            
            // draw kinect view
            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, kinTex->getId());
            kinQuad->draw();
        }
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::drawKinUnwrap()
    {
        float near = 0.1f;
        float far = 400.f;
        
        glm::mat4 screenMatr = glm::perspectiveFov(kpCalib.beamerFovY,
                                                   (float)scd->screenWidth,
                                                   (float)scd->screenHeight,
                                                   near, far);

        shdr->begin();
        shdr->setUniform1i("tex", 0);
        shdr->setIdentMatrix4fv("m_pvm");
        whiteQuad->draw();
//        glBindTexture(GL_TEXTURE_2D, kinTex->getId());
//        quad->draw();

        
        
        //        if (switchBeamerToObj == 0 && gotBeamerToObj < beamerToObjIntv)
        //        {
        
        // project test quad, size of the real test chessboard
        glm::vec3 tkinMid = glm::vec3(kpCalib.imgSize.width * 0.5f, kpCalib.imgSize.height * 0.5f, 0.f);
        getKinRealWorldCoord(tkinMid);
        
        // y-Abstand der Kinect in Relation zum kinect Mittelpunkt
        float yOffs = std::sin(kpCalib.camBeamerRots.x) * tkinMid.z;
        float tKinectRectReal = yOffs / std::tan(kpCalib.camBeamerRots.x);
        tBeamerDistRectReal = ((tKinectRectReal - kpCalib.camBeamerRealOffs.z) + oldBeamerDistRectReal *10.f) / 11.f;
        oldBeamerDistRectReal = tBeamerDistRectReal;
        
        chessOffsX = 0.5f;
        chessOffsY = 0.6f;
        
        glm::mat4 scale = glm::translate(glm::mat4(1.f), glm::vec3((chessOffsX -0.5f) * 120.f,
                                                                   (0.6f - chessOffsY) * 100.f,
                                                                   -(tBeamerDistRectReal * coordNormFact)));

        glm::mat4 rotZ = glm::rotate(glm::mat4(1.f), float(M_PI * 0.f), glm::vec3(0.f, 0.f, 1.f));
        scale = screenMatr * scale * rotZ;
        
        shdr->setUniformMatrix4fv("m_pvm", &scale[0][0]);
        chessBoard3->draw();
        
        // } else {
        
        
        if (!(switchBeamerToObj == 0 && gotBeamerToObj < beamerToObjIntv))
        {
            if ((gotBeamerToObj / 4) > beamerToObjIntv)
            {
                switchBeamerToObj = !switchBeamerToObj;
                gotBeamerToObj = 0;
            }
            
            // mach eine perspektivische Matrix, die den Beamer simuliert
            glm::mat4 beamerMat = glm::perspectiveFov(kpCalib.beamerFovY,
                                                      (float)scd->screenWidth,
                                                      (float)scd->screenHeight,
                                                      near, far);
            
            // setup lookAt Matrix
            glm::vec4 beamLookVec = glm::vec4(0.f, 0.f, 1.f, 0.f);
            float beamerAngle = -kpCalib.beamerLookAngle / 360.f * 2.f * M_PI;
            glm::mat4 rot = glm::rotate(glm::mat4(1.f), (float)beamerAngle, glm::vec3(1.f, 0.f, 0.f));
            beamLookVec = rot * beamLookVec;
            
            glm::mat4 lookAt = glm::lookAt(glm::vec3(beamLookVec) * std::abs(kpCalib.distBeamerObj),
                                           glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
            glm::mat4 beamerPersp = beamerMat * lookAt;
//            glm::mat4 thisMatr = beamerPersp * kpCalib.chessTrans;
            glm::mat4 thisMatr = beamerPersp * kpCalib.chessTrans * kpCalib.chessRotXY * kpCalib.chessRotZ;

            // z-offs is now being doing by the lookAt Matrix, so remove it here
            kpCalib.chessTrans[3][2] = 0.f;
            
            // get 2d undistortion matrix
            glm::vec3 chessPos = glm::vec3(0.f, 0.f, -kpCalib.distBeamerObj);
            beamPerspTrans = getBeamerPerspTrans(kpCalib, chessPos);
            
           // ---
            
            blendShader->begin();
            blendShader->setUniform1i("tex", 0);
            blendShader->setUniform1f("alpha", 0.5f);
            blendShader->setUniformMatrix4fv("m_pvm", &thisMatr[0][0]);
            blendShader->setUniformMatrix4fv("de_dist", &beamPerspTrans[0][0]);
            blendShader->setUniform1i("useDedist", 1);
            glBindTexture(GL_TEXTURE_2D, chessBoard3->getTexId());
            //chessBoard->draw();
            chessQA->draw();
            
            gotBeamerToObj++;
        }
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::drawBeamerToObj()
    {
        float near = 0.1f;
        float far = 400.f;
        
        glm::mat4 screenMatr = glm::perspectiveFov(kpCalib.beamerFovY,
                                                   (float)scd->screenWidth,
                                                   (float)scd->screenHeight,
                                                   near, far);
        
//        if (switchBeamerToObj == 0 && gotBeamerToObj < beamerToObjIntv)
//        {
            shdr->begin();
            shdr->setUniform1i("tex", 0);
            shdr->setIdentMatrix4fv("m_pvm");
            whiteQuad->draw();

            // project test quad, size of the real test chessboard
            glm::vec3 tkinMid = glm::vec3(kpCalib.imgSize.width * 0.5f, kpCalib.imgSize.height * 0.5f, 0.f);
            getKinRealWorldCoord(tkinMid);

            // y-Abstand der Kinect in Relation zum kinect Mittelpunkt
            float yOffs = std::sin(kpCalib.camBeamerRots.x) * tkinMid.z;
            float tKinectRectReal = yOffs / std::tan(kpCalib.camBeamerRots.x);
            tBeamerDistRectReal = ((tKinectRectReal - kpCalib.camBeamerRealOffs.z) + oldBeamerDistRectReal *10.f) / 11.f;
            oldBeamerDistRectReal = tBeamerDistRectReal;
            //cout << "tBeamerDistRectReal dist: " << tBeamerDistRectReal << endl;

            chessOffsX = 0.5f;
            chessOffsY = 0.6f;

            glm::mat4 scale = glm::translate(glm::mat4(1.f), glm::vec3((chessOffsX -0.5f) * 120.f,
                                                                       (0.6f - chessOffsY) * 100.f,
                                                                       -(tBeamerDistRectReal * coordNormFact)));
            scale = screenMatr * scale;
        
            shdr->setUniformMatrix4fv("m_pvm", &scale[0][0]);
            //chessBoard3->draw();

       // } else {
        
        
        if (!(switchBeamerToObj == 0 && gotBeamerToObj < beamerToObjIntv))
        {
            if ((gotBeamerToObj / 4) > beamerToObjIntv)
            {
                switchBeamerToObj = !switchBeamerToObj;
                gotBeamerToObj = 0;
            }
            
            // mach eine perspektivische Matrix, die den Beamer simuliert
            glm::mat4 beamerMat = glm::perspectiveFov(kpCalib.beamerFovY,
                                                      (float)scd->screenWidth,
                                                      (float)scd->screenHeight,
                                                      near, far);
            
            /*
            // setup lookAt Matrix
            glm::vec4 beamLookVec = glm::vec4(0.f, 0.f, 1.f, 0.f);
            float beamerAngle = -kpCalib.beamerLookAngle / 360.f * 2.f * M_PI;
            glm::mat4 rot = glm::rotate(glm::mat4(1.f), (float)beamerAngle, glm::vec3(1.f, 0.f, 0.f));
            beamLookVec = rot * beamLookVec;
            
            kpCalib.distBeamerObj = 95.f;
            
            glm::mat4 lookAt = glm::lookAt(glm::vec3(beamLookVec) * kpCalib.distBeamerObj,
                                           glm::vec3(0.f),
                                           glm::vec3(0.f, 1.f, 0.f));
            glm::mat4 beamerPersp = beamerMat * lookAt;
            
            //kpCalib.chessRotXY = glm::rotate(glm::mat4(1.f), float(M_PI * osc->blurOffs), glm::vec3(0.f, 1.f, 0.f));
            
            // z-offs is now being doing by the lookAt Matrix, so remove it here
            kpCalib.chessTrans[3][2] = 0.f;
             */

            //glm::mat4 thisMatr = beamerPersp;
            glm::mat4 thisMatr = beamerMat * kpCalib.chessTrans * kpCalib.chessRotXY * kpCalib.chessRotZ;
            
            
            // get 2d undistortion matrix
//            glm::vec3 chessPos = glm::vec3(0.f, 0.f, kpCalib.distBeamerObj * -coordNormFact);
//            beamPerspTrans = getBeamerPerspTrans(kpCalib, chessPos);
            
            //---

            blendShader->begin();
            blendShader->setUniform1i("tex", 0);
            blendShader->setUniform1f("alpha", 0.5f);
            blendShader->setUniformMatrix4fv("m_pvm", &thisMatr[0][0]);
            blendShader->setUniform1f("lookAngle", (kpCalib.beamerLookAngle / 360.f) * 2.f * M_PI);
            blendShader->setUniform1i("useDedist", 0);
            glBindTexture(GL_TEXTURE_2D, chessBoard3->getTexId());
            //chessBoard3->draw();
            chessQA->draw();

            /*
            glPointSize(2.f);
            colShader->begin();
            colShader->setUniformMatrix4fv("m_pvm", &beamerMat[0][0]);
            //            colShader->setIdentMatrix4fv("m_pvm");
            contrPoints->draw(GL_POINTS);
             */

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            
            colShader->begin();
            colShader->setIdentMatrix4fv("m_pvm");
            yLine->draw(GL_LINES);
            xLine->draw(GL_LINES);
            
           gotBeamerToObj++;
        }
    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::getChessWidth(float& _beamerDist, float& _chessWidth, float _scaleFact)
    {
       // float scaleF = 1.0043f;
        
        // get the width of the chessboard/beamer, this is dependent on the distance
        // get realworld coordinates
        std::vector<glm::vec3> realWorldCoord;
        int ind = 0;
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getKinRealWorldCoord(realWorldCoord.back());
            
            if ((int)rwCoord.size() < (int)pointbuf.size())
                rwCoord.push_back(new Median<glm::vec3>(15.f));
            
            rwCoord[ind]->update(realWorldCoord.back());
            
            ind++;
        }
        
        // get kinect mipoint
        glm::vec3 kinMid = glm::vec3((float)kin->getColorWidth() * 0.5f,
                                     (float)kin->getColorHeight() * 0.5f,
                                     0.f);
        getKinRealWorldCoord(kinMid);
        cout << "kinMid: " <<  glm::to_string(kinMid) << endl;
        
        
        // get the width of the chessboard at the y position of the kinect midpoint on the wall
        // get the two row between which the kinect midpoint lies
        int upperRow = -1;
        int widthHalf = kpCalib.virtBoardSize.width / 2;
        for (int i=0;i<kpCalib.virtBoardSize.height-1;i++)
            if (rwCoord[i * kpCalib.virtBoardSize.width + widthHalf]->get().y > kinMid.y
                && rwCoord[(i+1) * kpCalib.virtBoardSize.width + widthHalf]->get().y < kinMid.y)
                upperRow = i;
        
        if (upperRow != -1 && upperRow <= (kpCalib.virtBoardSize.height-2))
        {
            // get relative position between these two lines
            float distLines = rwCoord[upperRow * kpCalib.virtBoardSize.width + widthHalf]->get().y
                                - rwCoord[(upperRow+1) * kpCalib.virtBoardSize.width + widthHalf]->get().y;
            float relPosToUpper = rwCoord[upperRow * kpCalib.virtBoardSize.width + widthHalf]->get().y / distLines;
            
            // calulate both widths
            float upperWidth = rwCoord[upperRow * kpCalib.virtBoardSize.width + kpCalib.virtBoardSize.width -1]->get().x
                                - rwCoord[upperRow * kpCalib.virtBoardSize.width]->get().x;
            float lowerWidth = rwCoord[(upperRow+1) * kpCalib.virtBoardSize.width + kpCalib.virtBoardSize.width -1]->get().x
                                - rwCoord[(upperRow+1) * kpCalib.virtBoardSize.width]->get().x;
            
            // interpolate between both
            float medWidth = upperWidth * relPosToUpper + (1.f - relPosToUpper) * lowerWidth;
            
            // from this width calculate the full width of the beamer picture
            _chessWidth = medWidth * (float(kpCalib.virtBoardSize.width +1) / float(kpCalib.virtBoardSize.width -1)) * _scaleFact;
            cout << "_chessWidth: " << _chessWidth << endl;
            
            float beamerWidth = _chessWidth * (2.f / glChessBoardWidth);
            //cout << "beamerWidth: " << beamerWidth << endl;

            _beamerDist = beamerWidth * kpCalib.beamerThrowRatio;
            //cout << "_beamerDist: " << _beamerDist << endl;

        } else
        {
            cout << "couldn´t get midPoint location int realtion to chessboard" << endl;
        }
        
        //cout  << endl;
    }
    
    //------------------------------------------------------------------------------
    
    // in case of rectangular Beamer (ultra wide angle, zoom lens)
    // assume a centric Beamer cone anyway and caluculate a yOffs
    // depending on the distance and the characteristics of the beamer
    // gesucht ist eine Rotation/Transformation die den Beamerkonus der Kamera
    // auf den Beamerkonus des Beamers abbildet. also Kamera -> Beamer
    void SNTAugChessb::getRealWorldKinBeamerOffs(kpCalibData& kp)
    {
        // berechne den abstand des Beamers zur wand
        // nimm dazu die breite des kompletten schachbretts und die throwRatio

        // berechne die breite des gesamten Beamer-Bildes
        // un berechne daraus den abstand des beamers zur wand
        //float newBeamerWallDist = kp.beamerThrowRatio * kp.chessRealWidth * (2.f / glChessBoardWidth);
        float newBeamerWallDist = (kp.chessRealWidth * (2.f / glChessBoardWidth)) / (2.f * std::tan(kp.beamerFovX * 0.5f));
        beamerWallDist->update(newBeamerWallDist);
        kp.beamerWallDist = beamerWallDist->get();
        cout << "kp.beamerWallDist: " << kp.beamerWallDist << endl;
        
        // berechne den abstand der Kinect zur Wand, nimm dazu den Mittelpunkt
        // des kinect color bildes
        glm::vec3 kinMid = glm::vec3(kp.imgSize.width * 0.5f, kp.imgSize.height * 0.5f, 0.f);
        getKinRealWorldCoord(kinMid);
        
        // get chess realworld coordinates
        std::vector<glm::vec3> realWorldCoord;
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getKinRealWorldCoord(realWorldCoord.back());
        }

        kp.chessRealMid = getChessMidPoint(realWorldCoord, kp, kp.boardSize);

        // get rotation matrix to rotate chess-PointBuf to Beamerector plane2
        //glm::mat4 invRot = glm::mat4(RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f)));
        
        glm::mat4 invRot = glm::rotate(glm::mat4(1.f), kp.rotations.x, glm::vec3(1.f, 0.f, 0.f))
                            * glm::rotate(glm::mat4(1.f), kp.rotations.y, glm::vec3(0.f, 1.f, 0.f));
  
        
        for (std::vector<glm::vec3>::iterator it = realWorldCoord.begin(); it != realWorldCoord.end(); ++it)
            // take the realworld chess Coordinates, rotate them to fit the Beamerection plane of the kamera
            // rotate around the MidPoint of the chess
            (*it) = glm::vec3(invRot * glm::vec4((*it) - kp.chessRealMid, 1.f)) + kp.chessRealMid;

        
        // berechne die Position der Kinect in Relation zum Beamer
        float yOffs = std::sin(kp.rotations.x) * kinMid.z;
        
        // y-Abstand der Kinect in Relation zum kinect Mittelpunkt
        kp.camWallRectReal = std::cos(kp.rotations.x) * kinMid.z;
        cout << "kp.camWallRectReal: " << kp.camWallRectReal << endl;

        
        // offset kinect-beamer bei ultraweit winkel Beamerektions-konus
        float realKinBeamerYOffs, newBmWidth, newBmHeight;

        newBmWidth = kp.beamerWallDist / kp.beamerThrowRatio;
        newBmHeight = ((float)scd->screenHeight / (float)scd->screenWidth) * newBmWidth;
        
        beamerWidth->update(newBmWidth);
        beamerHeight->update(newBmHeight);
        cout << "groesse Beamer w: " << beamerWidth->get() << " h:" << beamerHeight->get() << endl;
        
        // bei ULTRA_WIDE abstand des unteren Bildrandes zur Beamer Linse
        float vOffsBeamer = std::tan(kp.beamerLowEdgeAngle) * kp.beamerWallDist;
        cout << "vOffsBeamer: " << vOffsBeamer << endl;

        //cout << "abstand bildmitte: Beamerektor kante: " << BeamerectionHeight / 2 + vOffsBeamerektion << endl;
        
        // nur zur kontrolle, abstand y beamer/ kamera ohne die beamer model korrektur
        //cout << "abstand-y kin/Beamer real bei ultra-weit winkel: " << (beamerHeight / 2 + vOffsBeamerektion) - (chessMid.y - pk.y) << endl;
        
        //float pkY = (kp.chessRealMid.y + (std::sin(rotXAlpha) * kinMid.z));
        
        /*
        glm::vec3 newOffs = glm::vec3(kp.chessRealMid.x,
                                      (beamerHeight->get() * 0.5f + vOffsBeamer) - (kp.chessRealMid.y - kinMid.y) - yOffs,
                                      kp.camWallRectReal - kp.beamerWallDist);
        
        kinCamOffset->update(newOffs);
        kp.camBeamerRealOffs = kinCamOffset->get();
        cout << "kinect Offset: " << glm::to_string(kp.camBeamerRealOffs) << endl;
         */

    }
    
    //------------------------------------------------------------------------------

    glm::mat4 SNTAugChessb::getRotRealToGLScreen(kpCalibData& kp)
    {
    	glm::mat4 out;
    	glm::vec3 midPoint;

    	if (gotNewChess)
    	{
    		int ind = 0;
    		chessRealWorldCoord.clear();
    		for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
    		{
    			chessRealWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
    			getKinRealWorldCoord(chessRealWorldCoord.back());

    			ind++;
    		}
    		gotNewChess = false;
    	}

    	if (chessRealWorldCoord.size() > 0)
    	{
    		// brechne die normale der wand / chessboard zur Kinect Ebene
    		kp.chessRotXY = calcChessRotXY(chessRealWorldCoord, kp, kp.boardSize);
    		// get the translation
    		kp.chessTrans = calcChessTrans(chessRealWorldCoord, kp, kp.boardSize);

    		kp.chessRotZ = calcChessRotZ(chessRealWorldCoord, kp, kp.boardSize, kp.rotations);
    	}

    	// combine the three transformations
    	out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;

    	return out;
    }
    
    //------------------------------------------------------------------------------
    
    // convert the input coordinates of the kinect to realworld coordinates.
    // to do this, first correct the projection error of the camera
    // (this is align the camera and beamer plane)
    glm::mat4 SNTAugChessb::getRotRealToCamToGL(kpCalibData& kp)
    {
        glm::mat4 out;
        std::vector<glm::vec3> realWorldCoord;
        std::vector<double> rotXAngles;
        glm::vec4 rotated;

        glm::mat4 rotXMat = glm::rotate(glm::mat4(1.f), -kp.camBeamerRots.x, glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 rotYMat = glm::rotate(glm::mat4(1.f), kp.camBeamerRots.y, glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 rotZMat = glm::rotate(glm::mat4(1.f), kp.camBeamerRots.z, glm::vec3(0.f, 0.f, 1.f));
        
       // cout << "" << endl;

        // get realworld coordinates of found chessboard corners
        realWorldCoord.clear();
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getKinRealWorldCoord(realWorldCoord.back());
        }

        // calculate the mid Point in realworld coordinates
        glm::vec3 chessMidPoint = getChessMidPoint(realWorldCoord, kp, kp.boardSize);
     //   cout << "chessMidPoint: " << glm::to_string(chessMidPoint) << endl;
        
       // float rotXAngAdd = std::atan2(chessMidPoint.y, chessMidPoint.z);
        //cout << "rotXAngAdd: " << rotXAngAdd << endl;

        // perspective correction, rotate around origin and object center
        // to get the correct z and y values
        GLfloat* ptr = (GLfloat*) contrPoints->getMapBuffer(POSITION);
        
        for (std::vector<glm::vec3>::iterator it = realWorldCoord.begin(); it != realWorldCoord.end(); ++it)
        {
            (*it) = glm::vec3(rotYMat * glm::vec4((*it) - chessMidPoint, 1.f)) + chessMidPoint;
            (*it) = glm::vec3(rotZMat * glm::vec4((*it) - chessMidPoint, 1.f)) + chessMidPoint;
        }

    //    cout << endl;
        
        for (std::vector<glm::vec3>::iterator it = realWorldCoord.begin(); it != realWorldCoord.end(); ++it)
        {
            // correct the y-values by rotating with rot.x around the center of the boards
            (*it) = getRealOffs((*it), kp, rotXMat);

            // assign value to debug vao
            *ptr = (*it).x * coordNormFact; ptr++;
            *ptr = (*it).y * coordNormFact; ptr++;
            *ptr = (*it).z * -coordNormFact; ptr++;
            
        }
        
        contrPoints->unMapBuffer();

        chessMidPoint = getChessMidPoint(realWorldCoord, kp, kp.boardSize);
        cout << "rot chessMidPoint: " << glm::to_string(chessMidPoint) << endl;
        
        // get rotation around y and z axis
        kp.chessRotXY = calcChessRotXY(realWorldCoord, kp, kp.boardSize);
        kp.chessRotXY = glm::rotate(kp.chessRotXY,
                                    (osc->blurFboAlpha - 0.5f) * float(M_PI) * 0.25f,
                                    glm::vec3(1.f, 0.f, 0.f));
        kp.chessRotXY = glm::rotate(kp.chessRotXY,
                                    (osc->blurFdbk - 0.5f) * float(M_PI) * 0.25f,
                                    glm::vec3(0.f, 1.f, 0.f));

        // get the translation
        kp.chessTrans = calcChessTrans(realWorldCoord, kp, kp.boardSize);
        kp.chessTrans = glm::translate(kp.chessTrans, glm::vec3((osc->alpha - 0.5f) * 10.f,
                                                                (osc->feedback - 0.5f) * 10.f,
                                                                (osc->blurOffs - 1.f) * 10.f));
        
        // get rotation around the z-axis
        kp.chessRotZ = calcChessRotZ(realWorldCoord, kp, kp.boardSize, kp.rotations);
        kp.chessRotZ = glm::rotate(kp.chessRotZ,
                                    (osc->rotYAxis - 0.5f) * float(M_PI) * 0.125f,
                                    glm::vec3(0.f, 1.f, 0.f));

        // combine the three transformations
        if(!kp.invertMatr)
        {
            out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        } else {
            out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        }
        
        kp.distBeamerObj *= coordNormFact;
        
        return out;
    }
    
    //------------------------------------------------------------------------------

    glm::mat4 SNTAugChessb::getRotRealToGL(kpCalibData& kp)
    {
        glm::mat4 out;
        glm::vec3 midPoint;        
        std::vector<glm::vec3> realWorldCoord;

        int ind = 0;
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getKinRealWorldCoord(realWorldCoord.back());
            
            if ((int)rwCoord.size() < (int)pointbuf.size())
                rwCoord.push_back(new Median<glm::vec3>(10.f));
            rwCoord[ind]->update(realWorldCoord.back());
            
            ind++;
        }
        
        
        // calculate the size of the board in relation to boardSize,
        // use this to convert real to normalized opengl coordinates
        kp.chessRealWidth = 0.f;
        float heightReal = 0.f;
        for (short h=0;h<kp.boardSize.height;h++)
            kp.chessRealWidth += glm::length(rwCoord[h * kp.boardSize.width]->get()
                                             - rwCoord[(h+1) * kp.boardSize.width -1]->get());
        
        kp.chessRealWidth /= static_cast<float>(kp.boardSize.height);
        kp.chessRealWidth *= (static_cast<float>(kp.boardSize.width +1) / static_cast<float>(kp.boardSize.width -1));
        cout << "chessboard real width: " << kp.chessRealWidth << endl;

        for (short h=0;h<kp.boardSize.width;h++)
            heightReal += glm::length(rwCoord[h * kp.boardSize.height]->get()
                                     - rwCoord[(h+1) * kp.boardSize.height -1]->get());
        
        heightReal /= static_cast<float>(kp.boardSize.width);
        heightReal *= (static_cast<float>(kp.boardSize.height +1) / static_cast<float>(kp.boardSize.height -1));
        cout << "chessboard real height " << heightReal << endl;
        
        
        
        // brechne die normale der wand / chessboard zur Kinect Ebene
//        kp.chessRotXY = calcChessRotMed(rwCoord, kp, kp.boardSize);
        kp.chessRotXY = calcChessRotXYWall(pointbuf, kp, kp.boardSize);
        
        
        // distance to kinect midpoint for debugging reasons
        glm::vec3 kinMid = glm::vec3((float)kin->getColorWidth() * 0.5f,
                                     (float)kin->getColorHeight() * 0.5f,
                                     0.f);
        getKinRealWorldCoord(kinMid);
        cout << "kinMid: " << glm::to_string(kinMid) << endl;
        
        // get the translation
        kp.chessTrans = calcChessTrans(realWorldCoord, kp, kp.boardSize);
        // correct chessMid Z
        kp.chessRealMid.z = kp.camWallRectReal;
        cout << "Beamer Mid: " << glm::to_string(kp.chessRealMid) << endl;

        kp.chessRotZ = calcChessRotZ(realWorldCoord, kp, kp.boardSize, kp.rotations);

        // combine the three transformations
        out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        
        cout << endl;

        return out;
    }
    
    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::getRotRealToCamUnwrap(kpCalibData& kp)
    {
        glm::mat4 out;
        std::vector<glm::vec3> realWorldCoord;
        std::vector<glm::vec2> unwarpCoords;
        glm::vec4 rotated;
        float beamOffsY, kinOffsY;

        
        cout << "" << endl;
        
        // get calibrated BeamerMid in Relation to Kinect Origin
        //float beamMidAbsY = std::tan(kp.camBeamerRots.x) * kp.beamerRealMid.z + kp.beamerRealMid.y + kp.camBeamerRealOffs.y;
        double beamMidAbsY = std::tan(kp.camBeamerRots.x) * kp.beamerRealMid.z + kp.beamerRealMid.y;
        cout << "beamMidAbsY: " << beamMidAbsY << endl;
        
        float beamMidRotX = std::atan2(beamMidAbsY, kp.beamerRealMid.z);
        cout << "beamMidRotX: " << beamMidRotX << endl;
        
        
        glm::mat4 perspMatr = cvMat33ToGlm(kp.camBeamerPerspTrans);
        
        // unwarp the found chessboard coordinates
        glm::vec3 chessMidPoint = glm::vec3(0.f, 0.f, 0.f);
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            unwarpCoords.push_back( glm::vec2((*it).x, (*it).y) );
            perspUnwarp(unwarpCoords.back(), perspMatr);
            chessMidPoint += glm::vec3(unwarpCoords.back(), 0.f);
        }
        
        chessMidPoint /= (float)pointbuf.size();
        chessMidPoint.x = chessMidPoint.x / (float)kin->getColorWidth();
        chessMidPoint.y = 1.f - (chessMidPoint.y / (float)kin->getColorHeight());
        chessMidPoint = (chessMidPoint + glm::vec3(-0.5f, -0.5f, 0.f)) * glm::vec3(2.f, 2.f, 1.f);
        cout << "midPoint: " << glm::to_string(chessMidPoint) << endl;


        // get the correct z-values (realworld coordinates with the beamer as origin)
        // of the chessboard by inverse rotating with the calibrated values around the kinect origin
        int ind = 0;
        float resX = static_cast<float>(kin->getColorWidth());
        float resY = static_cast<float>(kin->getColorHeight());
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            // get correct z-value
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getKinRealWorldCoord(realWorldCoord.back());
            realWorldCoord.back() = glm::vec3(glm::rotate(glm::mat4(1.f), -kp.camBeamerRots.x,
                                                          glm::vec3(1.f, 0.f, 0.f))
                                              * glm::vec4(realWorldCoord.back(), 1.f) );
            
            kinOffsY = std::tan(beamMidRotX) * realWorldCoord.back().z + kp.camBeamerRealOffs.y;
            
            realWorldCoord.back().z -= kp.camBeamerRealOffs.z;

            // unproject unwarpCoords
            double normalizedX = unwarpCoords[ind].x / resX - .5f;
            double normalizedY = .5f - unwarpCoords[ind].y / resY;
            
            double xzFactor = std::tan(kpCalib.beamerFovX * 0.5f) * 2.f;  // stimmt noch nicht... wieso?
            double yzFactor = std::tan(kpCalib.beamerFovY * 0.5f) * 2.f;  // stimmt!!!
            
            realWorldCoord.back().x = static_cast<float>(normalizedX * realWorldCoord.back().z * xzFactor);
            realWorldCoord.back().y = static_cast<float>(normalizedY * realWorldCoord.back().z * yzFactor);

            // calculate the height of the beamer middline at the actual distance
            beamOffsY = std::abs(std::tan(kp.beamerLookAngle * 0.0174532925199f)) * realWorldCoord.back().z;

            realWorldCoord.back().y += kinOffsY - beamOffsY;
            
//            cout << "kinOffsY: " << kinOffsY << endl;
//            cout << "beamOffsY: " << beamOffsY << endl;
//            cout << "yOffs: " << kinOffsY - beamOffsY << endl;
            
           // cout << glm::to_string(realWorldCoord.back()) << endl;
            ind++;
        }
        
        
        // get the perspective transformation matrix of the actual object
        /*
        cv::Mat trans = getPerspTrans(pointbuf, glChessBoardWidth, glChessBoardHeight);
        cv::Mat mtxR, mtxQ, Qx, Qy, Qz;
        cv::Vec3d rotAngles = cv::RQDecomp3x3(trans, mtxR, mtxQ, Qx, Qy, Qz);
        cout << "rotAngles: " << rotAngles << endl;

        kp.chessRotXY = cvMat33ToGlm(Qx) * cvMat33ToGlm(Qy) * cvMat33ToGlm(Qz);
        kp.chessRotXY = glm::inverse(kp.chessRotXY);
        */
        
        // get rotation around y and z axis
        kp.chessRotXY = calcChessRotXY(realWorldCoord, kp, kp.boardSize);
        // get the translation
        kp.chessTrans = calcChessTrans(realWorldCoord, kp, kp.boardSize);
        // get rotation around the z-axis
        kp.chessRotZ = calcChessRotZ(realWorldCoord, kp, kp.boardSize, kp.rotations);
        
        // combine the three transformations
        if(!kp.invertMatr)
        {
            out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        } else {
            out = kp.chessTrans * kp.chessRotXY * kp.chessRotZ;
        }
        
        return out;
    }

    //------------------------------------------------------------------------------
    
    void SNTAugChessb::getKinRealWorldCoord(glm::vec3& inCoord)
    {
        double depthScale = 1.0;
        float resX = static_cast<float>(kin->getDepthWidth());
        float resY = static_cast<float>(kin->getDepthHeight());
        
        // in case depth and color stream have different sizes
        inCoord.x = inCoord.x / static_cast<float>(kin->getColorWidth()) * static_cast<float>(kin->getDepthWidth());
        inCoord.y = inCoord.y / static_cast<float>(kin->getColorHeight()) * static_cast<float>(kin->getDepthHeight());
     
        //kin->lockDepthMutex();
        //const openni::DepthPixel* pDepthPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
        const openni::DepthPixel* blurPix = (const openni::DepthPixel*)fastBlur->getDataR16();

        int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
        //pDepthPix += rowSize * static_cast<int>(inCoord.y);
        //pDepthPix += static_cast<int>(inCoord.x);
        
        blurPix += rowSize * static_cast<int>(inCoord.y);
        blurPix += static_cast<int>(inCoord.x);
        
        //kin->unlockDepthMutex();
        
        // asus xtion tends to measure lower depth with increasing distance
        // experimental correction
        //        depthScale = 1.0 + std::pow(static_cast<double>(*blurPix) * 0.00032, 5.3);

        double multDiff = static_cast<double>(*blurPix) - 950.0;
        double upperCorr = std::fmax(multDiff, 0) * 0.02;
        double scaledDepth = static_cast<double>(*blurPix) + (multDiff * 0.0102) + upperCorr;
        
        
        glm::vec2 fov = glm::vec2(kpCalib.camFovX, kpCalib.camFovY);
        
        double normalizedX = inCoord.x / resX - .5f;
        double normalizedY = .5f - inCoord.y / resY;
        
        double xzFactor = std::tan(fov.x * 0.5f) * 2.f;  // stimmt noch nicht... wieso?
        double yzFactor = std::tan(fov.y * 0.5f) * 2.f;  // stimmt!!!

        inCoord.x = static_cast<float>(normalizedX * scaledDepth * xzFactor);
        inCoord.y = static_cast<float>(normalizedY * scaledDepth * yzFactor);
        inCoord.z = static_cast<float>(scaledDepth);
    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::getKinRealWorldCoordUnwarp(glm::vec3& inCoord, glm::mat4 _perspMatr)
    {
        float resX = static_cast<float>(kin->getDepthWidth());
        float resY = static_cast<float>(kin->getDepthHeight());
        
        // in case depth and color stream have different sizes
        inCoord.x = inCoord.x / static_cast<float>(kin->getColorWidth()) * static_cast<float>(kin->getDepthWidth());
        inCoord.y = inCoord.y / static_cast<float>(kin->getColorHeight()) * static_cast<float>(kin->getDepthHeight());
        
        // get unwarped pixel coordinates
        glm::vec2 unwarpPixCoord = glm::vec2(inCoord.x, inCoord.y);
        perspUnwarp(unwarpPixCoord, _perspMatr);
        
        const openni::DepthPixel* blurPix = (const openni::DepthPixel*)fastBlur->getDataR16();
        
        int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
        blurPix += rowSize * static_cast<int>(inCoord.y);
        blurPix += static_cast<int>(inCoord.x);
        
        // asus xtion tends to measure lower depth with increasing distance
        // experimental correction
        double scaledDepth = 1.0 + std::pow(static_cast<double>(*blurPix) * 0.00033, 5.3);
        
        // kinect v1 precision gets worse by distance
        // generally the distance will be a bit lower
        scaledDepth = static_cast<double>(*blurPix) * scaledDepth;
        
        glm::vec2 fov = glm::vec2(kpCalib.camFovX, kpCalib.camFovY);
        
        double normalizedX = unwarpPixCoord.x / resX - .5f;
        double normalizedY = .5f - unwarpPixCoord.y / resY;
        
        double xzFactor = std::tan(fov.x * 0.5f) * 2.f;  // stimmt noch nicht... wieso?
        double yzFactor = std::tan(fov.y * 0.5f) * 2.f;  // stimmt!!!
        
        inCoord.x = static_cast<float>(normalizedX * scaledDepth * xzFactor);
        inCoord.y = static_cast<float>(normalizedY * scaledDepth * yzFactor);
        inCoord.z = static_cast<float>(scaledDepth);
    }
    
    //------------------------------------------------------------------------------

    glm::vec3 SNTAugChessb::getRealOffs(glm::vec3& point, kpCalibData& kp, glm::mat4& rotXMat)
    {
        glm::vec3 outP;
        
        // first get the y-offset of the point to the intersection of kinect-middleline
        // and the vertical plane parallel to the y-axis of the beamer coordinate system
        float zDist = std::sqrt(point.z * point.z + point.y * point.y);
        
        // get angle of Point in relation to Kinect middle-line
        double pAngle = std::atan2(point.y, point.z);
        
        // get real distance of kinect origin to object
        float pKinDist = std::cos(-kp.camBeamerRots.x - pAngle) * zDist;
        //cout << "pKinDist: " << pKinDist << endl;

        float kinMidZ = pKinDist / std::cos(-kp.camBeamerRots.x);
        float kinMidYOffs = std::sqrt(point.y * point.y + std::pow(kinMidZ - point.z, 2.0)) * copysignf(1.0, point.y);

        // now get the offset to the beamer center

        // kinectMidPoint Y-value at the given distance in relation to "floor" or beamer lens origin
        // in case of non straight throwing optics
        float kinMidYToBeamer = std::tan(kp.camBeamerRots.x) * pKinDist + kp.camBeamerRealOffs.y;
        //cout << "kinMidYToBeamer: " << kinMidYToBeamer << endl;

        // proyected image center at the object distance
        float beamerToObjDist = (pKinDist - kp.camBeamerRealOffs.z);
        float beamerScrHeightAtObj = beamerToObjDist / (kp.beamerThrowRatio * kp.beamerAspectRatio);
        float beamerBottYOffs = std::tan(kp.beamerLowEdgeAngle) * beamerToObjDist;
        float beamerMidY = beamerScrHeightAtObj * 0.5f + beamerBottYOffs;
//        cout << "kinMidYToBeamer: " << kinMidYToBeamer << endl;
//        cout << "beamerMidY: " << beamerMidY << endl;
        float offsKinMidBeamerMidY = kinMidYToBeamer - beamerMidY;
//        cout << "offsKinMidBeamerMidY: " << offsKinMidBeamerMidY << endl;
//        cout << "kinMidYOffs: " << kinMidYOffs << endl;

        point.y = kinMidYOffs + offsKinMidBeamerMidY;
        point.z = pKinDist - kp.camBeamerRealOffs.z;
        
        outP.x = point.x;
        outP.y = point.y;
        outP.z = point.z;
        
        return outP;
    }
    
    //------------------------------------------------------------------------------

    
    glm::vec3 SNTAugChessb::getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp,
                                             cv::Size boardSize)
    {
        glm::vec3 midPoint = glm::vec3(0.f);
        
        // calculate midpoints in the vertical axis
        // the outer most corners and iterate to the center
        vector<glm::vec3> hMidsEnd;
        
        for (short x=0;x<boardSize.width;x++)
        {
            vector<glm::vec3> hMids;
            for (short h=0;h<boardSize.height/2;h++)
            {
                glm::vec3 midP = _realWorldCoord[x + (h * boardSize.width)]
                                + _realWorldCoord[x + (boardSize.height -h -1) * boardSize.width];
                hMids.push_back(midP * 0.5f);
            }
            
            // get mediums
            glm::vec3 medMidPH = glm::vec3(0.f, 0.f, 0.f);
            for (short h=0;h<boardSize.height/2;h++)
                medMidPH += hMids[h];
            
            hMidsEnd.push_back(medMidPH / static_cast<float>(boardSize.height/2));
        }
        
        vector<glm::vec3> vMids;
        int hMidsEndSizeHalf = static_cast<int>(hMidsEnd.size())/2;
        
        // iterate over all vMids and calc mediums for each pair
        for (int x=0;x<hMidsEndSizeHalf;x++)
        {
            glm::vec3 midP = hMidsEnd[x] + hMidsEnd[static_cast<int>(hMidsEnd.size()) -1 -x];
            vMids.push_back(midP * 0.5f);
        }
        
        midPoint = glm::vec3(0.f, 0.f, 0.f);
        for (short i=0;i<static_cast<short>(vMids.size());i++)
            midPoint += vMids[i];
        
        if(static_cast<int>(vMids.size()) > 0)
            midPoint /= static_cast<float>(vMids.size());
        
        return midPoint;
    }
    
    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::calcChessRotXYWall(std::vector<cv::Point2f> _pointbuf, kpCalibData& kp,
                                               cv::Size boardSize)
    {
        float fIndJ, fIndI;
        vector<double> medVals;
        glm::mat4 outMat = glm::mat4(1.f);
        int nrTest = 40;
        float sampleWidth = 0.8f;
        
        // calculate x-Axis rotation take vertical test columns and get the rotation angle for each
        // get test column
        for(int j=0;j<nrTest;j++)
        {
            vector<glm::vec3> testCol;
            fIndJ = static_cast<float>(j) / static_cast<float>(nrTest);
            
            for(int i=0;i<nrTest;i++)
            {
                fIndI = static_cast<float>(i) / static_cast<float>(nrTest);
                testCol.push_back(glm::vec3((float)kin->getColorWidth() * (fIndJ * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
                                            (float)kin->getColorHeight() * (fIndI * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
                                            0.f));
                getKinRealWorldCoord(testCol.back());
            }
            
            // berechne die steigung für alle punkte
            double midVal = 0;
            for(int i=0;i<nrTest-1;i++)
                midVal += (M_PI * 0.5) - std::atan2(testCol[i].y - testCol[i+1].y, (testCol[i].z - testCol[i+1].z));
            midVal /= static_cast<double>(nrTest);
            medVals.push_back(midVal);
        }
        
        // get final x-rotation
        //rotations.x = 0.f;
        float rotX = 0.f;
        for(int j=0;j<nrTest;j++) rotX += medVals[j];
        rotX /= -static_cast<float>(nrTest);
        kp.rotations.x = rotX;
        cout << "rotation x-axis: " << rotX << endl;
        
        
        
        medVals.clear();
        // calculate y-Axis rotation take vertical test columns and get the rotation angle for each
        // get test column
        for(int j=0;j<nrTest;j++)
        {
            vector<glm::vec3> testRow;
            fIndJ = static_cast<float>(j) / static_cast<float>(nrTest);
            
            for(int i=0;i<nrTest;i++)
            {
                fIndI = static_cast<float>(i) / static_cast<float>(nrTest);
                testRow.push_back(glm::vec3((float)kin->getColorWidth() * (fIndI * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
                                            (float)kin->getColorHeight() * (fIndJ * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
                                            0.f));
                getKinRealWorldCoord(testRow.back());
            }
            
            // berechne die steigung für alle punkte
            double midVal = 0;
            for(int i=0;i<nrTest-1;i++)
                midVal += std::atan2(testRow[i+1].z - testRow[i].z, testRow[i+1].x - testRow[i].x);
            midVal /= static_cast<double>(nrTest);
            
            medVals.push_back(midVal);
        }
        
        // get final x-rotation
        float rotY = 0.f;
        for(int j=0;j<nrTest;j++) rotY += medVals[j];
        rotY /= -static_cast<float>(nrTest);
        cout << "rotation y-axis: " << rotY << endl;
        kp.rotations.y = rotY;
        rotations->add(glm::vec3(rotX, rotY, 0.f));
        
        
        // quaternion
        glm::quat quatX = glm::angleAxis(kp.rotations.x, glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 rotQuatX = glm::mat4(quatX);
        
        glm::quat quatY = glm::angleAxis(kp.rotations.y, glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 rotQuatY = glm::mat4(quatY);
        
        outMat = rotQuatX * rotQuatY;
        
        //        if (kp.invertMatr)
        //            outMat = glm::inverse(outMat);
        
        return outMat;
    }
    
    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::calcChessRotXY(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp,
                                           cv::Size boardSize)
    {
        glm::mat4 outMat = glm::mat4(1.f);
        vector< glm::vec3 > normals;
        
        // calculate rotation, by calculating normals for all corners
        // through the conversion in realworld coordinates the perspective
        // distortion is automatically corrected, so the result is corrected
        for (short y=1;y<kp.boardSize.height;y++)
        {
            for (short x=0;x<kp.boardSize.width -1;x++)
            {
                glm::vec3 act = _realWorldCoord[y * kp.boardSize.width + x];
                glm::vec3 upper = _realWorldCoord[(y-1) * kp.boardSize.width + x];
                glm::vec3 right = _realWorldCoord[y * kp.boardSize.width + x+1];
                
                glm::vec3 upVec = upper - act;
                glm::vec3 rightVec = right - act;
                
                normals.push_back( glm::normalize ( glm::cross( glm::normalize(upVec), glm::normalize(rightVec) ) ) );
            }
        }
        
        // calculate medium normal
        glm::vec3 outNorm = glm::vec3(0.f, 0.f, 0.f);
        for (std::vector<glm::vec3>::iterator it = normals.begin(); it != normals.end(); ++it)
            if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
                outNorm += (*it);
        
        if ((int) normals.size() > 0)
        {
            outNorm /= (float)normals.size();
            
            cbNormal->update(outNorm);
            kp.chessNormal = cbNormal->get();
           // cout << "chessNormal: " << glm::to_string(kp.chessNormal) << endl;

            // quaternion
            glm::quat rot = RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f));
            outMat = glm::mat4(rot);
            kp.rotations = glm::eulerAngles(rot);
           // cout << "kp.rotations: " << glm::to_string(kp.rotations) << endl;
            
            if (kp.invertMatr)
                outMat = glm::inverse(outMat);
        }
        
        return outMat;
    }
    
    //------------------------------------------------------------------------------
    
    // rotations resulting from normals vary about 1 degree. this is caused by the kinect...
    glm::mat4 SNTAugChessb::calcChessRotMed(std::vector< Median<glm::vec3>* >& _realWorldCoord, kpCalibData& kp,
                                              cv::Size boardSize)
    {
        glm::mat4 outMat = glm::mat4(1.f);
        vector< glm::vec3 > normals;

        // calculate rotation, by calculating normals for all corners
        // through the conversion in realworld coordinates the perspective
        // distortion is automatically corrected, so the result is corrected
        for (short y=1;y<kp.boardSize.height;y++)
        {
            for (short x=0;x<kp.boardSize.width -1;x++)
            {
                glm::vec3 act = _realWorldCoord[y * kp.boardSize.width + x]->get();
                glm::vec3 upper = _realWorldCoord[(y-1) * kp.boardSize.width + x]->get();
                glm::vec3 right = _realWorldCoord[y * kp.boardSize.width + x+1]->get();
                
                glm::vec3 upVec = upper - act;
                glm::vec3 rightVec = right - act;
                
                normals.push_back( glm::normalize ( glm::cross( glm::normalize(upVec), glm::normalize(rightVec) ) ) );
            }
        }
        
        // calculate medium normal
        glm::vec3 outNorm = glm::vec3(0.f, 0.f, 0.f);
        for (std::vector<glm::vec3>::iterator it = normals.begin(); it != normals.end(); ++it)
            if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
                outNorm += (*it);
        
        if ((int) normals.size() > 0)
        {
            outNorm /= (float)normals.size();
            outNorm = glm::normalize(outNorm);
            
            cbNormal->update(outNorm);
            kp.chessNormal = cbNormal->get();
           // cout << "chessNormal: " << glm::to_string(kp.chessNormal) << endl;
            
            // quaternion
            glm::quat rot = RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f));
            outMat = glm::mat4(rot);
            kp.rotations = glm::eulerAngles(rot);
          //  cout << "kp.rotations: " << glm::to_string(kp.rotations) << endl;

            if (kp.invertMatr)
                outMat = glm::inverse(outMat);
        }
        
        return outMat;
    }
    
    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::calcChessRotZ(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp,
                                          cv::Size boardSize, glm::vec3 _rotations)
    {
        glm::mat4 outMat = glm::mat4(1.f);
        
//        glm::quat rot = RotationBetweenVectors(glm::vec3(0.f, 0.f, -1.f), kp.chessNormal);
        glm::quat rot = RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f));
        glm::mat4 chessRotInvXYMatr = glm::mat4(rot);
        
//        if (kp.invertMatr) chessRotInvXYMatr = glm::inverse(chessRotInvXYMatr);

        vector< glm::vec4 > leftVecs;
        
        // rotate the chess into the x-y plane
        chessbXyPlane.clear();
        for (std::vector<glm::vec3>::iterator it = _realWorldCoord.begin(); it != _realWorldCoord.end(); ++it)
        {
            chessbXyPlane.push_back( chessRotInvXYMatr * glm::vec4((*it).x, (*it).y, (*it).z, 1.f) );
           // cout << glm::to_string(chessbXyPlane.back()) << endl;
        }
        
      //  cout << endl;
        
        
        // calculate rotation
        // calculate vector pointing to +x for all corners
        for (short y=0;y<boardSize.height;y++)
        {
            for (short x=0;x<boardSize.width -1;x++)
            {
                glm::vec4 act = chessbXyPlane[y * boardSize.width + x];
                glm::vec4 right = chessbXyPlane[y * boardSize.width + x+1];
                leftVecs.push_back( glm::normalize ( right - act ) );
            }
        }
        
        // calculate medium leftVec
        glm::vec4 outLeftVec = glm::vec4(0.f, 0.f, 0.f, 0.f);
        for (std::vector<glm::vec4>::iterator it = leftVecs.begin(); it != leftVecs.end(); ++it)
            if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
                outLeftVec += (*it);
        
        if (static_cast<int>(leftVecs.size()) > 0)
        {
            outLeftVec /= (float)leftVecs.size();
            
            cbLeftVec->update(outLeftVec);
            glm::vec3 xyVec = glm::vec3(cbLeftVec->get());
            xyVec.z = 0.f;
            
            if(!kp.invertMatr)
                outMat = glm::mat4(RotationBetweenVectors(glm::vec3(1.f, 0.f, 0.f), xyVec));
            else
                outMat = glm::mat4(RotationBetweenVectors(xyVec, glm::vec3(1.f, 0.f, 0.f)));
        }
        
        return outMat;
    }
    
    
    // die z-koordinate bezieht auf den echten (unrektifizierten) abstand
    // des Chessboard Mittelpunktes zur Kinect
    
    glm::mat4 SNTAugChessb::calcChessTrans(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp,
                                           cv::Size boardSize)
    {
        glm::mat4 out;
        
        kp.chessRealMid = getChessMidPoint(_realWorldCoord, kp, boardSize);
        
        if(kp.chessRealMid.x != 0.f
           && kp.chessRealMid.y != 0.f
           && kp.chessRealMid.z != 0.f)
        {
            kp.chessRealMid *= coordNormFact;   // leave the coordinates as is, only scale by 0.1f
                                                // for better handling
            
            // apply kalman filter
            if (std::isfinite(kp.chessRealMid.x)
                && std::isfinite(kp.chessRealMid.y)
                && std::isfinite(kp.chessRealMid.z))
            {
                kalmanTrans->predict();
                kalmanTrans->update(kp.chessRealMid.x,
                                    kp.chessRealMid.y,
                                    std::min(std::max(kp.chessRealMid.z, 0.0001f), 1000.f));
                cv::Mat smoothed = kalmanTrans->getPrediction();
                
                if(!kp.invertMatr)
                {
                    out = glm::translate(glm::mat4(1.f), glm::vec3(smoothed.at<float>(0),
                                                                   smoothed.at<float>(1),
                                                                   -smoothed.at<float>(2)));
//                    cout << glm::to_string(glm::vec3(smoothed.at<float>(0),
//                                                     smoothed.at<float>(1),
//                                                     -smoothed.at<float>(2))) << endl;
                } else
                    out = glm::translate(glm::mat4(1.f), glm::vec3(-smoothed.at<float>(0),
                                                                   -smoothed.at<float>(1),
                                                                   -smoothed.at<float>(2)));

                kp.distBeamerObj = std::abs(smoothed.at<float>(2));
            }
        }
        
        return out;
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::reCheckRot(kpCalibData& kp)
    {
        std::vector<glm::vec3> realWorldCoord;
        
        glm::mat4 rot = glm::rotate(glm::mat4(1.f), kp.rotations.x, glm::vec3(1.f, 0.f, 0.f))
                        * glm::rotate(glm::mat4(1.f), kp.rotations.y, glm::vec3(0.f, 1.f, 0.f))
                        * glm::rotate(glm::mat4(1.f), kp.rotations.z, glm::vec3(0.f, 0.f, 1.f));
        
        for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
        {
            realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
            getKinRealWorldCoord(realWorldCoord.back());
            realWorldCoord.back() = glm::vec3( rot * glm::vec4(realWorldCoord.back(), 1.f) );
            // cout << glm::to_string( realWorldCoord.back() ) << endl;
        }
        
        // get 4 normals from the edges
        std::vector<glm::vec3> sides;
        glm::vec3 lowerLeft = realWorldCoord[kp.boardSize.width * (kp.boardSize.height-1)];
        glm::vec3 lowerRight = realWorldCoord[kp.boardSize.width * kp.boardSize.height -1];
        glm::vec3 upperRight = realWorldCoord[kp.boardSize.width -1];
        glm::vec3 upperLeft = realWorldCoord[0];
        
        
        // lower left to upper left
        sides.push_back( upperLeft - lowerLeft );
        // lower left to lower right
        sides.push_back( lowerRight - lowerLeft);
        
        // lower right to lower left
        sides.push_back( lowerLeft - lowerRight );
        // lower right to upper right
        sides.push_back( upperRight - lowerRight);
        
        // upper right to lower right
        sides.push_back( lowerRight - upperRight );
        // upper right to upper left
        sides.push_back( upperLeft - upperRight );
        
        // upper left to upper right
        sides.push_back( upperRight - upperLeft );
        // upper left to lower left
        sides.push_back( lowerLeft - upperLeft );
        
        
        for (std::vector<glm::vec3>::iterator it = sides.begin(); it != sides.end(); ++it)
            (*it) = glm::normalize((*it));
        
        
        // get normals and calculate medium
        glm::vec3 medNormal = glm::vec3(0.f);
        for (int i=0;i<static_cast<int>(sides.size()) / 2;i++)
        {
            medNormal += glm::cross( sides[i *2], sides[i *2 +1] );
            //cout << "norm[" << i << "]: " << glm::to_string(glm::cross( sides[i *2], sides[i *2 +1] )) << endl;
        }
        
        medNormal /= static_cast<float>(sides.size()) * 0.5f;
        cout << "final norm: " << glm::to_string(medNormal) << endl;
        
        glm::quat rotQ = RotationBetweenVectors(medNormal, glm::vec3(0.f, 0.f, -1.f));
        glm::vec3 rotAngles = glm::eulerAngles(rotQ);
        cout << "rotAngles: " << glm::to_string(rotAngles) << endl;
        
        // apply found correction
        kp.rotations += rotAngles;
    }

    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::getBeamerPerspTrans(kpCalibData& kp, glm::vec3& _chessPos)
    {
        glm::mat4 out;
        cv::Mat cvOut;
        float near = 0.1f; float far = 400.f;
        float fWidth = (float)scd->screenWidth;
        float fHeight = (float)scd->screenHeight;
        
        // create a source and destination pointbuffer with a quad
        std::vector< cv::Point2f > srcPoint;
        std::vector< cv::Point2f > dstPoint;
        std::vector< glm::vec3 > gSrcPoint;

        dstPoint.push_back( cv::Point2f(-1.f, -1.f) );
        dstPoint.push_back( cv::Point2f( 1.f, -1.f) );
        dstPoint.push_back( cv::Point2f( 1.f,  1.f) );
        dstPoint.push_back( cv::Point2f(-1.f,  1.f) );


        // get the size of a quad filling the whole screen at the given distance
        float srcScaleY = std::tan(kpCalib.beamerFovY *0.5f) * -_chessPos.z;
        float srcScaleX = srcScaleY * ((float)scd->screenWidth / (float)scd->screenHeight);
        
        gSrcPoint.push_back( glm::vec3(-srcScaleX, -srcScaleY, 0.f) );
        gSrcPoint.push_back( glm::vec3( srcScaleX, -srcScaleY, 0.f) );
        gSrcPoint.push_back( glm::vec3( srcScaleX,  srcScaleY, 0.f) );
        gSrcPoint.push_back( glm::vec3(-srcScaleX,  srcScaleY, 0.f) );
        

        // construct a perspective transformation with the realworld relations
        // of the beamer
        glm::mat4 beamerPersp = glm::perspectiveFov(kpCalib.beamerFovY,
                                                    (float)scd->screenWidth,
                                                    (float)scd->screenHeight,
                                                    near, far);
        
        glm::vec4 beamLookVec = glm::vec4(0.f, 0.f, 1.f, 0.f);
        float beamerAngle = -kpCalib.beamerLookAngle / 360.f * 2.f * M_PI;
        glm::mat4 rot = glm::rotate(glm::mat4(1.f), (float)beamerAngle, glm::vec3(1.f, 0.f, 0.f));
        beamLookVec = rot * beamLookVec;
        beamLookVec = glm::normalize(beamLookVec) * std::abs(_chessPos.z);
        
        glm::mat4 lookAt = glm::lookAt(glm::vec3(beamLookVec),
                                       glm::vec3(0.f),
                                       glm::vec3(0.f, 1.f, 0.f));

        beamerPersp = beamerPersp * lookAt;
        
        // transform the src points with this matrix and convert them to cv::Points
        for (std::vector<glm::vec3>::iterator it = gSrcPoint.begin(); it != gSrcPoint.end(); ++it)
        {
            (*it) = glm::project((*it), glm::mat4(1.f), beamerPersp, glm::vec4(-1.f, -1.f, 2.f, 2.f));
            srcPoint.push_back( cv::Point2f( (*it).x, (*it).y ) );
        }
        
        // now get the perspective transformation matrix (2d)
        cvOut = cv::getPerspectiveTransform(srcPoint, dstPoint);
        out = cvMat33ToGlm(cvOut);

        return out;
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::doPerspUndist(std::vector<glm::vec4>& inPoints, std::vector<glm::vec3>& outPoints, glm::mat4& perspMatr)
    {
        const double eps = FLT_EPSILON;
        
        // do only 2d transformation
        int ind=0;
        for (std::vector<glm::vec4>::iterator it = inPoints.begin(); it != inPoints.end(); ++it)
        {
            //cout <<  "before: " << glm::to_string(*it) << endl;
            
            float x = (*it).x, y = (*it).y;
            double w = x * perspMatr[0][2] + y * perspMatr[1][2] + perspMatr[2][2];
            
            if( std::fabs(w) > eps )
            {
                w = 1./w;
                outPoints[ind].x = (float)((x * perspMatr[0][0] + y * perspMatr[1][0] + perspMatr[2][0]) *w);
                outPoints[ind].y = (float)((x * perspMatr[0][1] + y * perspMatr[1][1] + perspMatr[2][1]) *w);
            }
            else
                outPoints[ind].x = outPoints[ind].y = (float)0;
            
            //cout <<  "after: " << glm::to_string(outPoints[ind]) << endl;
            
            ind++;
        }
    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::perspUnwarp(glm::vec2& inPoint, glm::mat4& perspMatr)
    {
        const double eps = FLT_EPSILON;
        
        float x = inPoint.x, y = inPoint.y;
        double w = x * perspMatr[0][2] + y * perspMatr[1][2] + perspMatr[2][2];
            
        if( std::fabs(w) > eps )
        {
            w = 1./w;
            inPoint.x = (float)((x * perspMatr[0][0] + y * perspMatr[1][0] + perspMatr[2][0]) *w);
            inPoint.y = (float)((x * perspMatr[0][1] + y * perspMatr[1][1] + perspMatr[2][1]) *w);
        }
        else
            inPoint.x = inPoint.y = (float)0;
    }

    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::rtMatToGlm(int imgW, int imgH, cv::Mat& rVec, cv::Mat& tVec, bool inv)
    {
        glm::mat4 outM = glm::mat4(1.f);
        glm::vec3 transV;
        
        if(inv)
        {
            transV = glm::vec3(static_cast<float>(tVec.at<double>(0) * -1.0),
                               static_cast<float>(tVec.at<double>(1)),
                               static_cast<float>(tVec.at<double>(2)));
            outM = glm::translate(outM, transV);
            
            outM = glm::rotate(outM, (float)rVec.at<double>(0) * -1.f, glm::vec3(1.f, 0.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(1), glm::vec3(0.f, 1.f, 0.f));
            outM = glm::rotate(outM, (float)rVec.at<double>(2), glm::vec3(0.f, 0.f, 1.f));
        } else
        {
            transV = glm::vec3(static_cast<float>(tVec.at<double>(0)),
                               static_cast<float>(tVec.at<double>(1) * -1.0),
                               static_cast<float>(tVec.at<double>(2) * -1.0));
            outM = glm::translate(outM, transV);
            
            outM = glm::rotate(outM, static_cast<float>(rVec.at<double>(0)), glm::vec3(1.f, 0.f, 0.f));
            outM = glm::rotate(outM, static_cast<float>(rVec.at<double>(1) * -1.0), glm::vec3(0.f, 1.f, 0.f));
            outM = glm::rotate(outM, static_cast<float>(rVec.at<double>(2) * -1.0), glm::vec3(0.f, 0.f, 1.f));
        }
        
        return outM;
    }
    
    //------------------------------------------------------------------------------
    
    glm::mat4 SNTAugChessb::getBeamerFromCv(cv::Mat& camMatr, float calibWidth, float calibHeight, float znear,
                                           float zfar, bool yUp)
    {
        glm::mat4 outM = glm::mat4(1.f);
        
        // See http://strawlab.org/2011/11/05/augmented-reality-with-OpenGL/
        //if (!intrinsic_valid) {throw "invalid intrinsic";}
        
        float depth = zfar - znear;
        float q = -(zfar + znear) / depth;
        float qn = -2 * (zfar * znear) / depth;
        
        float x0=0;
        float y0=0;
        
        float width = calibWidth;
        float height = calibHeight;
        
        if (yUp)
        {
            outM[0][0] = 2.f * camMatr.at<double>(0) / width;
            outM[1][0] = -2.f * camMatr.at<double>(1) / width;
            outM[1][1] = -2.f * camMatr.at<double>(4) / height;
            outM[2][0] = (width - 2.f * camMatr.at<double>(2) + 2.f * x0) / width;
            outM[2][1] = (height - 2.f * camMatr.at<double>(5) + 2.f * y0) / height;
            outM[2][2] = q;
            outM[2][3] = -1.f;
            outM[3][2] = qn;
            
        } else
        {
            outM[0][0] = 2.f * camMatr.at<double>(0) / width;
            outM[1][0] = -2.f * camMatr.at<double>(1) / width;
            outM[1][1] = 2.f * camMatr.at<double>(4) / height;
            outM[2][0] = (width - 2.f * camMatr.at<double>(2) + 2.f * x0) / width;
            outM[2][1] = (-height - 2.f * camMatr.at<double>(5) + 2.f * y0) / height;
            outM[2][2] = q;
            outM[2][3] = -1.f;
            outM[3][2] = qn;
        }
        
        return outM;
    }
    
    //------------------------------------------------------------------------------

    cv::Mat SNTAugChessb::getPerspTrans(std::vector<cv::Point2f>& pointbuf, float _chessWidth,
                                        float _chessHeight)
    {
        cv::Mat out;
        vector<cv::Point2f> inPoint;
        vector<cv::Point2f> dstPoint;
        
        // we don´t take the far out edges of the chessboard
        // but the next inner edges.
        float chessW = _chessWidth * static_cast<float>(kpCalib.boardSize.width -1)
                        / static_cast<float>(kpCalib.boardSize.width +1)
                        * 0.5f * kpCalib.imgSize.width;
        float chessH = _chessHeight * static_cast<float>(kpCalib.boardSize.height-1)
                        / static_cast<float>(kpCalib.boardSize.height +1)
                        * 0.5f * kpCalib.imgSize.height;
        
        float chessOffsX = (2.f - _chessWidth) * 0.25f * kpCalib.imgSize.width
                            + chessW / static_cast<float>(kpCalib.boardSize.width -1);
        float chessOffsY = (2.f - _chessHeight) * 0.25f * kin->getColorHeight()
                            + chessH / static_cast<float>(kpCalib.boardSize.height -1);
        
        // lower left (in point upper, weil kommt auf d. kopf rein)
        for( int y = 0; y < 2; y++ )
            for( int x = 0; x < 2; x++ )
                dstPoint.push_back(cv::Point2f(float(x) * chessW + chessOffsX,
                                               float(y) * chessH + chessOffsY));
        
        inPoint.push_back(pointbuf[0]);
        inPoint.push_back(pointbuf[kpCalib.boardSize.width-1]);
        inPoint.push_back(pointbuf[(kpCalib.boardSize.height-1) * kpCalib.boardSize.width]);
        inPoint.push_back(pointbuf[(kpCalib.boardSize.height-1) * kpCalib.boardSize.width + kpCalib.boardSize.width -1]);

        out = cv::getPerspectiveTransform(inPoint, dstPoint);

//        cv::Mat mtxR, mtxQ, Qx, Qy, Qz;
//        cv::Vec3d rotAngles = cv::RQDecomp3x3(out, mtxR, mtxQ, Qx, Qy, Qz);
//        cout << "rotAngles: " << rotAngles << endl;
        
        return out;
    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::loadCamToBeamerCalib(cv::FileStorage& fs, kpCalibData& kp)
    {
        
        fs["camera_matrix"] >> kp.cameraMatrix;
        fs["distortion_coefficients"] >> kp.distCoeffs;
        
        fs["invertMatr"] >> kp.invertMatr;
        fs["image_width"] >> kp.imgSize.width;
        fs["image_height"] >> kp.imgSize.height;
        fs["depth_image_width"] >> kp.depthImgSize.width;
        fs["depth_image_height"] >> kp.depthImgSize.height;

        fs["board_width"] >> kp.boardSize.width;
        fs["board_height"] >> kp.boardSize.height;
        fs["virtual_board_width"] >> kp.virtBoardSize.width;
        fs["virtual_board_height"] >> kp.virtBoardSize.height;

        fs["nrSamples"] >> kp.nrSamples;
        fs["beamerAspectRatio"] >> kp.beamerAspectRatio;
        fs["beamerFovX"] >> kp.beamerFovX;
        fs["beamerFovY"] >> kp.beamerFovY;
        fs["beamerLookAngle"] >> kp.beamerLookAngle;
        fs["beamerLowEdgeAngle"] >> kp.beamerLowEdgeAngle;
        fs["beamerThrowRatio"] >> kp.beamerThrowRatio;
        fs["beamerWallDist"] >> kp.beamerWallDist;
        fs["camAspectRatio"] >> kp.camAspectRatio;
        fs["camWallRectReal"] >> kp.camWallRectReal;
        
        fs["chessRealWidth"] >> kp.chessRealWidth;
        fs["camFovX"] >> kp.camFovX;
        fs["camFovY"] >> kp.camFovY;
        
        cv::Mat pRotXY, pRotZ, pTrans;
        fs["camBeamerRotXY"] >> pRotXY;
        fs["camBeamerRotZ"] >> pRotZ;
        fs["camBeamerTrans"] >> pTrans;
        fs["camBeamerPerspTrans"] >> kp.camBeamerPerspTrans;
        fs["camBeamerInvPerspTrans"] >> kp.camBeamerInvPerspTrans;

        kp.camBeamerRotXY = cvMatToGlm(pRotXY);
        kp.camBeamerRotZ = cvMatToGlm(pRotZ);
        kp.camBeamerTrans = cvMatToGlm(pTrans);
        
        if(!kp.invertMatr)
        {
            kp.camBeamerRotXY = glm::inverse(kp.camBeamerRotXY);
            kp.camBeamerRotZ = glm::inverse(kp.camBeamerRotZ);
        }
        
        cv::Mat beamerRealMid, camBeamerRotations, camBeamerRealOffs;
        fs["camBeamerRealOffs"] >> camBeamerRealOffs;
        fs["beamerRealMid"] >> beamerRealMid;
        fs["camBeamerRotations"] >> camBeamerRotations;

        kp.camBeamerRealOffs = glm::vec3(camBeamerRealOffs.at<float>(0),
                                         camBeamerRealOffs.at<float>(1),
                                         camBeamerRealOffs.at<float>(2));

        kp.beamerRealMid = glm::vec3(beamerRealMid.at<float>(0),
                                     beamerRealMid.at<float>(1),
                                     beamerRealMid.at<float>(2));
        
        kp.camBeamerRots = glm::vec3(-camBeamerRotations.at<float>(0),
                                     camBeamerRotations.at<float>(1),
                                     camBeamerRotations.at<float>(2));
        
        std::string beamMod;
        fs["beamerModel"] >> beamMod;
        
        if (std::strcmp(beamMod.c_str(), "ZOOM") == 0)
            kp.beamModel = ZOOM;
        
        if (std::strcmp(beamMod.c_str(), "WIDE") == 0)
            kp.beamModel = WIDE;
        
        if (std::strcmp(beamMod.c_str(), "ULTRA_WIDE") == 0)
            kp.beamModel = ULTRA_WIDE;
        
        camBeamerRotMatr = kp.camBeamerTrans * kp.camBeamerRotXY * kp.camBeamerRotZ;
        
        gotCamToBeamer = getNrCheckSamples;
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::saveCamToBeamerCalib(std::string _filename, kpCalibData& kp)
    {
        printf("saving camera to beamer calib \n");
        
        cv::FileStorage fs( _filename, cv::FileStorage::WRITE );
        
        fs << "camera_matrix" << kp.cameraMatrix;
        fs << "distortion_coefficients" << kp.distCoeffs;
        
        fs << "invertMatr" << kp.invertMatr;
        fs << "image_width" << kp.imgSize.width;
        fs << "image_height" << kp.imgSize.height;
        fs << "depth_image_width" << kp.depthImgSize.width;
        fs << "depth_image_height" << kp.depthImgSize.height;
        fs << "board_width" << kp.boardSize.width;
        fs << "board_height" << kp.boardSize.height;
        fs << "virtual_board_width" << kp.virtBoardSize.width;
        fs << "virtual_board_height" << kp.virtBoardSize.height;
        fs << "nrSamples" << kp.nrSamples;
        fs << "beamerAspectRatio" << kp.beamerAspectRatio;
        fs << "beamerFovX" << kp.beamerFovX;
        fs << "beamerFovY" << kp.beamerFovY;

        fs << "beamerLookAngle" << kp.beamerLookAngle;
        fs << "beamerLowEdgeAngle" << kp.beamerLowEdgeAngle;
        fs << "beamerThrowRatio" << kp.beamerThrowRatio;
        fs << "beamerWallDist" << kp.beamerWallDist;
        fs << "camAspectRatio" << kp.camAspectRatio;
        fs << "camWallRectReal" << kp.camWallRectReal;
        fs << "chessRealWidth" << kp.chessRealWidth;
        fs << "camFovX" << kp.camFovX;
        fs << "camFovY" << kp.camFovY;
        
        fs << "camBeamerRotXY" << glmToCvMat(kp.chessRotXY);
        fs << "camBeamerRotZ" << glmToCvMat(kp.chessRotZ);
        glm::mat4 camTrans = glm::translate(glm::mat4(1.f), kp.camBeamerRealOffs);
        fs << "camBeamerTrans" << glmToCvMat(camTrans);
        fs << "camBeamerPerspTrans" << kp.camBeamerPerspTrans;
        fs << "camBeamerInvPerspTrans" << kp.camBeamerPerspTrans.inv();


        cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
        glmVec.at<float>(0) = kp.chessRealMid.x;
        glmVec.at<float>(1) = kp.chessRealMid.y;
        glmVec.at<float>(2) = kp.camWallRectReal;
        fs << "beamerRealMid" << glmVec;
        
//        glmVec.at<float>(0) = kp.chessNormal.x;
//        glmVec.at<float>(1) = kp.chessNormal.y;
//        glmVec.at<float>(2) = kp.chessNormal.z;
//        fs << "camBeamerNormal" << glmVec;

        glmVec.at<float>(0) = kp.rotations.x;
        glmVec.at<float>(1) = kp.rotations.y;
        glmVec.at<float>(2) = kp.rotations.z;
        fs << "camBeamerRotations" << glmVec;

        glmVec.at<float>(0) = kp.camBeamerRealOffs.x;
        glmVec.at<float>(1) = kp.camBeamerRealOffs.y;
        glmVec.at<float>(2) = kp.camBeamerRealOffs.z;
        fs << "camBeamerRealOffs" << glmVec;

        switch(kp.beamModel)
        {
            case ZOOM: fs << "beamerModel" << "ZOOM";
                break;
            case WIDE: fs << "beamerModel" << "WIDE";
                break;
            case ULTRA_WIDE: fs << "beamerModel" << "ULTRA_WIDE";
                break;
        }
    }

    //------------------------------------------------------------------------------

    void SNTAugChessb::saveBeamerToObjCalib(std::string _filename, kpCalibData& kp)
    {
        printf("saving camera to beamer calib \n");
        
        cv::FileStorage fs( _filename, cv::FileStorage::WRITE );
        
        fs << "rotXY" << glmToCvMat(kp.chessRotXY);
        fs << "rotZ" << glmToCvMat(kp.chessRotZ);
        fs << "trans" << glmToCvMat(kp.chessTrans);
    }

    //------------------------------------------------------------------------------
    
    cv::Mat SNTAugChessb::glmToCvMat(glm::mat4& mat)
    {
        cv::Mat out = cv::Mat(4, 4, CV_32F);
        for (short j=0;j<4;j++)
            for (short i=0;i<4;i++)
                out.at<float>(j*4 + i) = mat[i][j];

        return out;
    }
    
    //------------------------------------------------------------------------------

    glm::mat4 SNTAugChessb::cvMatToGlm(cv::Mat& _mat)
    {
        glm::mat4 out = glm::mat4(1.f);
        for (short j=0;j<4;j++)
            for (short i=0;i<4;i++)
                out[i][j] = _mat.at<float>(j*4 + i);
        
        return out;
    }
    
    //------------------------------------------------------------------------------

    glm::mat4 SNTAugChessb::cvMat33ToGlm(cv::Mat& _mat)
    {
        glm::mat4 out = glm::mat4(1.f);
        for (short j=0;j<3;j++)
            for (short i=0;i<3;i++)
                out[i][j] = _mat.at<double>(j*3 + i);
                
                return out;
    }
    
    //------------------------------------------------------------------------------
    
    inline double SNTAugChessb::getRotX(glm::vec3& _normal)
    {
        //float zp = std::sqrt(_normal.z * _normal.z - _normal.x * _normal.x) * (_normal.z / std::abs(_normal.z));
        return M_PI - std::atan2((double)_normal.y, (double)_normal.z);
    }
    
    //------------------------------------------------------------------------------
    
    inline double SNTAugChessb::getRotY(glm::vec3& _normal)
    {
        //float zp = std::sqrt(_normal.z * _normal.z - _normal.x * _normal.x) * (_normal.z / std::abs(_normal.z));
        return M_PI - std::atan2((double)_normal.x, (double)_normal.z);
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::onKey(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_D:
                    deDist = !bool(deDist);
                    cout << "deDist: " << deDist << endl;
                break;
                case GLFW_KEY_S:
                    saveBeamerToObjCalib((*scd->dataPath)+"calib_cam/rotObjMap.yml", kpCalib);
                    cout << "save obj calib " << endl;
                break;
            }
        }
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::onCursor(GLFWwindow* window, double xpos, double ypos)
    {
        chessOffsX = static_cast<float>(xpos / static_cast<double>(scd->screenWidth));
        chessOffsY = static_cast<float>(ypos / static_cast<double>(scd->screenHeight));
    }
    
    //------------------------------------------------------------------------------
    
    void SNTAugChessb::onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
    }
    
    //------------------------------------------------------------------------------

    std::string SNTAugChessb::execCmd(char* cmd)
    {
        FILE* pipe = popen(cmd, "r");
        if (!pipe) return "ERROR";
        char buffer[128];
        std::string result = "";
        while(!feof(pipe)) {
            if(fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
        pclose(pipe);
        return result;
    }

    //------------------------------------------------------------------------------
    
    SNTAugChessb::~SNTAugChessb()
    {
        delete colShader;
        delete blendShader;
        delete quad;
        delete noFlipQuad;
        delete kinQuad;
        delete whiteQuad;
        delete xLine;
        delete yLine;
        delete kinTex;
        delete chessBoard;
        delete chessBoard2;
        delete kalmanRot;
        delete kalmanRotZ;
        delete kalmanTrans;
    }
    
}
