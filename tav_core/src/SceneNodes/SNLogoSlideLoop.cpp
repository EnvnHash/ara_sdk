//
//  SNLogoSlideLoop.cpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved.
//

#include "SNLogoSlideLoop.h"

#define STRINGIFY(A) #A

namespace tav
{
SNLogoSlideLoop::SNLogoSlideLoop(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "NoLight"), logoScale(0.8f), waitTime(23.0), blendTime(
				3.0), slideTime(10.0)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	stdCol = shCol->getStdCol();
	stdTex = shCol->getStdTex();

	logoPos = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{	blendToWhite->reset();
		blendToWhite->start(1.f, 0.f, blendTime, actTime, false);});

	waitForActivation = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{	blendToWhite->start(0.f, 1.f, blendTime, actTime, false);});

	blendToWhite = new AnimVal<float>(tav::RAMP_LIN_UP, [this]()
	{	this->startLogoSlide();});
	blendToWhite->setInitVal(0.f);

	samsLogo = new TextureManager();
	samsLogo->loadTexture2D(
			(*scd->dataPath) + "/textures/samsung/samsung_edge_logo.png");
	samsLogo->setWraping(GL_CLAMP_TO_BORDER);

	samsLogoWhite = new TextureManager();
	samsLogoWhite->loadTexture2D(
			(*scd->dataPath) + "/textures/samsung/samsung_edge_logo_white.png");
	samsLogoWhite->setWraping(GL_CLAMP_TO_BORDER);
}

//---------------------------------------------------------------

void SNLogoSlideLoop::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=2) in vec2 texCoord; out vec2 texCo; void main(void) { texCo = texCoord; gl_Position = position; });
	vert = "// SNLogoSlideLoop pos tex vertex shader\n" + shdr_Header + vert;

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

	geom = "// SNLogoSlideLoop pos tex geom shader\n" + shdr_Header + geom;

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 fragColor; in vec2 gs_texCo; uniform sampler2D tex; uniform float white; uniform float aspect; uniform vec2 offs; uniform float logoScale; uniform float alpha; vec2 moduCoord; void main() { moduCoord = vec2((gs_texCo.x - offs.x) * aspect, (gs_texCo.y - offs.y)) * logoScale; fragColor = texture(tex, moduCoord) * (1.0 - white) + vec4(white); fragColor.a *= alpha; });
	frag = "// SNLogoSlideLoop pos tex shader\n" + shdr_Header + frag;

	shdr = shCol->addCheckShaderText("SNLogoSlideLoopShader", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNLogoSlideLoop::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (!inited)
	{
		camP = cp;
		initShdr(cp);
		identMat = new glm::mat4[cp->nrCams];
		for (short i = 0; i < cp->nrCams; i++)
			identMat[i] = glm::mat4(1.f);

		scaleFact = 2.f / cp->roomDimen->x;

		roomDim.x = 2.f;
		roomDim.y = cp->roomDimen->y * scaleFact;
		roomDim.z = cp->roomDimen->z * scaleFact;

		waitForActivation->start(0.f, 1.f, waitTime, actTime, false);
		logoPos->setInitVal(1.f);
//		logoPos->setInitVal(-(roomDim.y / roomDim.x) * logoScale);
		//                                * (samsLogo->getHeightF() / samsLogo->getWidthF()));

		buildTunnelShape(cp);

		inited = true;
	}

	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
	}

	actTime = time;

	// animation loop
	logoPos->update(time);
	waitForActivation->update(time);
	blendToWhite->update(time);

	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glDepthMask (GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ohne dass kein antialiasing

	modelMat = glm::mat4(1.f);
	//        modelMat = glm::translate( modelMat, glm::vec3(0.f, 0.f, cp->roomDimen->z * -0.5f  - osc->blurOffs) );
	//        modelMat = glm::translate( modelMat, glm::vec3(1.f + osc->blurOffs, 0.f, 0.f) );
	//        modelMat = glm::rotate(modelMat, float(M_PI * -0.5f),
	//                               glm::vec3(0.f, 1.f, 0.f) );

	shdr->begin();
	shdr->setUniformMatrix4fv("projection_matrix_g",
			cp->multicam_projection_matrix, cp->nrCams);
	shdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix,
			cp->nrCams);
	shdr->setUniformMatrix4fv("model_matrix", &modelMat[0][0]);
	shdr->setUniform1i("tex", 0);

	// option 1 schwarz auf weiss
	// make walls white
	shdr->setUniform1f("white", 1.f);
	shdr->setUniform1f("alpha", blendToWhite->getVal());
	tunnelShape->drawElements(GL_TRIANGLES);

	shdr->setUniform1f("white", 0.f);
	shdr->setUniform2f("offs", logoPos->getVal(), (1.f - logoScale) * 0.5f);
	shdr->setUniform1f("logoScale", 1.f / logoScale);
	shdr->setUniform1f("aspect", 1.15f);
	shdr->setUniform1f("alpha", 1.f);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, samsLogo->getId());

	/*
	 // option 1 weiss auf schwarz
	 // make walls white
	 //	shdr->setUniform1f("white", 0.f);
	 //	shdr->setUniform1f("alpha", blendToWhite->getVal());
	 //	tunnelShape->drawElements(GL_TRIANGLES);

	 shdr->setUniform1f("white", 0.f);
	 shdr->setUniform2f("offs", logoPos->getVal(), (1.f - logoScale) * 0.5f);
	 shdr->setUniform1f("logoScale", 1.f / logoScale);
	 shdr->setUniform1f("aspect", 1.15f );
	 shdr->setUniform1f("alpha", 1.f);
	 glActiveTexture(GL_TEXTURE0);
	 glBindTexture(GL_TEXTURE_2D, samsLogoWhite->getId());
	 */

	tunnelShape->drawElements(GL_TRIANGLES);
}

//---------------------------------------------------------------

void SNLogoSlideLoop::update(double time, double dt)
{

}

//---------------------------------------------------------------

void SNLogoSlideLoop::buildTunnelShape(camPar* cp)
{
	// build the Shape of the tunnel (in this case left, back, right.
	// left and right are rendered as two screens)
	int numberVertices = 8;
	int numberIndices = 6 * 3;
	float totalLength = roomDim.x * 2.f + roomDim.z;

	//cp->roomDimen = glm::vec3(2.000000f, 0.628571f, 0.628571f);
	std::cout << glm::to_string(roomDim) << std::endl;

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
		vertices[i * 3 + 1] = roomDim.y * (i % 2 == 0 ? -0.5f : 0.5f);

		// z coordinate is - for i < 4 and +  for i > 3
		vertices[i * 3 + 2] = roomDim.z * (i < 4 ? -0.501f : 0.501f);

		// - tex Coords --

		// x coordinate is - for 0-1 and 7-6 and + for 2 - 5
		texCoords[i * 2] =
				(i / 2) < 2 ? float(i / 2) * roomDim.x / totalLength :
				(i / 2) < 3 ? (roomDim.x + roomDim.z) / totalLength : 1.f;

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

void SNLogoSlideLoop::startLogoSlide()
{
	if (blendToWhite->getVal() >= 1.f)
	{
		logoPos->start(-0.75f, 1.f, slideTime, actTime, false);
	}
	else
	{
		waitForActivation->reset();
		waitForActivation->start(0.f, 1.f, waitTime, actTime, false);
	}
}

//---------------------------------------------------------------

SNLogoSlideLoop::~SNLogoSlideLoop()
{
}
}
