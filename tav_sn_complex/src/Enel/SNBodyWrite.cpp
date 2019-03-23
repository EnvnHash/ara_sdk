/*
 * SNBodyWrite.cpp
 *
 *  Created on: 18.10.2017
 *      Author: sven
 */

#include "SNBodyWrite.h"

#define STRINGIFY(A) #A

namespace tav
{

SNBodyWrite::SNBodyWrite(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs),
		actDrawMode(DRAW),  // RAW_DEPTH, DEPTH_THRESH, DRAW
		depthThresh(2135.f),
		fastBlurAlpha(0.95f),
		noActivityThresh(400.f),
		recordThresh(0.9f),
		secondThres(0.71f),
		resetSnapShotTime(2.0),
		yPos(0.347f),
		trig(0.f),
		clear(0.f)
{

	kin = static_cast<KinectInput*>(_scd->kin);
	//kinRepro = static_cast<KinectReproTools*>(_scd->kinRepro);

	winMan = static_cast<GWindowManager*>(_scd->winMan);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// get onKey function
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	//--------------------------------------------------------------------------------

	rawQuad = _scd->stdQuad;
	rotateQuad = _scd->stdHFlipQuad;

	//--------------------------------------------------------------------------------

	texShader = shCol->getStdTex();
	texAlphaShader = shCol->getStdTexAlpha();

	//--------------------------------------------------------------------------------

	threshFbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
						GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	thresh2Fbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
						GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	diffFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, GL_R8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	recFbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
					GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

	fblur = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(), kin->getDepthHeight());
	fblur2nd = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(), kin->getDepthHeight());

	//--------------------------------------------------------------------------------

	histo = new GLSLHistogram(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
			GL_R8, 2, 128, true, false, 1.f);


	//--------------------------------------------------------------------------------

	// words
	nrWords = 3;
	wordPtr = 0;
	charPtr = 0;

	maxNrSnapShotFbos = 5;

	std::string words[nrWords][maxNrSnapShotFbos] = {
			{ "P", "O", "E", "M", "A"},
			{ "L", "I", "B", "R", "O"},
			{ "V", "I", "V", "I", "V"}
	};


	for (unsigned int i=0;i<nrWords;i++)
	{
		ft.push_back( std::vector<FreetypeTex*>() );

		for (unsigned int j=0; j<maxNrSnapShotFbos; j++)
		{
			ft[i].push_back( new FreetypeTex(((*scd->dataPath)+"/fonts/Arial.ttf").c_str(), 180) );
			ft[i].back()->setText( words[i][j] );
		}
	}


	snapPicWidth = 2.f / static_cast<float>(maxNrSnapShotFbos);
	snapShotPtr = 0;
	snapShotFbos = new FBO*[maxNrSnapShotFbos];
	for (unsigned int i=0; i<maxNrSnapShotFbos; i++)
		snapShotFbos[i] = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
			GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);


	//--------------------------------------------------------------------------------

	nrEnelColors = 8;
	enel_colors = new glm::vec4[nrEnelColors];
	enel_colors[0] = glm::vec4(0.9f, 0.078f, 0.f, 1.f);			// rot
	enel_colors[1] = glm::vec4(1.f, 0.059f, 0.39f, 1.f);		// hell rot
	enel_colors[2] = glm::vec4(0.0196f, 0.3333f, 0.98f, 1.f);	// blau
	enel_colors[3] = glm::vec4(0.f, 0.549f, 0.353f, 1.f);		// dunkel grün
	enel_colors[4] = glm::vec4(1.f, 0.353f, 0.059f, 1.f);		// orange
	enel_colors[5] = glm::vec4(1.f, 0.2745f, 0.53f, 1.f);		// rosa
	enel_colors[6] = glm::vec4(0.255f, 0.725f, 0.9f, 1.f);		// hellblau
	enel_colors[7] = glm::vec4(0.333f, 0.745f, 0.353f, 1.f);	// hellgrün

	enelColPtr = 0;

	//--------------------------------------------------------------------------------

	addPar("depthThresh", &depthThresh); // 10.f, 5000.f, 1.f, 1755.f, OSCData::LIN);
	addPar("fastBlurAlpha", &fastBlurAlpha); // 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
	addPar("noActivityThresh", &noActivityThresh); // 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
	addPar("recordThresh", &recordThresh); // 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
	addPar("secondThres", &secondThres);

	addPar("snapYPos", &yPos);

	addPar("trig", &trig);
	addPar("clear", &clear);

	//--------------------------------------------------------------------------------

	initDepthThresh();
	initDepthThresh2();
	initSilShdr();
	initSnapShotShdr();
	initDiffShdr();

	energySum = new Median<float>(2.f);
}

//---------------------------------------------------------------

void SNBodyWrite::initDepthThresh()
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

	stdVert = "// SNEnelFluid depth threshold vertex shader\n" +shdr_Header +stdVert;
	std::string frag = STRINGIFY(uniform sampler2D kinDepthTex; // 16 bit -> float
								 uniform float depthThres;
								 in vec2 tex_coord;
								 layout (location = 0) out vec4 color;
								 float outVal;
								 void main()
								 {
									 outVal = texture(kinDepthTex, tex_coord).r;
									 outVal = outVal > 100.0 ? (outVal < depthThres ? 1.0 : 0.0) : 0.0;
									 color = vec4(outVal);
								 });

	frag = "// SNEnelFluid depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres = shCol->addCheckShaderText("SNBodyWrite_thres", stdVert.c_str(), frag.c_str());

}

//---------------------------------------------------------------

void SNBodyWrite::initDepthThresh2()
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

	stdVert = "// SNEnelFluid depth threshold vertex shader\n" +shdr_Header +stdVert;
	std::string frag = STRINGIFY(uniform sampler2D tex; // R8
								 uniform float thres;
								 in vec2 tex_coord;
								 layout (location = 0) out vec4 color;
								 float outVal;
								 void main()
								 {
									 outVal = texture(tex, tex_coord).r;
									 outVal = outVal > thres ? 1.0 : 0.0;
									 color = vec4(outVal);
								 });

	frag = "// SNEnelFluid depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres2 = shCol->addCheckShaderText("SNBodyWrite_thres2", stdVert.c_str(), frag.c_str());

}

//---------------------------------------------------------------

void SNBodyWrite::initSilShdr()
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

	stdVert = "// SNEnelFluid depth threshold vertex shader\n" +shdr_Header +stdVert;
	std::string frag = STRINGIFY(uniform sampler2D tex; // R8
								 uniform vec4 silCol;
								 in vec2 tex_coord;
								 layout (location = 0) out vec4 color;
								 void main()
								 {
									 vec4 outVal = texture(tex, tex_coord);
									 float bright = outVal.r;
									 color = vec4( vec3(silCol) * bright, 1.0 );
								 });

	frag = "// SNEnelFluid depth threshold fragment shader\n"+shdr_Header+frag;

	silShdr = shCol->addCheckShaderText("SNBodyWrite_sill", stdVert.c_str(), frag.c_str());

}

//---------------------------------------------------------------

void SNBodyWrite::initSnapShotShdr()
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
									 tex_coord.x *= 0.5;
									 tex_coord.x += 0.5;
									 gl_Position = m_pvm * position;
								});

	stdVert = "// SNEnelFluid depth threshold vertex shader\n" +shdr_Header +stdVert;
	std::string frag = STRINGIFY(uniform sampler2D tex; // R8
								 uniform vec4 silCol;
								 in vec2 tex_coord;
								 layout (location = 0) out vec4 color;
								 void main()
								 {
									 vec4 outVal = texture(tex, tex_coord);
									 float bright = outVal.r;
									 color = vec4( silCol.rgb * bright, 1.0 );
								 });

	frag = "// SNEnelFluid depth threshold fragment shader\n"+shdr_Header+frag;

	snapShotShdr = shCol->addCheckShaderText("SNBodyWrite_snap", stdVert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNBodyWrite::initDiffShdr()
{
	std::string shdr_Header = "#version 410\n";

	std::string vert = STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			uniform mat4 m_pvm;\n
			out vec2 tex_coord;\n
			void main(){\n
				tex_coord = texCoord;\n
				gl_Position = m_pvm * position;\n
			});

	vert = "//SNBodyWrite diff vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform sampler2D refTex;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			vec4 refCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				refCol = texture(refTex, tex_coord);\n
				color = refCol - outCol;\n
				color.a = 1.0;\n
			});

	frag = "// SNBodyWrite diff frag\n"+shdr_Header+frag;

	diffShader = shCol->addCheckShaderText("SNBodyWrite_diff", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNBodyWrite::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
		switch(actDrawMode)
		{
			case RAW_DEPTH : // 1
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
//				glBindTexture(GL_TEXTURE_2D, transTexId);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
				rawQuad->draw();
				break;

			case DEPTH_THRESH: // 2
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());
				rawQuad->draw();
				break;

			case DEPTH_BLUR: // 3
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
				rawQuad->draw();
				break;

			case DIFF: // 4
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, diffFbo->getColorImg());
				rawQuad->draw();
				break;

			case DRAW: // 5

				glEnable(GL_BLEND);
				glClear(GL_COLOR_BUFFER_BIT);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE);


				if ( charPtr < maxNrSnapShotFbos )
				{
					// draw letter
					glm::mat4 lMat = glm::translate(glm::vec3(-0.5f, -0.2f, 0.f))
						* glm::scale(glm::vec3(0.35f, 0.75f, 1.f));

					texShader->begin();
					texShader->setUniformMatrix4fv("m_pvm", &lMat[0][0]);
					texShader->setUniform1i("tex", 0);
					ft[wordPtr][charPtr]->bind(0);
					rotateQuad->draw();
				}



				// draw the raw silhouette with feedback and colors
				silShdr->begin();
				silShdr->setIdentMatrix4fv("m_pvm");
				silShdr->setUniform1i("tex", 0);
				silShdr->setUniform4fv("silCol", &enel_colors[enelColPtr][0]);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
				rawQuad->draw();
				//rotateQuad->draw();



				/*
				// draw the result of all snapshots
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, recFbo->getColorImg());
				rawQuad->draw();
*/

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


				// draw snaps
				// draw the result of all snapshots
				texShader->begin();
				texShader->setUniform1i("tex", 0);

				for (unsigned int i=0; i<nrActiveSnapshots; i++)
				{
					//float totWidth = snapPicWidth * static_cast<float>(nrActiveSnapshots);
					float leftOffs = snapPicWidth * static_cast<float>(nrActiveSnapshots -1) * -0.5f;
					float xPos = leftOffs + static_cast<float>(i) * snapPicWidth;

					glm::mat4 tMat = glm::translate( glm::vec3(xPos, yPos, 0.f) )
							* glm::scale( glm::vec3(snapPicWidth * 0.5f, 0.2f, 1.f) );

					texShader->setUniformMatrix4fv("m_pvm", &tMat[0][0]);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, snapShotFbos[i]->getColorImg());
					rawQuad->draw();
					//rotateQuad->draw();

				}

				break;
		}
}

//---------------------------------------------------------------

void SNBodyWrite::update(double time, double dt)
{
	if (kin && kin->isReady())
	{
		if (!inited)
		{
			kin->setCloseRange(true);
			kin->setDepthVMirror(true);
			inited = true;

		} else
		{

			//--- Proc Depth Image and Update the Fluid Simulator --
			// upload depth image, wird intern gecheckt, ob neues Bild erforderlich oder nicht
			//if (kin->getDepthFrameNr(0) != lastDepthFrame)

			kin->uploadDepthImg(false);

			glDisable(GL_BLEND);

			threshFbo->bind();
			threshFbo->clear();

			depthThres->begin();
			depthThres->setIdentMatrix4fv("m_pvm");
			depthThres->setUniform1i("kinDepthTex", 0);
			depthThres->setUniform1f("depthThres", depthThresh);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

			rawQuad->draw();
			threshFbo->unbind();

			// ----------------------------------

			// 2nd threshold
			thresh2Fbo->bind();
			thresh2Fbo->clear();

			depthThres2->begin();
			depthThres2->setIdentMatrix4fv("m_pvm");
			depthThres2->setUniform1i("tex", 0);
			depthThres2->setUniform1f("thres", secondThres);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());
//			glBindTexture(GL_TEXTURE_2D, transTexId[1]);

			rawQuad->draw();
			thresh2Fbo->unbind();


			// ------ apply blur on silhoutte --------

			fblur->setAlpha(fastBlurAlpha);
			fblur->proc(thresh2Fbo->getColorImg());

			fblur2nd->proc(fblur->getResult());

			// ----------------------------------


			// check if there is someone inside the scene
			// if this is the case the amount of white must be above a threshold
			histo->proc(fblur2nd->getResult());
			gotActivity = noActivityThresh < histo->getEnergySum(0.1f);

			//std::cout << histo->getEnergySum(0.1f) << std::endl;

			if (!gotActivity){

				//std::cout << "no Activity" << std::endl;
				startRec = false;

			} else {

				//std::cout << "got Activity" << std::endl;

				// if we got activity calculate the threshold to the last kinect frame
				diffFbo->bind();
				diffFbo->clear();

				glBlendFunc(GL_ONE, GL_ZERO);

				diffShader->begin();
				diffShader->setIdentMatrix4fv("m_pvm");
				diffShader->setUniform1i("tex", 0);
				diffShader->setUniform1i("refTex", 1);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getLastResult());

				rawQuad->draw();

				diffFbo->unbind();



				// measure amount of white in the diff image
				histo->proc(diffFbo->getColorImg());

				energySum->update( float( histo->getEnergySum(0.1f) ) );
				//std::cout << "recordThresh: " << recordThresh << " getEnergySum: " << energySum->get() << std::endl;

				if (recordThresh > energySum->get() || manSnap)
				{
					//std::cout << "above record threshold " << std::endl;

					if(permitRecord)
					{
						if (!startRec && !manSnap)
						{
							std::cout << ">>>>>>>>>>> start rec trigger taking record time" << std::endl;

							startRec = true;
							startRecTime = time;
							tookSnapShot = false;


						} else if((time - startRecTime > 3.5 && !tookSnapShot) || manSnap)
						{
							std::cout << "*********** taking snapshot" << std::endl;

							tookSnapShot = true;
							tookSnapShotTime = time;

							manSnap = false;


							// snap
							if (nrActiveSnapshots < maxNrSnapShotFbos)
							{
								std::cout << "record to " << snapShotPtr << std::endl;

								snapShotFbos[snapShotPtr]->bind();

								snapShotShdr->begin();
								snapShotShdr->setIdentMatrix4fv("m_pvm");
								snapShotShdr->setUniform1i("tex", 0);
								snapShotShdr->setUniform4fv("silCol", &enel_colors[enelColPtr][0]);

								glActiveTexture(GL_TEXTURE0);
								glBindTexture(GL_TEXTURE_2D, fblur->getResult());
								rawQuad->draw();

								snapShotFbos[snapShotPtr]->unbind();

								snapShotPtr = ++snapShotPtr % maxNrSnapShotFbos;
								charPtr++;
								nrActiveSnapshots++;

								std::cout << "nrActiveSnapshots " << nrActiveSnapshots << std::endl;
								std::cout << "charPtr " << charPtr << std::endl;
								std::cout << "snapShotPtr " << snapShotPtr << std::endl;

							}


							/*
							recFbo->bind();

							snapShotShdr->begin();
							snapShotShdr->setIdentMatrix4fv("m_pvm");
							snapShotShdr->setUniform1i("tex", 0);
							snapShotShdr->setUniform4fv("silCol", &enel_colors[enelColPtr][0]);

							glActiveTexture(GL_TEXTURE0);
							glBindTexture(GL_TEXTURE_2D, fblur->getResult());
							rawQuad->draw();

							recFbo->unbind();
							*/

							enelColPtr = ++enelColPtr % nrEnelColors;

						} else if(tookSnapShot && (time - tookSnapShotTime > resetSnapShotTime))
						{

							std::cout << "<<<<<<<<<<<<<<  permit new record" << std::endl;
							permitRecord = true;

							startRec = false;
							tookSnapShot = false;
						}
					}

				} else
				{
					std::cout << "<<<<<<<<<<<<<<  permit new record" << std::endl;
					permitRecord = true;

					startRec = false;
					tookSnapShot = false;
				}
			}
		}
	}


	if (trig == 1.f)
	{
		std::cout << "body trig" << std::endl;
		manSnap = true;
		permitRecord = true;
		trig = 0.f;
	}

	if (clear == 1.f)
	{
		std::cout << "body clear" << std::endl;

		recFbo->bind();
		glClear(GL_COLOR_BUFFER_BIT);
		recFbo->unbind();

		nrActiveSnapshots = 0;
		snapShotPtr = 0;
		wordPtr = ++wordPtr % nrWords;
		charPtr = 0;

		// clear fbos
		for (unsigned int i=0; i<maxNrSnapShotFbos; i++)
		{
			snapShotFbos[i]->bind();
			snapShotFbos[i]->clear();
			snapShotFbos[i]->unbind();
		}



		printf(" CLEARING RECORDS \n");

		clear = 0.f;
	}
}

//---------------------------------------------------------------

void SNBodyWrite::onKey(int key, int scancode, int action, int mods)
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
			case GLFW_KEY_4 : actDrawMode = DIFF;
				printf("actDrawMode = DIFF \n");
				break;
			case GLFW_KEY_5 : actDrawMode = DRAW;
				printf("actDrawMode = DRAW \n");
				break;

			case GLFW_KEY_C :
				recFbo->bind();
				glClear(GL_COLOR_BUFFER_BIT);
				recFbo->unbind();

				nrActiveSnapshots = 0;
				snapShotPtr = 0;
				wordPtr = ++wordPtr % nrWords;
				charPtr = 0;

				// clear fbos
				for (unsigned int i=0; i<maxNrSnapShotFbos; i++)
				{
					snapShotFbos[i]->bind();
					snapShotFbos[i]->clear();
					snapShotFbos[i]->unbind();
				}



				printf(" CLEARING RECORDS \n");

				break;

			case GLFW_KEY_T :
				permitRecord = true;
				printf(" man snap\n");
				manSnap = true;
				break;

		}
	}
}

//---------------------------------------------------------------

SNBodyWrite::~SNBodyWrite()
{
	// TODO Auto-generated destructor stub
}

} /* namespace tav */
