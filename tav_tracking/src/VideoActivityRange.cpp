/*
 * VideoActivityRange.cpp
 *
 *  Created on: 25.01.2017
 *      Copyright by Sven Hahne
 */

#include "VideoActivityRange.h"

namespace tav
{

VideoActivityRange::VideoActivityRange(ShaderCollector* _shCol,
		unsigned int _width, unsigned int _height, float _dstRatio,
		float _minSize) :
		width(_width), height(_height), minSize(_minSize), dstRatio(_dstRatio)
{
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f, nullptr, 1, true);

	rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f, nullptr, 1, false);

	ctrlQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 1.f, 0.15f);

	centQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f,
			0.f, 0.f, 0.3f);

	tMed = new GLSLTimeMedian(_shCol, width, height, GL_RGBA8);
	medFbo = new FBO(_shCol, width, height, 1);
	colFbo = new FBO(_shCol, width, height, 1);
	colFbo->setMagFilter(GL_NEAREST);
	colFbo->setMinFilter(GL_NEAREST);

	blur = new FastBlurMem(0.48f, _shCol, width, height, GL_RGBA8);
	histo = new GLSLHistogram(_shCol, width, height, GL_RGBA8, 1, 256, true,
			true);

	kPos = new CvKalmanFilter(4, 2, 0.0000001f);
	kSize = new CvKalmanFilter(4, 2, 0.0000001f);
	kCent = new CvKalmanFilter(4, 2, 0.0000001f);

	quadSize = new Median<glm::vec2>(45.f);
	quadPos = new Median<glm::vec2>(35.f);
	energCentMed = new Median<glm::vec2>(45.f);
	dstQuadSize = new Median<glm::vec2>(40.f);
	//  dstQuadPos = new Median<glm::vec2>(10.f)
	dstQuadPos = glm::vec2(0.f);

	getGlError();

	stdTex = _shCol->getStdTex();
	stdCol = _shCol->getStdCol();

	initDiffShdr(_shCol);
	initPosColShdr(_shCol);
	initHistoDebug(_shCol);
}

//-------------------------------------------

void VideoActivityRange::update(GLint actTexId, GLint lastTexId)
{
	medFbo->bind();
	medFbo->clear();

	diffThres->begin();
	diffThres->setIdentMatrix4fv("m_pvm");
	diffThres->setUniform1i("tex", 0);
	diffThres->setUniform1i("lastTex", 1);
	diffThres->setUniform1f("thres", thres);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, actTexId);

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lastTexId);

	rawQuad->draw();

	medFbo->unbind();

	//-------------------------------------------

	tMed->setMedian(timeMedian);
	tMed->update(medFbo->getColorImg());

	//-------------------------------------------
	// blur

	blur->proc(tMed->getResTexId());

	//-------------------------------------------

	colFbo->bind();
	colFbo->clear();

	posColShdr->begin();
	posColShdr->setIdentMatrix4fv("m_pvm");
	posColShdr->setUniform1i("tex", 0);
	posColShdr->setUniform1f("thres", posColThres);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blur->getResult());

	rawQuad->draw();

	colFbo->unbind();

	//-------------------------------------------

	histo->setValThres(histoValThres);
	histo->setIndValThres(indValThres);
	histo->setSmoothing(histoSmooth);
	histo->proc(colFbo->getColorImg());

	updtTexId = actTexId;
}

//----------------------------------------------------

void VideoActivityRange::updateMat()
{
	bool updt = false;

	if (updtTexId != lastUpdtTexId)
	{
		updtTexId = lastUpdtTexId;
		updt = true;
	}

	actQuadSize = glm::vec2(histo->getMaxInd(0) - histo->getMinInd(0),
			histo->getMaxInd(1) - histo->getMinInd(1));
	actQuadPos = glm::vec2(actQuadSize.x * 0.5f + histo->getMinInd(0),
			actQuadSize.y * 0.5f + histo->getMinInd(1));

	quadPos->update(actQuadPos);
	quadSize->update(actQuadSize);

	kPos->predict();
	kSize->predict();
	kCent->predict();

	if (updt)
	{
		kSize->update(quadSize->get().x, quadSize->get().y);
		kPos->update(quadPos->get().x, quadPos->get().y);
		//kCent->update(energCentMed->get().x, energCentMed->get().y);
		kCent->update(histo->getHistoPeakInd(0), histo->getHistoPeakInd(1));
	}

	energCent = glm::vec2(kPos->get(0), kPos->get(1));
//	energCent = glm::vec2((kPos->get(0) * 2.f + kCent->get(0)) / 3.f,
//			(kPos->get(1) * 2.f + kCent->get(1)) / 3.f);
	energCentMed->update(energCent);

	float zoomQuadRatio = kSize->get(0) / kSize->get(1);
	glm::vec2 dstSize;

	if (dstRatio > zoomQuadRatio)
	{
		// wenn zoom quad hoeher ist als ziel ratio
		// beschneide oben und unten mit energCenter als Mittelpunkt
		dstSize = glm::vec2(kSize->get(0), kSize->get(0) / dstRatio);

	}
	else
	{
		// wenn zoom quad breiter ist als ziel ratio
		// beschneide links und rechts mit energCenter als Mittelpunkt
		dstSize = glm::vec2(kSize->get(1) * dstRatio, kSize->get(1));
	}

	// calculate size difference
	float spaceDiff = kSize->get(0) * kSize->get(1);
	spaceDiff = std::fabs(spaceDiff - dstSize.x * dstSize.y) / spaceDiff;

	// adjust scaling
	dstSize = dstSize / ((1.f - spaceDiff) * 0.5f + spaceDiff);
	// limit values
	for (int i = 0; i < 2; i++)
		dstSize[i] = std::fmin(std::fmax(dstSize[i], minSize), 1.f);

	dstQuadSize->update(dstSize);

	// adjust position
	glm::vec2 normPos = energCentMed->get() * 2.f - 1.f;

	// check if the resulting quad hits the boundaries
	float rightLim = -std::fmax(normPos.x + dstQuadSize->get().x, 1.f) + 1.f;
	float leftLim = -1.f - std::fmin(normPos.x - dstQuadSize->get().x, -1.f);
	float upperLim = -std::fmax(normPos.y + dstQuadSize->get().y, 1.f) + 1.f;
	float lowerLim = -1.f - std::fmin(normPos.y - dstQuadSize->get().y, -1.f);

	dstQuadPos = glm::vec2(normPos.x + rightLim + leftLim,
			normPos.y + upperLim + lowerLim);
//	dstQuadPos->update( glm::vec2(normPos.x +rightLim + leftLim,
//								  normPos.y +upperLim + lowerLim ));
}

//----------------------------------------------------

void VideoActivityRange::drawPosCol()
{
	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colFbo->getColorImg());

	rawQuad->draw();
}

//----------------------------------------------------

void VideoActivityRange::drawHistogram()
{
	histoTex->begin();
	histoTex->setIdentMatrix4fv("m_pvm");
	histoTex->setUniform1i("tex", 0);
	histoTex->setUniform1f("nrChan", 3);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, histo->getResult());

	quad->draw();
}

//----------------------------------------------------

void VideoActivityRange::drawActivityQuad()
{
	ctrlQuadMat = glm::translate(glm::mat4(1.f),
			glm::vec3(dstQuadPos.x, dstQuadPos.y, 0.f))
			* glm::scale(glm::mat4(1.f),
					glm::vec3(dstQuadSize->get().x, dstQuadSize->get().y, 1.f));
//	ctrlQuadMat = glm::translate(glm::mat4(1.f), glm::vec3(dstQuadPos->get().x, dstQuadPos->get().y, 0.f) )
//		* glm::scale(glm::mat4(1.f), glm::vec3(dstQuadSize->get().x, dstQuadSize->get().y, 1.f) );

	// draw ctrl quad
	stdCol->begin();
	stdCol->setUniformMatrix4fv("m_pvm", &ctrlQuadMat[0][0]);

	ctrlQuad->draw();

	//-------------------------------------------

//	glm::mat4 centQuadMat = glm::translate(glm::mat4(1.f), glm::vec3(dstQuadPos->get().x , dstQuadPos->get().y, 0.f) )
//		* glm::scale(glm::mat4(1.f), glm::vec3(0.05f, 0.05f, 1.f) );
	glm::mat4 centQuadMat = glm::translate(glm::mat4(1.f),
			glm::vec3(dstQuadPos.x, dstQuadPos.y, 0.f))
			* glm::scale(glm::mat4(1.f), glm::vec3(0.05f, 0.05f, 1.f));
	stdCol->begin();
	stdCol->setUniformMatrix4fv("m_pvm", &centQuadMat[0][0]);

	centQuad->draw();
}

//----------------------------------------------------

void VideoActivityRange::initDiffShdr(ShaderCollector* _shCol)
{
	//------ Position Shader -----------------------------------------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string
	vert = STRINGIFY(layout (location=0) in vec4 position;\n layout(location =
					1)
			in vec3
			normal;
			\n
			layout(location = 2)
			in vec2
			texCoord;
			\n layout(location = 3)
			in vec4
			color;
			\n out
			vec2 tex_coord;
			void main()
			{	\n
				tex_coord = texCoord;
				gl_Position = position;\n
			});
			vert = "// VideoActivityRange pos tex vertex shader\n" + shdr_Header
					+ vert;

			//------ Frag Shader -----------------------------------------

			shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

			std::string
			frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n uniform
					sampler2D tex;
					uniform sampler2D
					lastTex;
					in vec2
					tex_coord;
					vec4 act;
					vec4 last;
					vec3 diff;
					uniform
					float thres;
					void main()
					\n
					{	\n
						act = texture(tex, tex_coord);
						last = texture(lastTex, tex_coord);
						diff = abs(last.rgb - act.rgb);
						float thresC = diff.r + diff.g + diff.b;
						thresC *= thresC;
						thresC = thresC > thres ? 1.0 : 0.0;
						fragColor = vec4(thresC, thresC, thresC, 1.0);
					});
					frag = "// VideoActivityRange pos tex shader\n"
							+ shdr_Header + frag;

					diffThres = _shCol->addCheckShaderText(
							"VideoActivityRange_diff_thres", vert.c_str(),
							frag.c_str());
				}

//----------------------------------------------------

				void VideoActivityRange::initPosColShdr(ShaderCollector* _shCol)
				{
					//------ Position Shader -----------------------------------------

					std::string shdr_Header =
							"#version 410 core\n#pragma optimize(on)\n";

					std::string
					vert = STRINGIFY(layout (location=0) in vec4 position;\n layout(
									location = 1)
							in vec3
							normal;
							\n
							layout(location = 2)
							in vec2
							texCoord;
							\n layout(location = 3)
							in vec4
							color;
							\n out
							vec2 tex_coord;
							void main()
							{	\n
								tex_coord = texCoord;
								gl_Position = position;\n
							});
							vert =
									"// VideoActivityRange pos tex vertex shader\n"
											+ shdr_Header + vert;

							//------ Frag Shader -----------------------------------------

							shdr_Header =
									"#version 410 core\n#pragma optimize(on)\n";

							std::string
							frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n uniform
									float thres;
									uniform sampler2D
									tex;
									in vec2
									tex_coord;
									void main()
									\n
									{	\n
										float lum = texture(tex, tex_coord).r;
										if (lum > thres)
										{
											fragColor = vec4(tex_coord.x, tex_coord.y, 0.0, 1.0);
										}
										else
										{
											discard;
										}
									});
									frag =
											"// VideoActivityRange pos tex shader\n"
													+ shdr_Header + frag;

									posColShdr = _shCol->addCheckShaderText(
											"VideoActivityRange_pos_col",
											vert.c_str(), frag.c_str());
								}

//----------------------------------------------------

								void VideoActivityRange::initHistoDebug(
										ShaderCollector* _shCol)
								{
									//------ Position Shader -----------------------------------------

									std::string shdr_Header =
											"#version 410 core\n#pragma optimize(on)\n";

									std::string
									vert = STRINGIFY(layout (location=0) in vec4 position;\n layout(
													location = 1)
											in vec3
											normal;
											\n
											layout(location = 2)
											in vec2
											texCoord;
											\n layout(location = 3)
											in vec4
											color;
											\n out
											vec2 tex_coord;
											void main()
											{	\n
												tex_coord = texCoord;
												gl_Position = position;\n
											});
											vert =
													"// VideoActivityRange histo tex vertex shader\n"
															+ shdr_Header
															+ vert;

											//------ Frag Shader -----------------------------------------

											shdr_Header =
													"#version 410 core\n#pragma optimize(on)\n";

											std::string
											frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n uniform
													float nrChan;
													uniform sampler2D
													tex;
													in vec2
													tex_coord;
													void main()
													\n
													{	\n
														//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
														vec4 amp = texture(tex, tex_coord) / 10.0;
														vec3 col = vec3(
																float( tex_coord.y < (nrChan - 2.0) / nrChan ),
																tex_coord.y > (nrChan - 2.0) / nrChan ? (tex_coord.y < (nrChan - 1.0) / nrChan ? 1.0 : 0.0) : 0.0,
																(tex_coord.y > (nrChan - 1.0) / nrChan) ? 1.0 : 0.0
														);
														fragColor = vec4(col * amp.r, 1.0);
													});
													frag =
															"// VideoActivityRange histo tex shader\n"
																	+ shdr_Header
																	+ frag;

													histoTex =
															_shCol->addCheckShaderText(
																	"VideoActivityRange_histo",
																	vert.c_str(),
																	frag.c_str());
												}

//----------------------------------------------------

												void VideoActivityRange::setTimeMedian(
														float _val)
												{
													timeMedian = _val;
												}

//----------------------------------------------------

												void VideoActivityRange::setThres(
														float _val)
												{
													thres = _val;
												}

//----------------------------------------------------

												void VideoActivityRange::setPosColThres(
														float _val)
												{
													posColThres = _val;
												}

//----------------------------------------------------

												void VideoActivityRange::setHistoValThres(
														float _val)
												{
													histoValThres = _val;
												}

//----------------------------------------------------

												void VideoActivityRange::setIndValThres(
														float _val)
												{
													indValThres = _val;
												}

//----------------------------------------------------

												void VideoActivityRange::setHistoSmooth(
														float _val)
												{
													histoSmooth = _val;
												}

//----------------------------------------------------

												GLint VideoActivityRange::getPositionToColorTex()
												{
													return colFbo->getColorImg();
												}

//----------------------------------------------------

												GLint VideoActivityRange::getHistoTex()
												{
													return histo->getResult();
												}

//----------------------------------------------------

												glm::mat4* VideoActivityRange::getTransMat()
												{
													ctrlQuadMat =
															glm::translate(
																	glm::mat4(
																			1.f),
																	glm::vec3(
																			dstQuadPos.x,
																			dstQuadPos.y,
																			0.f))
																	* glm::scale(
																			glm::mat4(
																					1.f),
																			glm::vec3(
																					dstQuadSize->get().x,
																					dstQuadSize->get().y,
																					1.f));
													return &ctrlQuadMat;
												}

//----------------------------------------------------

												glm::mat4* VideoActivityRange::getInvTransMat()
												{
													invQuadMat =
															glm::translate(
																	glm::mat4(
																			1.f),
																	glm::vec3(
																			-dstQuadPos.x,
																			-dstQuadPos.y,
																			0.f))
																	* glm::scale(
																			glm::mat4(
																					1.f),
																			glm::vec3(
																					1.f
																							/ dstQuadSize->get().x,
																					1.f
																							/ dstQuadSize->get().y,
																					1.f));
//	invQuadMat = glm::translate(glm::mat4(1.f), glm::vec3(-dstQuadPos->get().x, -dstQuadPos->get().y, 0.f) )
//		* glm::scale(glm::mat4(1.f), glm::vec3(1.f / dstQuadSize->get().x, 1.f / dstQuadSize->get().y, 1.f) );

													return &invQuadMat;
												}

//----------------------------------------------------

												glm::mat4* VideoActivityRange::getInvTransFlipHMat()
												{
													invQuadMatFlipH =
															glm::translate(
																	glm::mat4(
																			1.f),
																	glm::vec3(
																			-dstQuadPos.x,
																			dstQuadPos.y,
																			0.f))
																	* glm::scale(
																			glm::mat4(
																					1.f),
																			glm::vec3(
																					1.f
																							/ dstQuadSize->get().x,
																					1.f
																							/ dstQuadSize->get().y,
																					1.f));
//	invQuadMat = glm::translate(glm::mat4(1.f), glm::vec3(-dstQuadPos->get().x, -dstQuadPos->get().y, 0.f) )
//		* glm::scale(glm::mat4(1.f), glm::vec3(1.f / dstQuadSize->get().x, 1.f / dstQuadSize->get().y, 1.f) );
													return &invQuadMatFlipH;
												}

//----------------------------------------------------

												VideoActivityRange::~VideoActivityRange()
												{
													delete quad;
													delete rawQuad;

													delete ctrlQuad;

													delete centQuad;

													delete tMed;
													delete medFbo;
													delete colFbo;

													delete blur;
													delete histo;

													delete kPos;
													delete kSize;
													delete kCent;

													delete quadSize;
													delete quadPos;
													delete energCentMed;
													delete dstQuadSize;
													//delete dstQuadPos;
												}

												} /* namespace tav */
