//
//  SNTestFaceLola.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//  parts from assimp simpleopenglexample part from assimp of implementation
//

#include "SNTestFaceLola.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
SNTestFaceLola::SNTestFaceLola(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), texBlendTime(0.25)
{
#ifdef HAVE_OPENCV
	VideoTextureCv** videos = static_cast<VideoTextureCv**>(scd->videoTextures);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	vts = new VideoTextureCv*[2];
	vtsId = new GLuint[2];
	for (int i=0;i<2;i++)
	{
		vtsId[i] = 0;
		vts[i] = videos[ static_cast<int>(_sceneArgs->at("vtex"+std::to_string(i)) ) ]; //
	}
#endif
	osc = (OSCData*) scd->osc;

	faceTex = new TextureManager();
	faceTex->loadTexture2D((*scd->dataPath)+"textures/cara_jimmy.jpg");

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	blendTexVal = new AnimVal<float>(tav::RAMP_LIN_UP, nullptr);
	blendTexVal->setInitVal(0.f);

	shdr = shCol->getStdDirLight();
}

//----------------------------------------------------

void SNTestFaceLola::init(TFO* _tfo)
{}

//----------------------------------------------------

void SNTestFaceLola::initStdShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//------ Position Shader -----------------------------------------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(layout (location=0) in vec4 position;\n
	layout (location=1) in vec3 normal;\n
	layout (location=2) in vec2 texCoord;\n
	layout (location=3) in vec4 color;\n
	uniform mat4 trans;\n
	uniform mat3 m_normal;\n

	uniform float aspect;\n

	out vec3 Normal;\n
	out vec2 tex_coord;\n

	void main() {\n
		tex_coord = vec2(texCoord.x, 1.0 - texCoord.y);\n
		// Normal = (m_normal * normal) * -1.0;\n
		gl_Position = trans * position;\n
	});
	vert = "// SNTestFaceLola Std shader\n" +shdr_Header +vert;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="+nrCams+") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["+nrCams+"];uniform mat4 projection_matrix_g["+nrCams+"];";

	std::string geom = STRINGIFY(in vec3 Normal[];\n
	in vec2 tex_coord[];\n

	out vec2 gs_texCo;\n
	out vec3 gs_Norm;\n

	void main()\n
	{\n
		gl_ViewportIndex = gl_InvocationID;\n
		for (int i=0; i<gl_in.length(); i++)\n
		{\n
			gs_texCo = tex_coord[i];\n
			gs_Norm = Normal[i];\n

			gl_Position = projection_matrix_g[gl_InvocationID]\n
			* view_matrix_g[gl_InvocationID]\n
			* gl_in[i].gl_Position;\n
			EmitVertex();\n
		}\n
		EndPrimitive();\n
	});

	geom = "// SNTestFaceLola Std geom shader\n" +shdr_Header +geom;



	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string frag = STRINGIFY(layout (location = 0) out vec4 FragColor;\n
	uniform sampler2D tex;\n
	in vec3 gs_Norm;\n
	in vec2 gs_texCo;\n

	vec4 pTex;\n

	void main()\n
	{\n
		pTex = texture(tex, gs_texCo);\n
		FragColor = vec4(pTex.rgb, 1.0);\n
	});
	frag = "// SNTestFaceLola Std shader\n"+shdr_Header+frag;

	stdShdr = shCol->addCheckShaderText("testFaceLolaStdShdr", vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTestFaceLola::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//------ Position Shader -----------------------------------------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(layout (location=0) in vec4 position;
	layout (location=1) in vec3 normal;
	layout (location=2) in vec2 texCoord;
	layout (location=3) in vec4 color;
	uniform mat4 m_pvm;
	uniform mat4 normAndCenter;
	uniform mat3 m_normal;
	uniform vec2 scale0;
	uniform vec2 offs0;
	uniform vec2 scale1;
	uniform vec2 offs1;
	uniform float blendTex;

	out vec3 Normal; // surface normal, interpolated between vertices
	out vec2 rawTex_coord;
	out vec2 pTex_coord;

	void main() {
		rawTex_coord = (m_pvm * normAndCenter * position).xy * 8.0;
		//rawTex_coord = (normAndCenter * position).xy;
		pTex_coord = vec2(rawTex_coord.x, 1.0 - rawTex_coord.y)
                                        				 * mix(scale0, scale1, blendTex)
														 + mix(offs0, offs1, blendTex);
		pTex_coord = pTex_coord * 0.5 + 0.5;
		Normal = (m_normal * normal) * -1.0;

		gl_Position = normAndCenter * position;
	});
	vert = "// SNTestFaceLola shader\n" +shdr_Header +vert;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="+nrCams+") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["+nrCams+"];uniform mat4 trans["+nrCams+"];\nuniform mat4 projection_matrix_g["+nrCams+"];";

	std::string geom = STRINGIFY(in vec3 Normal[]; // surface normal, interpolated between vertices
	in vec2 pTex_coord[];
	in vec2 rawTex_coord[];

	out vec2 gs_pTexCo;
	out vec3 gs_Norm;
	out vec2 gs_rawTexCo;
	out float invocID;

	void main()
	{
		gl_ViewportIndex = gl_InvocationID;
		invocID = float(gl_InvocationID);
		for (int i=0; i<gl_in.length(); i++)
		{
			gs_pTexCo = pTex_coord[i];
			gs_Norm = Normal[i];
			gs_rawTexCo = rawTex_coord[i];

			gl_Position = projection_matrix_g[gl_InvocationID]
											  * view_matrix_g[gl_InvocationID]
															  * trans[gl_InvocationID]
																	  * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	});

	geom = "// SNTestFaceLola  geom shader\n" +shdr_Header +geom;



	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(layout (location = 0) out vec4 FragColor;

	uniform sampler2D wait_tex;
	uniform sampler2D talk_tex;

	uniform vec4 ambient;
	uniform vec4 diffuse;
	uniform vec4 lightColor;
	uniform vec3 lightDirection;    // direction toward the light
	uniform vec3 halfVector;        // surface orientation for shiniest spots
	uniform float shininess;        // exponent for sharping highlights
	uniform float strength;         // extra factor to adjust shininess
	uniform float blend;         // extra factor to adjust shininess
	uniform float blendTex;
	uniform float bruteWhite;
	uniform float cutY0;
	uniform float cutY1;

	in vec3 gs_Norm; // surface normal, interpolated between vertices
	in vec2 gs_pTexCo;
	in vec2 gs_rawTexCo;
	in float invocID;

	vec4 waitTexC;
	vec4 talkTexC;
	vec4 outC;

	vec3 rgbWait;
	vec3 rgbTalk;

	void main()
	{
		waitTexC = texture(wait_tex, gs_pTexCo);
		talkTexC = texture(talk_tex, gs_pTexCo);

		rgbWait = waitTexC.rgb * (invocID > 0.0 ?
				(gs_rawTexCo.x > 0.0 ? 1.0 : 1.0 + gs_rawTexCo.x * blend * 20.0 ) :
				(gs_rawTexCo.x < 0.0 ? 1.0 : 1.0 - gs_rawTexCo.x * blend * 20.0 ) );

		rgbTalk = talkTexC.rgb * (invocID > 0.0 ?
				(gs_rawTexCo.x > 0.0 ? 1.0 : 1.0 + gs_rawTexCo.x * blend * 20.0 ) :
				(gs_rawTexCo.x < 0.0 ? 1.0 : 1.0 - gs_rawTexCo.x * blend * 20.0 ) );

		outC = vec4( mix(rgbWait, rgbTalk, blendTex), 1.0);
		FragColor = gs_pTexCo.y > mix(cutY0, cutY1, blendTex) ? outC : vec4(0.0);
		//FragColor = mix(outC, vec4(1.0), bruteWhite);
	});
	frag = "// SNTestFaceLola shader\n"+shdr_Header+frag;

	projTexShdr = shCol->addCheckShaderText("testFaceLola", vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTestFaceLola::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glm::mat4 modelMat;
	glm::mat4 transMat;
	glm::mat3 normalMat;
	float aspect = float(cp->actFboSize.y) / float(cp->actFboSize.x);

	float distFace = 7.f;
	float heightFace = 3.f;
	float heightFondo = 5.f;

	// ----------------------------------------------------------------------------------------

	// wenn das mapping 100% sein soll, muss die generation der textur koordinaten
	// die durch projektion auf das Kopfmodell gewonnen werden mit der perspektivischen
	// verzerrung der kamera berechnet werden, damit es genau passt
	float camFovHor = (74.f / 360.f) * float(M_PI) * 2.f;
	float heightHeadJimmy = 0.3f;	//model kopf hat die Hoehe 1 also durch die Kopgroesse teilen
	float distCamJimmy = 1.7f / heightHeadJimmy;
	float aspectCam = 16.f / 9.f;
	float left = std::tan(camFovHor) * distCamJimmy;
	float top = left / aspect;

	GLMCamera* camSimu = new GLMCamera(GLMCamera::FRUSTUM,
			int(cp->actFboSize.x), int(cp->actFboSize.y),
			-left * aspectCam, left * aspectCam,
			-top, top,                // left, right, bottom, top
			0.f, 0.f, distCamJimmy,              // camPos
			0.f, 0.f, 0.f,   // lookAt
			0.f, 1.f, 0.f,
			distCamJimmy, 100.f);

	// ----------------------------------------------------------------------------------------

	blendTexVal->update(time);


	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if(!inited)
	{
		initShdr(cp);
		initStdShdr(cp);

		transMatr = new glm::mat4[cp->nrCams];

		faceNode = this->addChild();
		faceNode->setName(name);
		faceNode->setActive(true);

		AssimpImport* aImport = new AssimpImport(scd, true);
		aImport->load(((*scd->dataPath)+"models/cara_lola.obj").c_str(), faceNode, [this](){
		});

		fondo = new Quad(-1.f, -1.f, 2.f, 2.f,
				glm::vec3(0.f, 0.f, 1.f),
				0.f, 0.f, 0.f, 1.f);

		inited = true;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_CULL_FACE);

	float rot[2] = { osc->speed, (osc->zoom - 0.5f) * 2.f };


	// FONDO
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

#ifdef HAVE_OPENCV
	projTexShdr->begin();
	projTexShdr->setUniformMatrix4fv("projection_matrix_g", cp->multicam_projection_matrix, cp->nrCams);
	projTexShdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix, cp->nrCams);
	projTexShdr->setUniformMatrix4fv("m_pvm", camSimu->getMVPPtr() );
	projTexShdr->setIdentMatrix4fv("normAndCenter");
	projTexShdr->setUniform1i("wait_tex", 0);
	projTexShdr->setUniform1i("talk_tex", 1);
	projTexShdr->setUniform1f("blend", osc->blurOffs);
	projTexShdr->setUniform1f("cutY0", 0.f);
	projTexShdr->setUniform1f("cutY1", 0.f);
	projTexShdr->setUniform1f("blendTex", blendTexVal->getVal());


	// camera simulation setup wait tex
	projTexShdr->setUniform2f("offs0", 0.f, (0.4f - 1.f) * 2.f );
	projTexShdr->setUniform2f("scale0", 1.f, 1.f );

	// camera simulation setup talk tex
	projTexShdr->setUniform2f("offs1", 0.f, (0.37f - 1.f) * 2.f );
	projTexShdr->setUniform2f("scale1", 1.f, 1.f );

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, vtsId[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, vtsId[1]);
#endif

	/*
	for (int i=0;i<2;i++)
	{
		transMatr[i] = glm::mat4(1.f)
		* glm::translate( glm::vec3(0.f, 0.f, -distFace ) )
		* glm::rotate( float(rot[i] * M_PI), glm::vec3(0.f, 1.f, 0.f) )
		* glm::translate( glm::vec3(0.f, 0.f, aImport->getDimensions().z * aImport->getNormalizedScale() * heightFace * -0.5f
				+ 0.09f) )
		* glm::scale( glm::vec3(heightFondo, heightFondo, 1.f) );
	}

	projTexShdr->setUniformMatrix4fv("trans", &transMatr[0][0][0], 2);

	fondo->draw();



	// CARA
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	modelMat = (*aImport->getNormAndCenterMatr());

	for (int i=0;i<2;i++)
	{
		transMatr[i] = glm::mat4(1.f)
		* glm::translate(
				glm::vec3(0.f, 0.f,
						aImport->getDimensions().z * aImport->getNormalizedScale() * heightFace * -0.5f - distFace ) )
		* glm::rotate( float(rot[i] * M_PI), glm::vec3(0.f, 1.f, 0.f) )
		* glm::scale( glm::vec3(heightFace) );
	}
*/
	normalMat = glm::mat3( glm::transpose( glm::inverse( modelMat * transMat ) ) );

	projTexShdr->setUniformMatrix4fv("normAndCenter", &modelMat[0][0]);
	projTexShdr->setUniformMatrix4fv("trans", &transMatr[0][0][0], 2);
	projTexShdr->setUniformMatrix3fv("m_normal", &normalMat[0][0] );
	projTexShdr->setUniform1f("bruteWhite", 0.f );
	projTexShdr->setUniform1f("cutY0", 0.f);
	projTexShdr->setUniform1f("cutY1", 0.f);

	// setup with camera simulation, wait texture
	projTexShdr->setUniform2f("offs0", 0.f, (0.14f - 0.5f) * 2.f);
	projTexShdr->setUniform2f("scale0", 0.25f * 2.f, 0.26f * 2.f );

	// setup with camera simulation, talk texture
	projTexShdr->setUniform2f("offs1", 0.f, (0.11f - 0.5f) * 2.f);
	projTexShdr->setUniform2f("scale1", 0.25f * 2.f, 0.26f * 2.f );
}

//----------------------------------------------------

void SNTestFaceLola::update(double time, double dt)
{
	if (osc->extStartHasNewVal)
	{
		std::cout << osc->extStartHasNewVal << std::endl;

		if(osc->extStart == 0)
		{
			// wait tex
			blendTexVal->reset();
			blendTexVal->setInitVal(1.f);
			blendTexVal->start(1.f, 0.f, texBlendTime, time, false);

			//texSelect = 0.f;
		} else
		{
			// explo tex
			blendTexVal->reset();
			blendTexVal->setInitVal(0.f);
			blendTexVal->start(0.f, 1.f, texBlendTime, time, false);

			//        		texSelect = 1.f;
		}

		osc->extStartHasNewVal = false;
	}

#ifdef HAVE_OPENCV
	for (int i=0;i<2;i++)
	{
		//videoTextures[i]->get_frame_pointer(21.0, false);
		vts[i]->updateDt(dt, true);
		vtsId[i] = vts[i]->loadFrameToTexture();
	}
#endif
}

//--------------------------------------------------------------------------------

void SNTestFaceLola::onKey(int key, int scancode, int action, int mods)
{}

//---------------------------------------------------------

void SNTestFaceLola::startThreads(double time, double dt)
{}

//---------------------------------------------------------

void SNTestFaceLola::stopThreads(double time, double dt)
{}

//---------------------------------------------------------

void SNTestFaceLola::cleanUp()
{}

//----------------------------------------------------

SNTestFaceLola::~SNTestFaceLola()
{
	delete shdr;
}

}
