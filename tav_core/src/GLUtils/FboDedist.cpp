//
//  FboDedist.cpp
//  tav_core
//
//  Created by Sven Hahne on 22/12/15.
//  Copyright © 2015 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "pch.h"
#include "FboDedist.h"

namespace tav
{
FboDedist::FboDedist(sceneData* _scd, GWindowManager* _winMan) :
		scd(_scd), winMan(_winMan), cornersSize(10), actBoundFbo(-1),
		showCorners(false), newCornerVals(false)
{
	cornersNames.push_back("bl");
	cornersNames.push_back("br");
	cornersNames.push_back("tr");
	cornersNames.push_back("tl");

	fboQuad = new QuadArray(24, 24, -1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f));

	cornerQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f,
			0.f, 0.f, 1.f);

	quadSkel[0] = 0.f;
	quadSkel[1] = 0.f;
	quadSkel[2] = 1.f;
	quadSkel[3] = 0.f;
	quadSkel[4] = 1.f;
	quadSkel[5] = 1.f;
	quadSkel[6] = 0.f;
	quadSkel[7] = 1.f;

	shCol = scd->shaderCollector;
	setupRenderFboShader();
	colShader = shCol->getStdCol();

	uBlock = new UniformBlock(texShader->getProgram(), "quadData");
}

//---------------------------------------------------------------

void FboDedist::add(GWindow* _win, unsigned short width, unsigned short height,
		unsigned short offsX, unsigned short offsY)
{
	scrData.push_back(new fboScreenData());
	scrData.back()->win = _win;
	scrData.back()->width = width;
	scrData.back()->height = height;
	scrData.back()->offsX = offsX;
	scrData.back()->offsY = offsY;
	scrData.back()->cornerOffs = static_cast<unsigned short>(corners.size());
	scrData.back()->cornerSize = glm::vec2(
			static_cast<float>(cornersSize) / static_cast<float>(width),
			static_cast<float>(cornersSize) / static_cast<float>(height));

	for (int i = 0; i < 4; i++)
	{
		corners.push_back(glm::vec2(quadSkel[i * 2], quadSkel[i * 2 + 1]));
		blCorners.push_back(glm::vec2(quadSkel[i * 2], quadSkel[i * 2 + 1]));
	}

	// muss so intialisiert werden...
	for (int i = 0; i < 4; i++)
		uBlock->addVarName(cornersNames[i], &corners[i][0], GL_FLOAT_VEC2);

	for (int i = 0; i < 4; i++)
		uBlock->addVarName("b_" + cornersNames[i], &blCorners[i][0],
				GL_FLOAT_VEC2);

	cornersState.push_back(false);
}

//---------------------------------------------------------------

void FboDedist::init()
{
	for (std::vector<fboScreenData*>::iterator it = scrData.begin();
			it != scrData.end(); ++it)
		(*it)->fbo = new FBO(shCol, (*it)->width, (*it)->height, GL_RGBA8,
		GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	// check if calibration file exists, if this is the case load it
	if (access(fileName.c_str(), F_OK) != -1)
		loadCalib();
}

//--------------------------------------------------------------------------------

void FboDedist::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_C:
				showCorners = !showCorners;
				printf("showCorners: %d\n", showCorners);
				break;
			case GLFW_KEY_S:
				saveCalib();
				printf("save Settings\n");
				break;
			}
		}
	}
}

//--------------------------------------------------------------------------------

void FboDedist::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
	float ref[4] =
	{ 0.f, 0.f, 0.f, 0.f };

	if (action == GLFW_PRESS && showCorners)
	{
		for (std::vector<fboScreenData*>::iterator it = scrData.begin();
				it != scrData.end(); ++it)
		{
			// check which context was clicked
			if ((*it)->win->getWin() == window)
			{
				float xPos = (mouseX - float((*it)->offsX))
						/ static_cast<float>((*it)->width);
				float yPos = (mouseY - float((*it)->offsY))
						/ static_cast<float>((*it)->height);

				// std::cout << "(*it)->cornerOffs: " << (*it)->cornerOffs << " xPos: " << xPos << " yPos: " << yPos << std::endl;

				// check if one of the corners was clicked
				for (int i = 0; i < 4; i++)
				{
					ref[0] = corners[(*it)->cornerOffs * 4 + i].x;
					ref[1] = corners[(*it)->cornerOffs * 4 + i].x
							+ (*it)->cornerSize.x
									* (quadSkel[i * 2] == 0.f ? 1.f : -1.f);
					ref[2] = 1.f - corners[(*it)->cornerOffs * 4 + i].y;
					ref[3] = 1.f - corners[(*it)->cornerOffs * 4 + i].y
							+ (*it)->cornerSize.y
									* (quadSkel[i * 2 + 1] == 0.f ? -1.f : 1.f);

					if ((i == 0 || i == 3 ? xPos > ref[0] : xPos < ref[0])
							&& (i == 0 || i == 3 ? xPos < ref[1] : xPos > ref[1])
							&& (i < 2 ? yPos < ref[2] : yPos > ref[2])
							&& (i < 2 ? yPos > ref[3] : yPos < ref[3]))
					{
						cornersState[(*it)->cornerOffs * 4 + i] = true;
//                            std::cout << cornersNames[i] << ": " << cornersState[(*it)->cornerOffs *4 + i] << std::endl;
					}
				}
			}
		}
	}
	else if (action == GLFW_RELEASE && showCorners)
	{
		for (std::vector<fboScreenData*>::iterator it = scrData.begin();
				it != scrData.end(); ++it)
			for (int i = 0; i < 4; i++)
				cornersState[(*it)->cornerOffs * 4 + i] = false;
	}
}

//--------------------------------------------------------------------------------

void FboDedist::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos;
	mouseY = ypos;

	for (std::vector<fboScreenData*>::iterator it = scrData.begin();
			it != scrData.end(); ++it)
	{
		// check which context was clicked
		if ((*it)->win->getWin() == window)
		{
			for (int i = 0; i < 4; i++)
			{
				// if the corner is active move it
				if (cornersState[(*it)->cornerOffs * 4 + i])
				{
					float xPos = (mouseX - float((*it)->offsX))
							/ static_cast<float>((*it)->width);
					float yPos = 1.0
							- (mouseY - float((*it)->offsY))
									/ static_cast<float>((*it)->height);

					corners[(*it)->cornerOffs * 4 + i].x = xPos;
					corners[(*it)->cornerOffs * 4 + i].y = yPos;
				}
			}

			calcMidAndDist(it);
			newCornerVals = true;
		}
	}
}

//---------------------------------------------------------

void FboDedist::calcMidAndDist(std::vector<fboScreenData*>::iterator it)
{
	glm::vec2 midPoint;
	float dist[4];
	float mP0P2, mP1P3;

	if ((corners[(*it)->cornerOffs * 4 + 2].x - corners[(*it)->cornerOffs * 4].x)
			!= 0.f)
		mP0P2 = (corners[(*it)->cornerOffs * 4 + 2].y
				- corners[(*it)->cornerOffs * 4].y)
				/ (corners[(*it)->cornerOffs * 4 + 2].x
						- corners[(*it)->cornerOffs * 4].x);
	else
		mP0P2 = (corners[(*it)->cornerOffs * 4 + 2].y
				- corners[(*it)->cornerOffs * 4].y);

	float cP0P2 = corners[(*it)->cornerOffs * 4].y
			- mP0P2 * corners[(*it)->cornerOffs * 4].x;

	if ((corners[(*it)->cornerOffs * 4 + 3].x
			- corners[(*it)->cornerOffs * 4 + 1].x) != 0.f)
		mP1P3 = (corners[(*it)->cornerOffs * 4 + 3].y
				- corners[(*it)->cornerOffs * 4 + 1].y)
				/ (corners[(*it)->cornerOffs * 4 + 3].x
						- corners[(*it)->cornerOffs * 4 + 1].x);
	else
		mP1P3 = (corners[(*it)->cornerOffs * 4 + 3].y
				- corners[(*it)->cornerOffs * 4 + 1].y);

	float cP1P3 = corners[(*it)->cornerOffs * 4 + 1].y
			- mP1P3 * corners[(*it)->cornerOffs * 4 + 1].x;

	midPoint.x = (cP0P2 - cP1P3) / (mP1P3 - mP0P2);
	midPoint.y = mP0P2 * midPoint.x + cP0P2;

	for (auto i = 0; i < 4; i++)
		dist[i] = std::fabs(
				glm::length(midPoint - corners[(*it)->cornerOffs * 4 + i]));
}

//--------------------------------------------------------------------------------

void FboDedist::bindFbo(unsigned short _ind)
{
	if ((static_cast<int>(scrData.size()) - 1) >= _ind)
	{
		scrData[_ind]->fbo->bind();
		actBoundFbo = _ind;
	}
	else
	{
		actBoundFbo = -1;
	}
}

//---------------------------------------------------------------

void FboDedist::drawFbo(unsigned short _ind)
{
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
	{
		if (newCornerVals)
		{
			uBlock->update();   // update the corner values
			newCornerVals = false;
		}

		glViewportIndexedf(0, scrData[_ind]->offsX, scrData[_ind]->offsY,
				scrData[_ind]->width, scrData[_ind]->height);

		// render the fbo
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		uBlock->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, scrData[_ind]->fbo->getColorImg());
		fboQuad->draw();

		if (showCorners)
		{
			glDisable(GL_DEPTH_TEST);
			colShader->begin();

			for (auto i = 0; i < 4; i++)
			{
				mat = glm::translate(glm::mat4(1.f),
						glm::vec3(
								(corners[scrData[_ind]->cornerOffs * 4 + i].x
										* 2.f - 1.f)
										+ scrData[_ind]->cornerSize.x
												* (quadSkel[i * 2] == 0 ?
														1.f : -1.f),
								(corners[scrData[_ind]->cornerOffs * 4 + i].y
										* 2.f - 1.f)
										+ scrData[_ind]->cornerSize.y
												* (quadSkel[i * 2 + 1] == 0 ?
														1.f : -1.f), 1.f));
				mat = glm::scale(mat,
						glm::vec3(scrData[_ind]->cornerSize, 1.f));

				colShader->setUniformMatrix4fv("m_pvm", &mat[0][0]);
				cornerQuad->draw();
			}

			glEnable(GL_DEPTH_TEST);
		}
		uBlock->unbind();
	}
}
//---------------------------------------------------------------

void FboDedist::drawExtFbo(unsigned short _ind, int scrWidth, int scrHeight)
{
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
	{
		if (newCornerVals)
		{
			uBlock->update();   // update the corner values
			newCornerVals = false;
		}

		glViewport(0, 0, scrWidth, scrHeight);

		// render the fbo
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		uBlock->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _ind);
//            glBindTexture(GL_TEXTURE_2D, scrData[_ind]->fbo->getColorImg());
		fboQuad->draw();

		/*
		 if (showCorners)
		 {
		 glDisable(GL_DEPTH_TEST);
		 colShader->begin();

		 for(auto i=0;i<4;i++)
		 {
		 mat = glm::translate(glm::mat4(1.f),
		 glm::vec3((corners[scrData[_ind]->cornerOffs *4 + i].x * 2.f - 1.f)
		 + scrData[_ind]->cornerSize.x * (quadSkel[i*2]==0 ? 1.f : -1.f),
		 (corners[scrData[_ind]->cornerOffs *4 + i].y * 2.f - 1.f)
		 + scrData[_ind]->cornerSize.y * (quadSkel[i*2+1]==0 ? 1.f : -1.f),
		 1.f));
		 mat = glm::scale(mat, glm::vec3(scrData[_ind]->cornerSize, 1.f));

		 colShader->setUniformMatrix4fv("m_pvm", &mat[0][0]);
		 cornerQuad->draw();
		 }

		 glEnable(GL_DEPTH_TEST);
		 }
		 */
		uBlock->unbind();
	}
}

//---------------------------------------------------------------

void FboDedist::setupRenderFboShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color;

					uniform quadData { vec2 bl; vec2 b_bl; vec2 br; vec2 b_br; vec2 tl; vec2 b_tl; vec2 tr; vec2 b_tr; };

					uniform mat4 m_pvm; out vec2 tex_coord; out vec4 col;

					void main() {
					// interpolate corner coordinates
					vec2 p = (vec2(position.x, position.y) + 1.0) * 0.5; p = mix(mix(bl, br, p.x), mix(tl, tr, p.x), p.y); p = (p - 0.5) * 2.0;

					col = color; tex_coord = texCoord; gl_Position = m_pvm * vec4(p, 0, 1); });

	vert = "// fboDeDist, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D tex; in vec2 tex_coord; in vec4 col; layout (location = 0) out vec4 color; void main() { color = texture(tex, tex_coord) + col; });

	frag = "// fboDeDist shader, frag\n" + shdr_Header + frag;

	texShader = shCol->addCheckShaderText("fboDeDist", vert.c_str(),
			frag.c_str());
}

//---------------------------------------------------------

glm::mat4 FboDedist::cvMat33ToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j = 0; j < 3; j++)
		for (short i = 0; i < 3; i++)
			out[i][j] = _mat.at<double>(j * 3 + i);

	return out;
}

//--------------------------------------------------------------------------------

void FboDedist::loadCalib()
{
#ifdef HAVE_OPENCV
	printf("loading calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		for (int j = 0; j < static_cast<int>(scrData.size()); j++)
		{
			for (int i = 0; i < 4; i++)
			{
				fs["corner" + std::to_string(j) + "_" + std::to_string(i) + "x"] >> corners[j * 4 + i].x;
				fs["corner" + std::to_string(j) + "_" + std::to_string(i) + "y"] >> corners[j * 4 + i].y;
			}
		}
	}
#endif
}

//--------------------------------------------------------------------------------

void FboDedist::saveCalib()
{
#ifdef HAVE_OPENCV
	printf("saving calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		for (int j = 0; j < static_cast<int>(scrData.size()); j++)
		{
			for (int i = 0; i < 4; i++)
			{
				fs << "corner" + std::to_string(j) + "_" + std::to_string(i) + "x" << corners[j * 4 + i].x;
				fs << "corner" + std::to_string(j) + "_" + std::to_string(i) + "y" << corners[j * 4 + i].y;
			}
		}
	}
#endif
}

//---------------------------------------------------------------

FboDedist::~FboDedist()
{
	delete fboQuad;
	delete cornerQuad;
}
}
