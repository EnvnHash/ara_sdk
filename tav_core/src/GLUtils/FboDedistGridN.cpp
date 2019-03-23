//
//  FboDedistGridN.cpp
//  tav_core
//
//  Created by Sven Hahne on 22/12/15.
//  Copyright © 2015 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "FboDedistGridN.h"

namespace tav
{
FboDedistGridN::FboDedistGridN(ShaderCollector* _shCol, unsigned _nrGridPointsX,
		unsigned _nrGridPointsY, unsigned int _width, unsigned int _height, GLenum _type,
		GLenum _target, bool _depthBuf, int _nrSamples, bool _saveToDisk) :
		shCol(_shCol), nrGridPointsX(_nrGridPointsX), nrGridPointsY(_nrGridPointsY),
		width(_width), height(_height), showCorners(false), newCornerVals(false),
		saveToDisk(_saveToDisk)
{
	fboQuad = new QuadArray(_nrGridPointsX, _nrGridPointsY, -1.f, -1.f, 2.f, 2.f,
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

	setupRenderFboShader();
	colShader = shCol->getStdCol();

	uBlock = new UniformBlock(texShader->getProgram(), "quadData");

	// check if calibration file exists, if this is the case load it
	if (saveToDisk && access(fileName.c_str(), F_OK) != -1)
		loadCalib();
}

//--------------------------------------------------------------------------------

void FboDedistGridN::onKey(int key, int scancode, int action, int mods)
{
	/*
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
	*/
}

//---------------------------------------------------------------

void FboDedistGridN::draw()
{
	/*
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
	*/
}

//---------------------------------------------------------------

void FboDedistGridN::drawExtFbo(int scrWidth, int scrHeight, int texId)
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
		glBindTexture(GL_TEXTURE_2D, texId);
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

//---------------------------------------------------------------

void FboDedistGridN::setupRenderFboShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;
		layout( location = 1 ) in vec4 normal;
		layout( location = 2 ) in vec2 texCoord;
		layout( location = 3 ) in vec4 color;

		uniform quadData {
			vec2 bl; vec2 b_bl; vec2 br; vec2 b_br; vec2 tl; vec2 b_tl; vec2 tr; vec2 b_tr;
		};

		uniform mat4 m_pvm;
		out vec2 tex_coord;
		out vec4 col;

		void main() {
			// interpolate corner coordinates
			vec2 p = (vec2(position.x, position.y) + 1.0) * 0.5;
			p = mix(mix(bl, br, p.x), mix(tl, tr, p.x), p.y);
			p = (p - 0.5) * 2.0;

			col = color;
			tex_coord = texCoord;
			gl_Position = m_pvm * vec4(p, 0, 1);
		});

	vert = "// FboDedistGridN, vert\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
		uniform sampler2D tex;
		in vec2 tex_coord;
		in vec4 col;
		layout (location = 0) out vec4 color;
		void main() {
			color = texture(tex, tex_coord) + col;
		});

	frag = "// FboDedistGridN shader, frag\n" + shdr_Header + frag;

	texShader = shCol->addCheckShaderText("FboDedistGridN", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------

glm::mat4 FboDedistGridN::cvMat33ToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j = 0; j < 3; j++)
		for (short i = 0; i < 3; i++)
			out[i][j] = _mat.at<double>(j * 3 + i);

	return out;
}

//--------------------------------------------------------------------------------

void FboDedistGridN::loadCalib()
{
#ifdef HAVE_OPENCV
	printf("loading calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::READ);

	/*
	if (fs.isOpened())
	{
			for (int i = 0; i < 4; i++)
			{
				fs["corner" + std::to_string(j) + "_" + std::to_string(i) + "x"]
						>> corners[j * 4 + i].x;
				fs["corner" + std::to_string(j) + "_" + std::to_string(i) + "y"]
						>> corners[j * 4 + i].y;
			}
	}
	*/
#endif
}

//--------------------------------------------------------------------------------

void FboDedistGridN::saveCalib()
{
#ifdef HAVE_OPENCV

	printf("saving calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::WRITE);
/*
	if (fs.isOpened())
	{
		for (int j = 0; j < static_cast<int>(scrData.size()); j++)
		{
			for (int i = 0; i < 4; i++)
			{
				fs
						<< "corner" + std::to_string(j) + "_"
								+ std::to_string(i) + "x"
						<< corners[j * 4 + i].x;
				fs
						<< "corner" + std::to_string(j) + "_"
								+ std::to_string(i) + "y"
						<< corners[j * 4 + i].y;
			}
		}
	}
	*/
#endif
}

//---------------------------------------------------------------

FboDedistGridN::~FboDedistGridN()
{
	delete fboQuad;
	delete cornerQuad;
}
}
