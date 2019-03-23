//
//  SNTrustHeightMap.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  funktioniert mit dem rohen tiefenbild der kinect (ohne normalisierung) 16bit integer: 0 - 65536
//
//  Hintergrund und Vordergrund werden über einen einfachen Tiefen-Schwellenwert getrennt
//  Die Idee, das Tiefenbild mit einem Referenztiefenbild zu "subtrahiert" geht nicht, weil das Tiefenbild
//  zu sehr rauscht...
//  Der Tiefenschwellenwert wird
//

#include "SNTrustHeightMap.h"

using namespace std;

namespace tav
{
SNTrustHeightMap::SNTrustHeightMap(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), inited(false), flWidth(200), flHeight(200),
			fblurSize(512), captureIntv(120.0), inActAddNoiseInt(40.0),  // wenn nix los emit noise
			actDrawMode(DRAW)  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
{
	winMan = static_cast<GWindowManager*>(scd->winMan);
	VideoTextureCvActRange** vts = static_cast<VideoTextureCvActRange**>(scd->videoTextsActRange);
	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];

	// get onKey function
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	// - add OSC Parameter --

	addPar("alpha", &alpha);
	addPar("depthTresh", &depthTresh);
	addPar("fastBlurAlpha", &fastBlurAlpha);
	addPar("optFlowBlurBright", &optFlowBlurBright);
	addPar("optFlowBlurAlpha", &optFlowBlurAlpha);
	addPar("heightOffs", &heightOffs);
	addPar("heightScale", &heightScale);
	addPar("fluidColTexForce", &fluidColTexForce);
	addPar("timeStep", &timeStep);
	addPar("fluidVelTexThres", &fluidVelTexThres);
	addPar("fluidVelTexRadius", &fluidVelTexRadius);
	addPar("fluidVelTexForce", &fluidVelTexForce);
	addPar("fluidSmoke", &fluidSmoke);
	addPar("fluidSmokeWeight", &fluidSmokeWeight);
	addPar("fluidVelDissip", &fluidVelDissip);
	addPar("fluidScaleXY", &fluidScaleXY);
	addPar("fluidNormHeightAdj", &fluidNormHeightAdj);
	addPar("fluidColMix", &fluidColMix);
	addPar("shapeHeight", &shapeHeight);
	addPar("shapeAddFluidHeight", &shapeAddFluidHeight);
	addPar("shapeHeightAlpha", &shapeHeightAlpha);
	addPar("rotScene", &rotScene);

	// - Fluid System --

	fluidAddCol = new glm::vec4[3];
	fluidAddCol[0] = glm::vec4(25.f, 39.f, 175.f, 255.f) / 255.f;
	fluidAddCol[1] = glm::vec4(247.f, 159.f, 35.f, 255.f) / 255.f;
	fluidAddCol[2] = glm::vec4(134.f, 26.f, 255.f, 255.f) / 255.f;

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->dissipation = 0.999f;
	fluidSim->velocityDissipation = 0.999f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));
	// forceScale nur zum Mouse test
	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.125f,
			static_cast<float>(flHeight) * 0.125f);

	// --- Fbo zum generieren der normalen karte  ---

	normFbo = new FBO(shCol, flWidth, flHeight,
			GL_RGB16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	normFbo->clear();

	fluidAndShape = new FBO(shCol, vt->getWidth(), vt->getHeight(),
			GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	fluidAndShape->clear();

	zoomFbo = new FBO(shCol, vt->getWidth(), vt->getHeight(),
			GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	zoomFbo->clear();

	hmGrid = new QuadArray(256, 256);

	// - Optical Flow --

	optFlow = new GLSLOpticalFlow(shCol, vt->getWidth(), vt->getHeight());
	optFlow->setMedian(0.8f);
	optFlow->setBright(0.3f);

	// - FastBlurs --

	optFlowBlur = new FastBlurMem(0.84f, shCol, vt->getWidth(),
			vt->getHeight(), GL_RGBA16F);

	// --- Texturen fuer den Litsphere Shader  ---

	litsphereTex = new TextureManager();
	litsphereTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/Unknown-37.jpeg");

	bumpMap = new TextureManager();
	bumpMap->loadTexture2D(*scd->dataPath+"/textures/bump_maps/Unknown-2.jpeg");

	// --- Geo Primitives ---

	rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on

	// --- Shaders ---

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
	normShader = shCol->getStdHeightMapSobel();

	initAddShapeShader();
	initFluidHeightShader();
}

//----------------------------------------------------

void SNTrustHeightMap::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	switch(actDrawMode)
	{
	case RAW_DEPTH :
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, actUplTexId);
		rawQuad->draw();
		break;

	case OPT_FLOW:
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
		//glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
		rawQuad->draw();
		break;

	case OPT_FLOW_BLUR:
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());
		rawQuad->draw();
		break;

	case FLUID_VEL:
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());
		rawQuad->draw();
		break;

	case FLUID_AND_SHAPE:
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
		rawQuad->draw();
		break;

	case DRAW:
	{

		zoomFbo->bind();
		zoomFbo->clear();

		texShader->begin();
		texShader->setUniformMatrix4fv("m_pvm", vt->getTransMatPtr());
		//				fluidHeightShdr->setUniformMatrix4fv("vid_pvm", vt->getTransMatFlipHPtr());
		//texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		vt->bindActFrame(0);
		rawQuad->draw();

		zoomFbo->unbind();




		glEnable(GL_DEPTH_TEST);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// translate model back to origin
		glm::mat4 rotMatr = glm::translate(cp->model_matrix_mat4, glm::vec3(0.f, 0.f, heightOffs));
		// rotate
		rotMatr = glm::scale(rotMatr, glm::vec3(1.f, -1.f, 1.f));
		// translate back into visible area
		rotMatr = glm::translate( rotMatr, glm::vec3(0.f, 0.f, -heightOffs));

		glm::mat3 normalMat = glm::mat3( glm::transpose( glm::inverse( rotMatr ) ) );
		rotMatr = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * rotMatr;

		fluidHeightShdr->begin();
		fluidHeightShdr->setUniformMatrix4fv("m_pvm", &rotMatr[0][0]);
		fluidHeightShdr->setUniformMatrix3fv("m_normal", &normalMat[0][0]);
		fluidHeightShdr->setUniform1f("scaleXY", fluidScaleXY);
		fluidHeightShdr->setUniform1f("heightScale", heightScale);
		fluidHeightShdr->setUniform1f("heightOffset", heightOffs);
		fluidHeightShdr->setUniform1f("alpha", alpha);
		fluidHeightShdr->setUniform1f("bumpAmt", 0.f);
		//				fluidHeightShdr->setUniform1f("bumpAmt", 0.12f);
		fluidHeightShdr->setUniform1f("bumpScale", 3.4f);
		fluidHeightShdr->setUniform1f("colMix", fluidColMix);

		fluidHeightShdr->setUniform1i("rgbHeightMap", 0);
		fluidHeightShdr->setUniform1i("normMap", 1);
		fluidHeightShdr->setUniform1i("litSphereTex", 2);
		fluidHeightShdr->setUniform1i("bumpMap", 3);
		fluidHeightShdr->setUniform1i("vidTex", 4);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
		//                    glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normFbo->getColorImg());

		litsphereTex->bind(2);
		bumpMap->bind(3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, zoomFbo->getColorImg());

		hmGrid->draw();

		break;
	}
	default:
		break;
	}
}

//----------------------------------------------------

void SNTrustHeightMap::update(double time, double dt)
{
	vt->updateDt(time, false);

	if ( actUplTexId != vt->loadFrameToTexture() )
	{
		lastTexId = actUplTexId;
		actUplTexId = vt->loadFrameToTexture();

		// -- calculate optical flow, the result will be used to add velocity to the fluid --

		optFlow->update(actUplTexId, lastTexId);
		optFlow->setPVM( vt->getTransMatPtr() );

		//optFlowBlur->setPVM( vt->getTransMatFlipHPtr() );
		optFlowBlur->setBright(optFlowBlurBright);
		optFlowBlur->setAlpha(optFlowBlurAlpha);
		optFlowBlur->proc(optFlow->getResTexId());

		//--- Update Fluid, muss jeden Frame passieren, sonst flimmer effekte ----

		// set Fluid Color
		glm::vec4 eCol = glm::vec4(0.7f, 0.7f, 0.7f, 1.f);

		fluidSim->setVelTexThresh(fluidVelTexThres);
		fluidSim->setSmokeBuoyancy(fluidSmoke);
		fluidSim->setVelTexRadius(fluidVelTexRadius);
		fluidSim->setTimeStep( timeStep );
		fluidSim->velocityDissipation = fluidVelDissip;

		fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
		fluidSim->addColor(optFlowBlur->getResult(), glm::vec3(eCol), fluidColTexForce, true);
		fluidSim->update();

		// --- add the silhoutte to the fluid

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		fluidAndShape->bind();
		fluidAndShape->clearAlpha(1.f - shapeHeightAlpha);

		addShapeShdr->begin();
		addShapeShdr->setUniform1i("fluid", 0);
		addShapeShdr->setUniform1i("optFlow", 1);
		addShapeShdr->setUniform1f("shapeHeightScale", shapeHeight);
		addShapeShdr->setUniform1f("shapeFluidcale", shapeAddFluidHeight);
		addShapeShdr->setUniform1f("alpha", shapeHeightAlpha);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());

		rawQuad->draw();
		fluidAndShape->unbind();


		// generate normals
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		normFbo->bind();
		normShader->begin();
		normShader->setUniform1i("heightMap", 0);
		normShader->setUniform2f("texGridStep", 1.f / float(flWidth), 1.f / float(flHeight));
		normShader->setUniform1f("heightFact", fluidNormHeightAdj);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fluidAndShape->getColorImg());
		//glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

		rawQuad->draw();
		normFbo->unbind();
	}
}

//----------------------------------------------------

void SNTrustHeightMap::initFluidHeightShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;

	uniform sampler2D rgbHeightMap;
	uniform mat4 m_pvm;
	uniform float scaleXY;
	uniform float heightScale;
	uniform float heightOffset;
	vec4 rgbHmPix;
	float height;
	out vec2 tex_coord;

	void main()
	{
		rgbHmPix = texture(rgbHeightMap, texCoord);
		height = (rgbHmPix.r + rgbHmPix.g + rgbHmPix.b) * heightScale + heightOffset;
		tex_coord = texCoord;
		// gl_Position = m_pvm * vec4(position.xy * scaleXY, min(height, 0.5), 1.0);
		gl_Position = m_pvm * vec4(position.xy * scaleXY, height, 1.0);
	});

	stdVert = "// SNTrustHeightMapfluid Heightmap vertex shader\n" +shdr_Header +stdVert;

	std::string frag = STRINGIFY(in vec2 tex_coord;
	uniform sampler2D normMap;
	uniform sampler2D litSphereTex;
	uniform sampler2D bumpMap;
	uniform sampler2D vidTex;
	uniform sampler2D rgbHeightMap;
	uniform mat3 m_normal;
	uniform float bumpAmt;
	uniform float alpha;
	uniform float bumpScale;
	uniform float colMix;
	vec3 texNorm;
	vec3 bumpNorm;
	vec4 orgCol;
	vec4 litColor;
	float outVal;
	layout (location = 0) out vec4 color;

	void main()
	{
		//orgCol = texture(rgbHeightMap, tex_coord) * 1.5;
		texNorm = m_normal * texture(normMap, tex_coord).xyz;
		bumpNorm = m_normal * texture(bumpMap, tex_coord * bumpScale).xyz;
		//litColor = texture(litSphereTex, tex_coord);
		litColor = texture(litSphereTex, mix(texNorm.xy, bumpNorm.xy, bumpAmt) * 0.5 + vec2(0.5));
		orgCol = texture(vidTex, tex_coord);

		color = mix(orgCol, litColor, colMix);
		color.a = alpha;
	});

	frag = "// SNTrustHeightMap fluid Heightmap fragment shader\n"+shdr_Header+frag;

	fluidHeightShdr = shCol->addCheckShaderText("SNTrustHeightMap_hm", stdVert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTrustHeightMap::initAddShapeShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout (location=0) in vec4 position;
	layout (location=1) in vec3 normal;
	layout (location=2) in vec2 texCoord;
	layout (location=3) in vec4 color;
	out vec2 tex_coord;
	void main(void) {
		tex_coord = texCoord;
		gl_Position = position;
	});
	vert = "//SNTrustHeightMap add shape Vert\n" +shdr_Header +vert;

	std::string frag = STRINGIFY(layout(location = 0) out vec4 color;
	in vec2 tex_coord;
	uniform sampler2D fluid;
	uniform sampler2D optFlow;
	uniform float alpha;
	uniform float shapeHeightScale;
	uniform float shapeFluidcale;

	float height;
	vec4 fluidCol;
	vec4 optFlowCol;

	void main()
	{
		optFlowCol = texture(optFlow, tex_coord);

		fluidCol = texture(fluid, tex_coord);
		height = (optFlowCol.r + optFlowCol.g + optFlowCol.b) * shapeHeightScale;
		height += (fluidCol.r + fluidCol.g + fluidCol.b) * shapeFluidcale;
		if (height <= 0.0) discard;

		color = vec4(height, height, height, alpha);
	});
	frag = "// SNTrustHeightMap add shape frag\n"+shdr_Header+frag;

	addShapeShdr = shCol->addCheckShaderText("SNTrustHeightMap_add_shape", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTrustHeightMap::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1 : actDrawMode = RAW_DEPTH;
		printf("actDrawMode = RAW_DEPTH \n");
		break;
		case GLFW_KEY_2 : actDrawMode = OPT_FLOW;
		printf("actDrawMode = OPT_FLOW \n");
		break;
		case GLFW_KEY_3 : actDrawMode = OPT_FLOW_BLUR;
		printf("actDrawMode = OPT_FLOW_BLUR \n");
		break;
		case GLFW_KEY_4 : actDrawMode = FLUID_VEL;
		printf("actDrawMode = FLUID_VEL \n");
		break;
		case GLFW_KEY_5 : actDrawMode = FLUID_AND_SHAPE;
		printf("actDrawMode = FLUID_AND_SHAPE \n");
		break;
		case GLFW_KEY_6 : actDrawMode = DRAW;
		printf("actDrawMode = DRAW \n");
		break;
		}
	}
}

//----------------------------------------------------

void SNTrustHeightMap::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = (ypos / scd->screenHeight);
}

//----------------------------------------------------

SNTrustHeightMap::~SNTrustHeightMap()
{
	delete userMapConv;
	delete userMapRGBA;
	delete fluidSim;
	delete fluidAddCol;
	delete partEmitCol;
	delete rawQuad;
	delete optFlow;
}
}
