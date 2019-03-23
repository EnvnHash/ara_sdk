//
//  SNTestCs5Tunnel.cpp
//  tav_scene
//
//  Created by Sven Hahne on 09/03/16.
//  Copyright © 2016 Sven Hahne. All rights reserved.
//

#include "SNTestCs5Tunnel.h"

#define STRINGIFY(A) #A

namespace tav
{
SNTestCs5Tunnel::SNTestCs5Tunnel(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs,"NoLight"), logoScale(0.8f)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	stdCol = shCol->getStdCol();
	stdTex = shCol->getStdTex();

	testPic = new TextureManager();
	testPic->loadTexture2D((*scd->dataPath)+"/textures/tv.jpg");
	testPic->setWraping(GL_CLAMP_TO_BORDER);
}

//----------------------------------------------------

void SNTestCs5Tunnel::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout (location=0) in vec4 position;
	layout (location=2) in vec2 texCoord;
	out vec2 texCo;
	void main(void) {
		texCo = texCoord;
		gl_Position = position;
	});
	vert = "// SNTestCs5Tunnel pos tex vertex shader\n" +shdr_Header +vert;



	shdr_Header = "#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="+nrCams+") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["+nrCams+"];\n uniform mat4 projection_matrix_g["+nrCams+"];";

	std::string geom = STRINGIFY(in vec2 texCo[];\n
	out vec2 gs_texCo;\n
	uniform mat4 model_matrix;\n

	void main()\n
	{\n
		gl_ViewportIndex = gl_InvocationID;\n

		for (int i=0; i<gl_in.length(); i++)\n
		{\n
			gs_texCo = texCo[i];\n
			gl_Position = projection_matrix_g[gl_InvocationID]\n
			* view_matrix_g[gl_InvocationID]\n
			* model_matrix\n
			* gl_in[i].gl_Position;\n
			EmitVertex();\n
		}\n
		EndPrimitive();\n
	});

	geom = "// SNTestCs5Tunnel pos tex geom shader\n" +shdr_Header +geom;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;
	in vec2 gs_texCo;
	uniform sampler2D tex;
	void main()
	{
		fragColor = texture(tex, gs_texCo);
	});

	frag = "// SNTestCs5Tunnel pos tex shader\n"+shdr_Header+frag;

	shdr = shCol->addCheckShaderText("SNTestCs5TunnelShader",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTestCs5Tunnel::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(!inited)
	{
		camP = cp;
		initShdr(cp);
		identMat = new glm::mat4[cp->nrCams];
		for (short i=0;i<cp->nrCams;i++)
			identMat[i] = glm::mat4(1.f);

		buildTunnelShape(cp);

		inited = true;
	}

	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
	}

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // ohne dass kein antialiasing

	modelMat = glm::mat4(1.f);

	shdr->begin();
	shdr->setUniformMatrix4fv("projection_matrix_g", cp->multicam_projection_matrix, cp->nrCams);
	shdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix, cp->nrCams );
	shdr->setUniformMatrix4fv("model_matrix", &modelMat[0][0]);
	shdr->setUniform1i("tex", 0);

	// make walls white
	glBindTexture(GL_TEXTURE_2D, testPic->getId());

	//tunnelShape->drawElements(GL_TRIANGLES);
	tunnelLeftWall->drawElements(GL_TRIANGLES);
	tunnelBackWall->drawElements(GL_TRIANGLES);
	tunnelRightWall->drawElements(GL_TRIANGLES);
}

//----------------------------------------------------

void SNTestCs5Tunnel::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestCs5Tunnel::buildTunnelShape(camPar* cp)
{
	// build the Shape of the tunnel (in this case left, back, right.
	// left and right are rendered as two screens)
	int numberVertices = 8;
	int numberIndices = 6 *3;

	// get room dimensions
	roomDim = *cp->roomDimen;
	//roomDim = glm::vec3(2.000000f, 0.628571f, 0.628571f);

	// skaliere tunnel dimensionen auf normkubus
	// bestimme den skalierungsfaktor
	float scaleF = 2.f / roomDim.x;

	// skaliere
	float tunnelHeight = roomDim.y * scaleF;	// 0,628
	float tunnelWidth = roomDim.z * scaleF;	// 0,628
	float tunnelDepth = 2.f;

	// umfang des tunnel (seitenwaende links + mitte + rechts)
	float totalLength = tunnelDepth *2.f + tunnelWidth;

	roomDim = glm::vec3(tunnelDepth, tunnelHeight, tunnelWidth);


	GLfloat* vertices = new GLfloat[3 * numberVertices];
	GLfloat* texCoords = new GLfloat[2 * numberVertices];
	GLuint* indices = new GLuint[numberIndices];

	const char* format = "position:3f,texCoord:2f";

	tunnelShape = new VAO(format, GL_STATIC_DRAW, nullptr, 1, true);
	tunnelLeftWall = new VAO(format, GL_STATIC_DRAW, nullptr, 1, true);
	tunnelBackWall = new VAO(format, GL_STATIC_DRAW, nullptr, 1, true);
	tunnelRightWall= new VAO(format, GL_STATIC_DRAW, nullptr, 1, true);

	// construction perspective is looking from the Origin to the center
	// of the left wall

	// define the edge points according to the cube size
	for (short i=0;i<numberVertices;i++)
	{
		// - position --

		// x coordinate is - for 0-1 and 7-6 and + for 2 - 5
		vertices[i*3] = (i < 2 || i > 5) ? -1.f : 1.f;

		// y coordinate is altering from - to +
		vertices[i*3 +1] = roomDim.y * (i % 2 == 0 ? -0.5f : 0.5f);

		// z coordinate is - for i < 4 and +  for i > 3
		vertices[i*3 +2] = roomDim.z * (i < 4 ? -0.5f : 0.5f);


		// - tex Coords --

		// x coordinate is - for 0-1 and 7-6 and + for 2 - 5
		texCoords[i*2] = (i/2) < 2 ? float(i/2) * roomDim.x / totalLength
				: (i/2) < 3 ? (roomDim.x + roomDim.z) / totalLength
						: 1.f;

		// y coordinate is altering from - to +
		texCoords[i*2 +1] = (i % 2 == 0 ? 0.f : 1.f);

		//std::cout << texCoords[i*2] << ", " << texCoords[i*2 +1] << std::endl;
	}

	indices[0] = 1; indices[1] = 0; indices[2] = 2;
	indices[3] = 2; indices[4] = 3; indices[5] = 1;
	indices[6] = 3; indices[7] = 2; indices[8] = 4;
	indices[9] = 4; indices[10] = 5; indices[11] = 3;
	indices[12] = 5; indices[13] = 4; indices[14] = 6;
	indices[15] = 6; indices[16] = 7; indices[17] = 5;

	tunnelShape->upload(POSITION, &vertices[0], numberVertices);
	tunnelShape->upload(TEXCOORD, &texCoords[0], numberVertices);
	tunnelShape->setElemIndices(numberIndices, &indices[0]);


	int wallNrVert = 4;
	int wallNrIndices = 6;
	GLfloat* wallVertices = new GLfloat[3 * wallNrVert];
	GLfloat* wallTexCoords = new GLfloat[2 * wallNrVert];
	GLuint* wallIndices = new GLuint[wallNrIndices];

	//--- tunnelLeftWall-

	for (short i=0;i<wallNrVert;i++)
	{
		for (short j=0;j<3;j++)
			wallVertices[i*3 + j] = vertices[i*3 + j];

		wallTexCoords[i*2] = i < 2 ? 0.f : 1.f;
		wallTexCoords[i*2 +1] = (i % 2 == 0 ? 0.f : 1.f);
	}

	wallIndices[0] = 1; wallIndices[1] = 0; wallIndices[2] = 2;
	wallIndices[3] = 2; wallIndices[4] = 3; wallIndices[5] = 1;

	tunnelLeftWall->upload(POSITION, &wallVertices[0], wallNrVert);
	tunnelLeftWall->upload(TEXCOORD, &wallTexCoords[0], wallNrVert);
	tunnelLeftWall->setElemIndices(wallNrIndices, &wallIndices[0]);


	//--- tunnelBackWall-

	for (short i=0;i<wallNrVert;i++)
		for (short j=0;j<3;j++)
			wallVertices[i*3 +j] = vertices[i*3 +j +6];

	tunnelBackWall->upload(POSITION, &wallVertices[0], wallNrVert);
	tunnelBackWall->upload(TEXCOORD, &wallTexCoords[0], wallNrVert);
	tunnelBackWall->setElemIndices(wallNrIndices, &wallIndices[0]);

	//--- tunnelBackRight-


	for (short i=0;i<wallNrVert;i++)
		for (short j=0;j<3;j++)
			wallVertices[i*3 +j] = vertices[i*3 +j +12];

	tunnelRightWall->upload(POSITION, &wallVertices[0], wallNrVert);
	tunnelRightWall->upload(TEXCOORD, &wallTexCoords[0], wallNrVert);
	tunnelRightWall->setElemIndices(wallNrIndices, &wallIndices[0]);
}

//----------------------------------------------------

SNTestCs5Tunnel::~SNTestCs5Tunnel()
{}
}
