//
// SNTrustOpticalFlow.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustOpticalFlow.h"

#define STRINGIFY(A) #A


namespace tav
{
SNTrustOpticalFlow::SNTrustOpticalFlow(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"NoLight"), vidStartOffs(20), downscale(2)
{
#ifdef HAVE_CUDA
	VideoTextureCvActRange** vts = static_cast<VideoTextureCvActRange**>(scd->videoTextsActRange);
	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, false);

	flipQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

	d_frame = new cv::cuda::GpuMat[2];
	d_framef = new cv::cuda::GpuMat[2];
	size_frame = cv::Mat(vt->getHeight() / downscale, vt->getWidth() / downscale, CV_8UC3);

    

	ogl_buf = new cv::ogl::Buffer[2];
    VAOId = new GLuint[2];

	for (int i=0;i<2;i++)
	{
		ogl_buf[i] = cv::ogl::Buffer(
		    		vt->getHeight() / downscale,
					vt->getWidth() / downscale,
					CV_32FC2,
					cv::ogl::Buffer::ARRAY_BUFFER,
					false);

		// create vao for rendering ogl_buf obj
		glGenVertexArrays(1, &VAOId[i]);           // returns 1 unused names for use as VAO in the array VAO
		glBindVertexArray(VAOId[i]);               // create VAO, assign the name and bind that array

		ogl_buf[i].bind(cv::ogl::Buffer::ARRAY_BUFFER);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		cv::ogl::Buffer::unbind(cv::ogl::Buffer::ARRAY_BUFFER);

		glBindVertexArray(0);               // create VAO, assign the name and bind that array
	}

    

    brox = cv::cuda::BroxOpticalFlow::create(0.197f, 50.0f, 0.8f, 10, 77, 10);
    lk = cv::cuda::DensePyrLKOpticalFlow::create(cv::Size(7, 7));
    farn = cv::cuda::FarnebackOpticalFlow::create();
    tvl1 = cv::cuda::OpticalFlowDual_TVL1::create();

    flow = new GLSLOpticalFlow(shCol, vt->getWidth(), vt->getHeight());
    tMed = new GLSLTimeMedian(shCol, vt->getWidth()/ downscale,	vt->getHeight()/ downscale, GL_RGBA8);

    stdTex = shCol->getStdTexAlpha();
    initShdr();

	addPar("median", &median);
	addPar("flowFdbk", &flowFdbk);
	addPar("alpha", &alpha);

	medFbo = new FBO(shCol, vt->getWidth()/ downscale, vt->getHeight()/ downscale, 1);
#endif
}

//----------------------------------------------------

void SNTrustOpticalFlow::initShdr()
{
     //- Position Shader ---

     std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

     std::string vert = STRINGIFY(layout (location=0) in vec2 val;\n
                                  uniform int width;\n
                                  uniform int height;\n
                                  out vec4 col;\n
                                  bool first = true;\n
                                  float pi = 3.14159265358979323846;\n

                                  vec3 computeColor(float fx, float fy)\n
                                  {
                                      // relative lengths of color transitions:
                                      // these are chosen based on perceptual similarity
                                      // (e.g. one can distinguish more shades between red and yellow
                                      //  than between yellow and green)
                                      const int RY = 15;\n
                                      const int YG = 6;\n
                                      const int GC = 4;\n
                                      const int CB = 11;\n
                                      const int BM = 13;\n
                                      const int MR = 6;\n
                                      const int NCOLS = RY + YG + GC + CB + BM + MR;\n
                                      vec3 colorWheel[NCOLS];\n

                                      if (first)\n
                                      {\n
                                          int k = 0;\n

                                          for (int i = 0; i < RY; ++i, ++k)\n
                                              colorWheel[k] = vec3(255.0, float(255 * i) / RY, 0);\n

                                          for (int i = 0; i < YG; ++i, ++k)\n
                                              colorWheel[k] = vec3(float(255 - 255 * i) / YG, 255.0, 0);\n

                                          for (int i = 0; i < GC; ++i, ++k)
                                              colorWheel[k] = vec3(0, 255.0, float(255 * i) / GC);\n

                                          for (int i = 0; i < CB; ++i, ++k)
                                              colorWheel[k] = vec3(0, float(255 - 255 * i) / CB, 255.0);\n

                                          for (int i = 0; i < BM; ++i, ++k)\n
                                              colorWheel[k] = vec3(float(255 * i) / BM, 0, 255.0);\n

                                          for (int i = 0; i < MR; ++i, ++k)\n
                                              colorWheel[k] = vec3(255.0, 0, float(255 - 255 * i) / MR);\n

                                          first = false;\n
                                      }\n

                                      float rad = sqrt(fx * fx + fy * fy);\n
                                      float a = atan(-fy, -fx) / pi;\n
                                      float fk = (a + 1.0) / 2.0 * float(NCOLS - 1);\n
                                      int k0 = int(fk);\n
                                      int k1 = (k0 + 1) % NCOLS;\n
                                      float f = fk - float(k0);\n

                                      vec3 pix;\n

                                      for (int b = 0; b < 3; b++)\n
                                      {
                                          float col0 = colorWheel[k0][b] / 255.0;\n
                                          float col1 = colorWheel[k1][b] / 255.0;\n
                                          float col = (1.0 - f) * col0 + f * col1;\n

                                          if (rad <= 1.0) {\n
                                              col = 1.0 - rad * (1.0 - col);\n // increase saturation with radius
                                      	  } else {\n
                                              col *= .75;\n // out of range
                                      	  }
                                          pix[2 - b] = col;\n
                                      }\n

                                      return pix;
                                  }

                                  void main() {\n
                                	  gl_PointSize = 4.f;
                                      vec2 pos = vec2(
                                    		  float(gl_VertexID % width) / float(width),
                                    		  float(gl_VertexID / width) / float(height) );\n
                                      pos.y = 1.0 - pos.y;

                                      pos = (pos - 0.5) * 2.0;
                                      col = vec4(computeColor(val.x *0.2, val.y * 0.2), 1.0);
                                      col = vec4(1.0 - col.r, 1.0 - col.g, 1.0 - col.b, 1.0);

//                                      col = vec4(val, 0.0, 1.0);
                                      gl_Position = vec4(pos, 0.0, 1.0);\n
                                  });
     vert = "// SNTrustOpticalFlow pos tex vertex shader\n" +shdr_Header +vert;


     //- Frag Shader ---

     shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
     std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                  in vec4 col;\n

                                  void main()\n {\n
                                      fragColor = col;\n
                                  });
     frag = "// SNTrustOpticalFlow pos tex shader\n"+shdr_Header+frag;

     displOptFlow = shCol->addCheckShaderText("SNTrustOpticalFlow",
                                                         vert.c_str(), frag.c_str());
 }

//----------------------------------------------------

void SNTrustOpticalFlow::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

#ifdef HAVE_CUDA
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

    // if there is a new frame copy it to an opengl array buffer
	if(actUplTexId != 0 && actUplTexId != lastTexId)
	{
		//tMed->update( actUplTexId );
  	  	//flow->update( tMed->getResTexId(), tMed->getLastResId(), getOscPar("flowFdbk") );
  	  	//lastTexId = actUplTexId;
	}

	// copy opticalflow processed frame
	if (optFrame != optLastFrame)
	{
		mutex.lock();

		cv::cuda::GpuMat gpu_mat = ogl_buf[cvOptPtr].mapDevice();
		d_flow.copyTo(gpu_mat);
		ogl_buf[cvOptPtr].unmapDevice();

		optLastFrame = optFrame;
		mutex.unlock();
	}

	medFbo->bind();
	medFbo->clearAlpha(0.01f, 0.f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	displOptFlow->begin();
	displOptFlow->setUniform1i("width", vt->getWidth() / downscale);
	displOptFlow->setUniform1i("height", vt->getHeight() / downscale);

    glBindVertexArray(VAOId[cvOptPtr]);               // create VAO, assign the name and bind that array
    ogl_buf[cvOptPtr].bind(cv::ogl::Buffer::ARRAY_BUFFER);
    glDrawArrays(GL_POINTS, 0, vt->getWidth() * vt->getHeight() / downscale);

    medFbo->unbind();

	tMed->update( medFbo->getColorImg() );


	//

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
//	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniformMatrix4fv("m_pvm", vt->getTransMatPtr());

	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", getOscPar("alpha"));

	glActiveTexture(GL_TEXTURE0);
	//vt->bindActFrame(0);
//	glBindTexture(GL_TEXTURE_2D, flow->getResTexId());
	glBindTexture(GL_TEXTURE_2D, tMed->getResTexId());
//	glBindTexture(GL_TEXTURE_2D, actUplTexId);

	quad->draw(_tfo);

	_shader->begin();
#endif
}

//----------------------------------------------------

void SNTrustOpticalFlow::update(double time, double dt)
{
#ifdef HAVE_CUDA
	tMed->setMedian(getOscPar("median"));
	vt->updateDt(time, false);
	actUplTexId = vt->loadFrameToTexture();

	// brox optical flow
    // if there is a new frame copy it to an opengl array buffer
	if(!optProcessing && actUplTexId != 0 && actUplTexId != lastOptTexId)
    	boost::thread m_Thread = boost::thread(&SNTrustOpticalFlow::processQueue, this);
#endif
}

//----------------------------------------------------

#ifdef HAVE_CUDA
  
void SNTrustOpticalFlow::processQueue()
{
  
	optProcessing = true;

	cv::resize( *(vt->getActMat()), size_frame, cv::Size(vt->getWidth() /downscale, vt->getHeight() /downscale) );
	cv::cvtColor ( size_frame, gray_frame, CV_BGR2GRAY );

	d_frame[cvOptPtr] = cv::cuda::GpuMat(gray_frame);
	d_frame[cvOptPtr].convertTo(d_framef[cvOptPtr], CV_32F, 1.0 / 255.0);

	if (!cvOptInit && cvOptPtr == 1) cvOptInit = true;
	if(cvOptInit)
	{
		mutex.lock();

		brox->calc(d_framef[cvOptPtr], d_framef[(cvOptPtr +1) % 2], d_flow);
	    //lk->calc( d_frame[cvOptPtr], d_frame[(cvOptPtr +1) % 2], d_flow);
	    //farn->calc(d_frame[cvOptPtr], d_frame[(cvOptPtr +1) % 2], d_flow);
	    //tvl1->calc(d_frame[cvOptPtr], d_frame[(cvOptPtr +1) % 2], d_flow);

		optFrame++;
		mutex.unlock();
	}

	cvOptPtr = (cvOptPtr +1)  %2;
	lastOptTexId = actUplTexId;

	optProcessing = false;
}

#endif
  
//----------------------------------------------------

SNTrustOpticalFlow::~SNTrustOpticalFlow()
{
	delete quad;
}

}
