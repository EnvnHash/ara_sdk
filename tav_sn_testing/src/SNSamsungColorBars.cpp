//
//  SNSamsungColorBars.cpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved.
//

#include "SNSamsungColorBars.h"

#define STRINGIFY(A) #A

namespace tav
{
SNSamsungColorBars::SNSamsungColorBars(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "NoLight"), endSwitchTime(7.0), // = muss totale zeit sein
		offTime(39.0), enterFromLeftTime(1.0), waitTime(1.0), scaleTexTime(1.0), rotate90Time(
				1.0), fromTopTime(0.5), waitToRotTime(0.5), multQuadTransTime(
				2.0), nrBars(4), goToStart(false)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	stdCol = shCol->getStdCol();
	stdTex = shCol->getStdTex();

	wallMats = new glm::mat4[3];
	wallMats[0] = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f))
			* glm::scale(glm::vec3(2.5f, 1.f, 1.f));

	wallMats[1] = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 1.f))
			* glm::scale(glm::vec3(2.5f, 1.f, 1.f));

	wallMats[2] = glm::translate(glm::mat4(1.f), glm::vec3(1.2f, 0.f, 0.f))
			* glm::rotate(glm::mat4(1.f), float(M_PI) * 0.5f,
					glm::vec3(0.f, 1.f, 0.f))
			* glm::scale(glm::vec3(0.35f, 1.f, 1.f));

	modelMat = new glm::mat4[80];

	rotMats = new glm::mat4[nrBars];

	offTimeV = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{

		enterFromLeft->start(-5.5f, 2.f, enterFromLeftTime, actTime, false);
		enterFromLeft->setInitVal(-5.5f);

		endSwitch->setInitVal(0.f);
		endSwitch->start(0.f, 1.f, endSwitchTime, actTime, false);
	});

	endSwitch = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{
	});

	multQuadTrans = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{

	});
	multQuadTrans->setInitVal(-4.f);

	rotateQuad = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{
		multQuadTrans->setInitVal(-3.f);
		multQuadTrans->start(-4.f, 3.f, multQuadTransTime, actTime, false);
	});
	rotateQuad->setInitVal(0.f);

	waitRot = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{
		rotateQuad->setInitVal(0.f);
		rotateQuad->start(0.f, 1.f, rotate90Time, actTime, false);
	});

	fromTop = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{
		waitRot->start(0.f, 1.f, waitToRotTime, actTime, false );
	});
	fromTop->setInitVal(5.f);

	enterFromLeft = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{

		if(enterFromLeftTimeCntr < 2)
		{
			enterFromLeft->reset();
			enterFromLeft->setInitVal(-5.5f);
			enterFromLeft->start(-5.5f, 2.f, enterFromLeftTime, actTime, false);

			enterFromLeftTimeCntr++;

		}
		else
		{
			fromTop->setInitVal(2.f);
			fromTop->start(2.f, 0.f, fromTopTime, actTime, false);
		}
	});

	colorBar = new TextureManager();
	colorBar->loadTexture2D((*scd->dataPath) + "/textures/samsung/sieben.png");
	colorBar->setWraping(GL_CLAMP_TO_EDGE);

	colorBarLR = new TextureManager();
	colorBarLR->loadTexture2D(
			(*scd->dataPath) + "/textures/samsung/sieben_2.png");
	colorBarLR->setWraping(GL_CLAMP_TO_EDGE);

	black = new TextureManager();
	black->loadTexture2D((*scd->dataPath) + "/textures/black.png");
	black->setWraping(GL_CLAMP_TO_EDGE);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);

	clearQuad = new Quad(-1.2f, -1.f, 2.4f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);

	quadColorBLR = new Quad(-1.f, -1.f, 4.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);

}

//---------------------------------------------------------------

void SNSamsungColorBars::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=2) in vec2 texCoord; out vec2 texCo; void main(void) { texCo = texCoord; gl_Position = position; });
	vert = "// SNSamsungColorBars pos tex vertex shader\n" + shdr_Header + vert;

	shdr_Header =
			"#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="
					+ nrCams
					+ ") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["
					+ nrCams + "];\n uniform mat4 projection_matrix_g[" + nrCams
					+ "];";

	std::string geom =
			STRINGIFY(
					in vec2 texCo[];\n out vec2 gs_texCo;\n uniform mat4 model_matrix;\n

					void main()\n {\n gl_ViewportIndex = gl_InvocationID;\n

					for (int i=0; i<gl_in.length(); i++)\n {\n gs_texCo = texCo[i];\n gl_Position = projection_matrix_g[gl_InvocationID]\n * view_matrix_g[gl_InvocationID]\n * model_matrix\n * gl_in[i].gl_Position;\n EmitVertex();\n }\n EndPrimitive();\n });

	geom = "// SNSamsungColorBars pos tex geom shader\n" + shdr_Header + geom;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 fragColor; in vec2 gs_texCo; uniform sampler2D tex; uniform vec2 offs; uniform float scaleTexY; void main() { fragColor = texture(tex, gs_texCo); });
	frag = "// SNSamsungColorBars pos tex shader\n" + shdr_Header + frag;

	shdr = shCol->addCheckShaderText("SNSamsungColorBarsShader", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNSamsungColorBars::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	actTime = time;

	enterFromLeft->update(time);
	fromTop->update(time);
	rotateQuad->update(time);
	rotateQuad->update(time);
	waitRot->update(time);
	multQuadTrans->update(time);
	endSwitch->update(time);
	offTimeV->update(time);

	if (!inited)
	{
		camP = cp;
		initShdr(cp);
		identMat = new glm::mat4[cp->nrCams];
		for (short i = 0; i < cp->nrCams; i++)
			identMat[i] = glm::mat4(1.f);

		enterFromLeft->start(-5.5f, 2.f, enterFromLeftTime, time, false);
		enterFromLeft->setInitVal(-5.5f);

		endSwitch->start(0.f, 1.f, endSwitchTime, time, false);
		//buildTunnelShape(cp);

		inited = true;
	}

	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
	}

	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // ohne dass kein antialiasing

	/*
	 for(int i=0;i<nrBars;i++)
	 {
	 modelMat[i] = glm::mat4(1.f);
	 modelMat[i] = glm::scale(modelMat[i], glm::vec3(rotateQuad->getVal() + 1.f, scaleTex->getVal(), 1.f) );
	 }
	 */

	modelMat[0] = glm::translate(glm::vec3(enterFromLeft->getVal(), 0.f, 0.f));

	modelMat[1] = glm::translate(glm::vec3(0.f, fromTop->getVal(), 0.f));
	modelMat[1] = glm::rotate(modelMat[1],
			rotateQuad->getVal() * float(M_PI) * 1.f, glm::vec3(0.f, 0.f, 1.f));

	for (int i = 0; i < 3; i++)
		modelMat[2 + i] = glm::translate(
				glm::vec3(multQuadTrans->getVal() + float(i) / 1.f, 0.f, 0.f));

	if (endSwitch->getVal() < 1.f)
	{

		shdr->begin();
		shdr->setUniformMatrix4fv("projection_matrix_g",
				cp->multicam_projection_matrix, cp->nrCams);
		shdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix,
				cp->nrCams);
		//shdr->setUniform1f("scaleTexY", scaleTex->getVal());
		shdr->setUniform1i("tex", 0);

		for (int i = 0; i < 2; i++)
		{
			calcMat = wallMats[i] * modelMat[0];
			shdr->setUniformMatrix4fv("model_matrix", &calcMat[0][0]);
			shdr->setUniform1f("alpha", 0);
			colorBarLR->bind(0);
			quadColorBLR->draw();

			calcMat = wallMats[i] * modelMat[1];
			shdr->setUniformMatrix4fv("model_matrix", &calcMat[0][0]);
			colorBar->bind(0);
			quad->draw();

			for (int j = 0; j < 3; j++)
			{
				calcMat = wallMats[i] * modelMat[2 + j];
				shdr->setUniformMatrix4fv("model_matrix", &calcMat[0][0]);
				colorBarLR->bind(0);
				quad->draw();
			}
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ohne dass kein antialiasing

		// render black over back wall
		shdr->setUniformMatrix4fv("model_matrix", &wallMats[2][0][0]);
		black->bind(0);
		clearQuad->draw();
	}
}

//---------------------------------------------------------------

void SNSamsungColorBars::update(double time, double dt)
{

	// animation loop
	//scaleTex->update(time);
}

//---------------------------------------------------------------

void SNSamsungColorBars::buildTunnelShape(camPar* cp)
{
	// build the Shape of the tunnel (in this case left, back, right.
	// left and right are rendered as two screens)
	int numberVertices = 8;
	int numberIndices = 6 * 3;
	float totalLength = cp->roomDimen->x * 2.f + cp->roomDimen->z;

	//cp->roomDimen = glm::vec3(2.000000f, 0.628571f, 0.628571f);
	std::cout << glm::to_string(*cp->roomDimen) << std::endl;

	GLfloat* vertices = new GLfloat[3 * numberVertices];
	GLfloat* texCoords = new GLfloat[2 * numberVertices];
	GLuint* indices = new GLuint[numberIndices];

	const char* format = "position:3f,texCoord:2f";
	tunnelShape = new VAO(format, GL_STATIC_DRAW, nullptr, 1, true);

	// construction perspective is looking from the Origin to the center
	// of the left wall

	// define the edge points according to the cube size

	for (short i = 0; i < numberVertices; i++)
	{
		// - position --

		// x coordinate is - for 0-1 and 7-6 and + for 2 - 5
		vertices[i * 3] = (i < 2 || i > 5) ? -1.f : 1.f;

		// y coordinate is altering from - to +
		vertices[i * 3 + 1] = cp->roomDimen->y * (i % 2 == 0 ? -0.5f : 0.5f);

		// z coordinate is - for i < 4 and +  for i > 3
		vertices[i * 3 + 2] = cp->roomDimen->z * (i < 4 ? -0.5f : 0.5f);

		// - tex Coords --

		// x coordinate is - for 0-1 and 7-6 and + for 2 - 5
		texCoords[i * 2] =
				(i / 2) < 2 ? float(i / 2) * cp->roomDimen->x / totalLength :
				(i / 2) < 3 ?
						(cp->roomDimen->x + cp->roomDimen->z) / totalLength :
						1.f;

		// y coordinate is altering from - to +
		texCoords[i * 2 + 1] = (i % 2 == 0 ? 0.f : 1.f);
	}

	indices[0] = 1;
	indices[1] = 0;
	indices[2] = 2;
	indices[3] = 2;
	indices[4] = 3;
	indices[5] = 1;
	indices[6] = 3;
	indices[7] = 2;
	indices[8] = 4;
	indices[9] = 4;
	indices[10] = 5;
	indices[11] = 3;
	indices[12] = 5;
	indices[13] = 4;
	indices[14] = 6;
	indices[15] = 6;
	indices[16] = 7;
	indices[17] = 5;

	tunnelShape->upload(POSITION, &vertices[0], numberVertices);
	tunnelShape->upload(TEXCOORD, &texCoords[0], numberVertices);
	tunnelShape->setElemIndices(numberIndices, &indices[0]);
}

//---------------------------------------------------------------

SNSamsungColorBars::~SNSamsungColorBars()
{
}
}
