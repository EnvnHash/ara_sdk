/*
 * SNEnelPalabras.cpp
 *
 *  Created on: 14.10.2017
 *      Author: sven
 */

#include "SNEnelPalabras.h"

#define STRINGIFY(A) #A

namespace tav
{

SNEnelPalabras::SNEnelPalabras(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), flWidth(256), flHeight(256),
		accumAlpha(0.056f),
		actDrawMode(DRAW),
		charAlpha(0.65f),
		charFdbk(0.66f),
		contThresh(163.f),
		damping(0.33f),
		depthThresh(3100.f),
		emitIntrv(0.04),
		emitThres(1.f),
		fastBlurAlpha(0.64f),
		fblurSize(512),
		firstCrossMax(false),
		fluidVelTexThres(0.31f),
		fluidVelTexRadius(0.053f),
		fluidVelTexForce(1.17f),
		fluidSmoke(0.457f),
		fluidVelDissip(0.8f),
		lifeTime(1.87f),
		maxNumParticles(200000),
		nrEmitPart(58.f),
		spriteSize(14.f),
		switchThres(5800.f),
		userMapAlpha(0.42f),
		velDirForceRel(0.005f),
		velPosForceRel(3.6f),
		optFlowBlurAlpha(0.38f),
		timeMedFact(20.f),
		secondThresh(0.5f),
		velScale(0.03f),
		switchFadeTime(15.f)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	kin = static_cast<KinectInput*>(scd->kin);

	winMan = static_cast<GWindowManager*>(scd->winMan);
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	// ------- Geometries -------------------

	stdHFlipQuad = static_cast<Quad*>(_scd->stdHFlipQuad);
	stdQuad = static_cast<Quad*>(_scd->stdQuad);
	redQuad = new Quad(-0.01f, -0.01f, 0.02f, 0.02f,
						glm::vec3(0.f, 0.f, 1.f),
						1.f, 0.f, 0.f, 1.f);

	// ------- FBOs -------------------

	textFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight);
	charDrawFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight);
	accumFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight);
	accumFbo->clear();
	diffFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, GL_R8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	compFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight);
	threshFbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
						GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

	// - FastBlurs --
	fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
	fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);



//	optFlowBlur = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(), kin->getDepthHeight(), GL_RGB16F);
	optFlowBlur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize, GL_RGB16F);


	// - Optical Flow --
	optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(), kin->getDepthHeight());
	optFlow->setMedian(0.8f);
	optFlow->setBright(0.3f);


//	timeMed = new GLSLTimeMedian(shCol, fblurSize, fblurSize, GL_RGB16F);

	// ------- Historgram -------------------

	histo = new GLSLHistogram(shCol, _scd->screenWidth, _scd->screenHeight,
			GL_R8, 2, 128, true, false, 1.f);

	// ------- Textures -------------------

    userMap = new TextureManager();
    userMap->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
                      GL_RGBA8, GL_RGBA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE); // for the userMa
	texShader = shCol->getStdTex();
	colShader = shCol->getStdCol();
	stdColAlphaShader = shCol->getStdColAlpha();

	//-------------- Build Char Map ----------------

	charWidth = 256;
	charHeight = 384;
	nrCharSide = 6; // 6x6  felder
	unsigned int nrChar = nrCharSide * nrCharSide; // 8 x 8  felder
	nrChars = 25; // 6x6  felder


	//-------------- Init a Compute Shader Particle System ----------------

	texNrElemStep = 1.f / float(nrCharSide);
	elemPerSide = float(nrCharSide);


	ps = new GLSLParticleSystemFbo(shCol, maxNumParticles,
			float(_scd->screenWidth) / float(_scd->screenHeight));
	ps->setFriction(0.f);
	ps->setAging(true);

	data.emitVel = normalize(glm::vec3(0.f, 1.f, 0.f));
	data.emitCol = glm::vec4(1.f, 0.f, 1.f, 1.f);
	data.posRand = 0.01f; // wenn emit per textur und > 0 -> mehr gpu
	data.dirRand = 0.f;
	data.angle = 0.f;
	data.angleRand = 0.2f;
	data.sizeRand = 0.5f;
	data.texNr = 0;
	data.maxNrTex = nrChar;
	data.texRand = 1.f;
	data.colRand = 0.4f;

	ps->setEmitData(&data);

	// --- Fluid System ----

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->dissipation = 0.999f;
	fluidSim->velocityDissipation = 0.999f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));



	//-------------- Init Shaders ----------------

//	initPartRenderShdr();
	initSpriteShader();
	initDrawInvShdr();
	initAccumShdr();
	initDrawAccumShdr();
	initDiffShdr();
	initGrayShdr();
	initShaders();

	// ----------------- init Character Texture ----------------------------

	nrWords = 4;
	wordPtr = 0;
	ft = new FreetypeTex*[nrWords];

	std::string words[nrWords] = { "POEMA", "LIBRO", "AMOR", "ODA" };
	for (unsigned int i=0;i<nrWords;i++)
	{
		ft[i] = new FreetypeTex(((*scd->dataPath)+"/fonts/Arial.ttf").c_str(), 180);
		ft[i]->setText(words[i]);
	}

	//-----------------------------------------------------------------------

	addPar("charAlpha", &charAlpha);
	addPar("lifeTime", &lifeTime);
	addPar("noiseFreq", &noiseFreq);
	addPar("noiseStren", &noiseStren);
	addPar("spriteSize", &spriteSize);
	addPar("initAmt", &initAmt);
	addPar("damping", &damping);
	addPar("velScale", &velScale);
	addPar("nrEmitPart", &nrEmitPart);
	addPar("velDirForceRel", &velDirForceRel);
	addPar("charFdbk", &charFdbk);
	addPar("accumAlpha", &accumAlpha);
	addPar("velPosForceRel", &velPosForceRel);
	addPar("userMapAlpha", &userMapAlpha);
	addPar("emitIntrv", &emitIntrv);
	addPar("timeMedFact", &timeMedFact);
	addPar("switchThres", &switchThres);


	addPar("depthThresh", &depthThresh); // 10.f, 5000.f, 1.f, 1755.f, OSCData::LIN);
	addPar("fastBlurAlpha", &fastBlurAlpha); // 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
	addPar("optFlowBlurAlpha", &optFlowBlurAlpha); //0.f, 1.f, 0.0001f, 0.76f, OSCData::LIN);


	int tex_units;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &tex_units);
	std::cout << "tex_units: " << tex_units << std::endl;

}

//---------------------------------------------------------------

void SNEnelPalabras::initShaders()
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

	stdVert = "// SNEnelPalabras depth threshold vertex shader\n" +shdr_Header +stdVert;
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

	frag = "// SNEnelPalabras depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres = shCol->addCheckShaderText("SNEnelPalabras_thres", stdVert.c_str(), frag.c_str());


	//-------------------------------------

	std::string fragThres2 = STRINGIFY(uniform sampler2D kinDepthTex; // 16 bit -> float
								 uniform float thresh;
								 in vec2 tex_coord;
								 layout (location = 0) out vec4 color;
								 float outVal;
								 void main()
								 {
									 outVal = texture(kinDepthTex, tex_coord).r;
									 color = vec4( outVal > thresh ? 1.0 : 0.0 );
								 });

	fragThres2 = "// SNEnelPalabras depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres2 = shCol->addCheckShaderText("SNEnelPalabras_thres2", stdVert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::initSpriteShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout(location = 0) in vec4 position;
	uniform mat4 m_pvm;
	void main() {
		gl_Position = position;
	});

	vert = "// SNTestParticlesFboVelTex  Draw Point Sprite Shader vertex shader\n" + shdr_Header + vert;

	//-----------------------------------------------------------------------------------

	std::string geom = STRINGIFY(
	uniform sampler2D pos_tex;
	//uniform sampler2D vel_tex;
	//uniform sampler2D col_tex;
	uniform sampler2D aux0_tex;

	uniform ivec2 cellSize;
	uniform mat4 m_pvm;

	out GS_VS_DATA {
		vec4 pos;
		//vec4 color;
		vec4 aux0;
	} outData;

	vec4 pPos;
	vec4 pVel;
	//vec4 pCol;
	vec4 pAux0;
	ivec2 texCoord;

	void main()
	{
		ivec2 baseTexCoord = ivec2(gl_in[0].gl_Position.xy);

		// read the textures
		for (int y=0;y<cellSize.y;y++)
		{
			for (int x=0;x<cellSize.x;x++)
			{
				texCoord = baseTexCoord + ivec2(x, y);
				pAux0 = texelFetch(aux0_tex, texCoord, 0);

				// switching doesn´t work, since we are double buffering...
				// check if the particle is living and if it´s not the last
				// one in the cell, since this is used for offset writing
				if ( pAux0.x > 0.001 )
				{
					pPos = texelFetch(pos_tex, texCoord, 0);
					//pVel = texture(vel_tex, texCoord);
					//pCol = texelFetch(col_tex, texCoord, 0);

					gl_PointSize = pAux0.y;

					//outData.color = pCol * min(pAux0.x, 1.0);
					outData.aux0 = pAux0;
					//outData.aux0.w = 0.0;

					outData.pos = pPos;
					gl_Position = m_pvm * pPos;

					EmitVertex();
					EndPrimitive();
				}
			}
		}
	});

	geom = "// SNTestParticlesFboVelTex Draw Point Sprite Shader\n"
			+ shdr_Header
			+ "layout (points) in;\n layout (points, max_vertices = "
			+ std::to_string(ps->getMaxGeoAmpPoints())+") out;"
			+ geom;

	//-----------------------------------------------------------------------------------


	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	uniform sampler2D tex;
	uniform float elemStep;
	uniform float elemPerSide;
	uniform float alpha;

	in GS_VS_DATA {
		vec4 pos;
		//vec4 color;
		vec4 aux0;
	} inData;

	vec4 procColor;
	vec4 normal;
	vec2 texCoord;
	void main()
	{
		// apply rotation
		// center
		texCoord = vec2(gl_PointCoord.x, 1.05 - gl_PointCoord.y) - vec2(0.5);

		// convert to angle, radius
		texCoord = vec2(atan(texCoord.y, texCoord.x),
						sqrt(texCoord.x * texCoord.x + texCoord.y * texCoord.y));
		// rotate
		texCoord.x += inData.aux0.z;

		// convert back to x, y and move to normalized space
		texCoord = vec2(cos(texCoord.x) * texCoord.y + 0.5,
						sin(texCoord.x) * texCoord.y + 0.5);

		// apply texSelection
		texCoord = texCoord * elemStep
				+ vec2( elemStep * mod(inData.aux0.w, elemPerSide),
						elemStep * floor(inData.aux0.w / elemPerSide));

		procColor = texture(tex, texCoord);

		color = procColor;
		color.a *= alpha;
	});

	frag = "// SNTestParticlesFboVelTex Draw Point Sprite Shader\n" + shdr_Header + frag;

	spriteShader = shCol->addCheckShaderText("testPartFboDrawSpriteTex", vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::initDrawInvShdr()
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

	vert = shdr_Header +vert;

	//----------------------------------------------------------

	shdr_Header = "#version 430\n";

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				//if (outCol.a < 0.1){\n
					//discard;\n
				//} else {\n
					color = vec4(1.0 - outCol.r, 1.0 - outCol.g, 1.0 - outCol.b, outCol.a);\n
			//	}\n
			});

	frag = "// SNEnelPalabras frag\n"+shdr_Header+frag;

	invShader = shCol->addCheckShaderText("SNEnelPalabras_inv", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::initAccumShdr()
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

	vert = "//SNEnelPalabras accum vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform sampler2D maskTex;\n
			uniform float alpha;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				float mask = texture(maskTex, tex_coord).r;\n
				if (mask < 0.01){\n
					discard;\n
				} else {\n
					color = outCol * 0.5;\n
					color.a *= alpha;\n
				}\n
			});

	frag = "// SNEnelPalabras accum frag\n"+shdr_Header+frag;

	accumShader = shCol->addCheckShaderText("SNEnelPalabras_accum", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::initDrawAccumShdr()
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

	vert = "//SNEnelPalabras draw accum vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform float alpha;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				float bright = (outCol.r + outCol.g + outCol.b) * 0.4;
				color = vec4(bright) * alpha;\n
			});

	frag = "// SNEnelPalabras draw accum frag\n"+shdr_Header+frag;

	drawAccumShader = shCol->addCheckShaderText("SNEnelPalabras_drawAccum", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::initDiffShdr()
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

	vert = "//SNEnelPalabras diff vert\n"+shdr_Header +vert;

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

	frag = "// SNEnelPalabras diff frag\n"+shdr_Header+frag;

	diffShader = shCol->addCheckShaderText("SNEnelPalabras_diff", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::initGrayShdr()
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

	vert = "//SNEnelPalabras gray vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform float alpha;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				float gray = (outCol.r + outCol.g + outCol.b) * 0.5;
				color = vec4(gray);\n
				color.a = alpha;\n
			});

	frag = "// SNEnelPalabras gray frag\n"+shdr_Header+frag;

	grayShader = shCol->addCheckShaderText("SNEnelPalabras_gray", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelPalabras::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if ( !wordInited )
	{
		// Build Char Map
		std::string chars[] = {
				"A","B","C","D","E","F",
				"G","H","I","J","K","L",
				"M","N","O","P","Q","R",
				"S","T","U","V","W","X",
				"X","Z"};

		charMapFbo = new FBO(shCol, nrCharSide * charWidth, nrCharSide * charHeight,
				GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
		//std::cout << " build char map size " << nrCharSide * charWidth << ", " << nrCharSide * charHeight << std::endl;


		charMapFbo->bind();
		charMapFbo->clear();

		FreetypeTex** type = new FreetypeTex*[nrChars];
		for (unsigned int i=0;i<nrChars;i++)
		{
			type[i] = new FreetypeTex(((*scd->dataPath)+"/fonts/Arial.ttf").c_str(), 100);
			type[i]->setText(chars[i]);

			float propoTex = type[i]->getTex()->getWidthF() / type[i]->getTex()->getHeightF();
			float aspect = charMapFbo->getWidth() / charMapFbo->getHeight();

			charStep = 2.f / float(nrCharSide);

			glm::vec3 pos = glm::vec3(
					charStep * (float(i % nrCharSide) +0.5f) -1.f + 0.01f,
					charStep * (float(i / nrCharSide) +0.5f) -1.f + 0.01f, // rundungsfehler in glsl zuvorkommne
					0.f);

			glm::mat4 mat = glm::translate(pos)
							* glm::scale( glm::vec3(1.f / float(nrCharSide) * propoTex * 0.9f,
													1.f / float(nrCharSide) * 0.9f, 1.f));

			texShader->begin();
			texShader->setUniformMatrix4fv("m_pvm", &mat[0][0]);
			texShader->setUniform1i("tex", 0);
			type[i]->getTex()->bind(0);
			stdHFlipQuad->draw();
		}

		charMapFbo->unbind();

		delete [] type;

		// normalize char step for texture lookup
		charStep *= 0.5f;

		//----------------------------------------------------

		renderActWord(cp);
		wordInited = true;
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());

	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);
	stdQuad->draw();



	if (actDrawMode == DRAW)
	{
		//------------------------------------
		// draw the char particles

		charDrawFbo->bind();
		charDrawFbo->clearAlpha(charFdbk, 0.f);
		drawParticles(time, dt, cp);
		charDrawFbo->unbind();

		//------------------------------

		// draw Particles to accumulation buffer
		accumFbo->bind();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		accumShader->begin();
		accumShader->setIdentMatrix4fv("m_pvm");
		accumShader->setUniform1i("tex", 0);
		accumShader->setUniform1i("maskTex", 1);
		accumShader->setUniform1f("alpha", accumAlpha * float(1 - int(switching)));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, charDrawFbo->getColorImg());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textFbo->getColorImg());

		stdQuad->draw();

		accumFbo->unbind();

		//-----------------------------

		// build the difference with the word

		diffFbo->bind();
		diffFbo->clear();

		glBlendFunc(GL_ONE, GL_ZERO);

		diffShader->begin();
		diffShader->setIdentMatrix4fv("m_pvm");
		diffShader->setUniform1i("tex", 0);
		diffShader->setUniform1i("refTex", 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, accumFbo->getColorImg());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textFbo->getColorImg());

		stdQuad->draw();

		diffFbo->unbind();


		//------------------------------

		// get the range of filling
		histo->proc(diffFbo->getColorImg());

		//std::cout << histo->getMaximum(0) << std::endl;

		if (!firstCrossMax && histo->getMaximum(0) > 300)
			firstCrossMax = true;

		if( !switching && firstCrossMax && (histo->getMaximum(0) < switchThres) && (time - lastSwitch > 2.0) )
		{
			diffFbo->clear();
			textFbo->clear();
			charDrawFbo->clear();

			wordPtr = (wordPtr +1) % nrWords;
			renderActWord(cp);

			lastSwitch = time;
			switching = true;
		}


		if (switching)
		{
			if ((time - lastSwitch) < switchFadeTime )
			{
				accumDrawAlpha = (time - lastSwitch) / switchFadeTime;

				float switchAtVal = 0.5f;
				float switchAmp = 20.f;

				if (accumDrawAlpha < switchAtVal){

					accumDrawAlpha = 1.f + accumDrawAlpha * switchAmp;

				} else {

					accumDrawAlpha = (1.f + switchAtVal * switchAmp)
							- (accumDrawAlpha - switchAtVal) * 2.f * (1.f + switchAtVal * switchAmp	);
				}

				//std::cout << accumDrawAlpha << std::endl;
			} else
			{
				accumFbo->bind();
				glClearColor(0.f, 0.f, 0.f, 0.f);
				glClear(GL_COLOR_BUFFER_BIT);
				accumFbo->unbind();
				accumFbo->clear();

				accumDrawAlpha = 1.f;
				switching = false;
			}
		}
	}

	//------------------------------

	switch(actDrawMode)
	{
		case RAW_DEPTH : // 1
			kin->uploadDepthImg(true);
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true));
			stdQuad->draw();
			break;

		case DEPTH_THRESH: // 2
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());
			stdQuad->draw();
			break;

		case OPT_FLOW : // 3
			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, timeMed->getResTexId());
//			glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
			//glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
			glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());

			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			stdQuad->draw();

			break;

		case FLUID : // 4

		{
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());

			stdQuad->draw();

			glm::mat4 mat = glm::translate(glm::vec3(0.5f, 0.5f, 0.f))
				* glm::scale( glm::vec3(0.3f, 0.3f, 1.f));

			texShader->setUniformMatrix4fv("m_pvm", &mat[0][0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());
			stdQuad->draw();

		}
			break;

		case SHOW_WORD : // 5

			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);

			glBindTexture(GL_TEXTURE_2D, textFbo->getColorImg());
			stdQuad->draw();
			break;

		case CHAR_MAP : // 6
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, charMapFbo->getColorImg());
			stdQuad->draw();
			break;

		case WORD_ACCUM : // 7
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, accumFbo->getColorImg());
			stdQuad->draw();
			break;

		case WORD_DIFF : // 8

			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffFbo->getColorImg());
			stdQuad->draw();
			break;

		case DRAW : // 9


			compFbo->bind();
			compFbo->clear();

			glEnable(GL_BLEND);


			//-------------------------------------------
			// draw the resulting accumulation buffer

			drawAccumShader->begin();
			drawAccumShader->setIdentMatrix4fv("m_pvm");
			drawAccumShader->setUniform1i("tex", 0);

			drawAccumShader->setUniform1f("alpha", accumDrawAlpha);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, accumFbo->getColorImg());
			stdQuad->draw();


			//-------------------------------------------
			// draw the particles

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw Particles
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, charDrawFbo->getColorImg());
			stdQuad->draw();

			compFbo->unbind();


			//-------------------------------------------
			//-------------------------------------------
			// draw the result inverted

			glClearColor(1.f, 1.f, 1.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBlendFunc(GL_ONE, GL_ZERO);

			// draw Particles
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, compFbo->getColorImg());
			stdQuad->draw();

			break;
	}
}

//----------------------------------------------------

void SNEnelPalabras::renderActWord(camPar* cp)
{

	float propoTex = ft[wordPtr]->getTex()->getWidthF() / ft[wordPtr]->getTex()->getHeightF();
	float aspect = cp->actFboSize.x / cp->actFboSize.y;

	glm::mat4 mat = glm::scale(glm::vec3(1.f, aspect / propoTex, 1.f));

	textFbo->bind();
	textFbo->clear();

	texShader->begin();
	texShader->setUniformMatrix4fv("m_pvm", &mat[0][0]);
	texShader->setUniform1i("tex", 0);
	ft[wordPtr]->getTex()->bind(0);
	stdHFlipQuad->draw();

	textFbo->unbind();
}

//----------------------------------------------------

void SNEnelPalabras::drawParticles(double time, double dt, camPar* cp)
{

	glEnable(GL_POINT_SPRITE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw as sprite
	spriteShader->begin();
	spriteShader->setUniformMatrix4fv("m_pvm", cp->mvp);
	spriteShader->setUniform2iv("cellSize", ps->getCellSizePtr(), 1);
	spriteShader->setUniform1f("elemStep", texNrElemStep);
	spriteShader->setUniform1f("elemPerSide", elemPerSide);
	spriteShader->setUniform1f("alpha", charAlpha);
	spriteShader->setUniform1i("tex", ps->getNrAttachments());

	ps->bindTexBufs(spriteShader);

	glActiveTexture(GL_TEXTURE0 + ps->getNrAttachments());
	glBindTexture(GL_TEXTURE_2D, charMapFbo->getColorImg());

	ps->draw();
}

//----------------------------------------------------

void SNEnelPalabras::update(double time, double dt)
{
	int userLimit = 5;

	if (kin && kin->isReady())
	{
		if (!inited)
		{
			kin->setCloseRange(true);
			kin->setDepthVMirror(true);
			inited = true;

		} else
		{
			if (kin->uploadDepthImg(false))
			{
				//frameNr = kin->getDepthUplFrameNr(false);

				// -- threshold the depth image --
				glDisable(GL_BLEND);

				threshFbo->bind();
				threshFbo->clear();

				depthThres->begin();
				depthThres->setIdentMatrix4fv("m_pvm");
				depthThres->setUniform1i("kinDepthTex", 0);
				depthThres->setUniform1f("depthThres", depthThresh);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

				stdQuad->draw();
				threshFbo->unbind();

				// ------ apply blur on silhoutte --------
				fblur->setAlpha(fastBlurAlpha);
				fblur->proc(threshFbo->getColorImg());

				fblur2nd->setAlpha(fastBlurAlpha);
				fblur2nd->proc(fblur->getResult());


				// 2nd threshold with rgba8
				threshFbo->bind();
				threshFbo->clear();

				depthThres2->begin();
				depthThres2->setIdentMatrix4fv("m_pvm");
				depthThres2->setUniform1i("kinDepthTex", 0);
				depthThres2->setUniform1f("thresh", secondThresh);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

				stdQuad->draw();
				threshFbo->unbind();


				//-------------------------------------------------------

				// -- calculate optical flow, the result will be used to add velocity to the fluid --
				optFlow->update(fblur2nd->getResult(), fblur2nd->getLastResult());

				optFlowBlur->setAlpha(optFlowBlurAlpha);
				optFlowBlur->proc(optFlow->getResTexId());

				if (time - lastTime > emitIntrv)
				{
					// update emit parameters
					data.speed = velScale;
					data.size = spriteSize;
					data.texNr = 0;
					ps->setEmitData(&data);

					ps->emit(int(nrEmitPart), optFlowBlur->getResult(), flWidth,
						flHeight, fluidSim->getVelocityTex());
					lastTime = time;
				}
			}
		}
	}


	//--- Update Fluid, muss jeden Frame passieren, sonst flimmer effekte ----

	fluidSim->setVelTexThresh(fluidVelTexThres);
	fluidSim->setSmokeBuoyancy(fluidSmoke);
	fluidSim->setVelTexRadius(fluidVelTexRadius);
	fluidSim->velocityDissipation = std::pow(fluidVelDissip, 0.2) * 0.1f + 0.9f;
	fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
	fluidSim->update();


	ps->setLifeTime(lifeTime);
	ps->setFriction(damping);
	ps->update(time);
}

//---------------------------------------------------------------

void SNEnelPalabras::onKey(int key, int scancode, int action, int mods)
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
			case GLFW_KEY_3 : actDrawMode = OPT_FLOW;
				printf("actDrawMode = OPT_FLOW \n");
				break;
			case GLFW_KEY_4 : actDrawMode = FLUID;
				printf("actDrawMode = FLUID \n");
				break;
			case GLFW_KEY_5 : actDrawMode = SHOW_WORD;
				printf("actDrawMode = SHOW_WORD \n");
				break;
			case GLFW_KEY_6 : actDrawMode = CHAR_MAP;
				printf("actDrawMode = CHAR_MAP \n");
				break;
			case GLFW_KEY_7 : actDrawMode = WORD_ACCUM;
				printf("actDrawMode = WORD_ACCUM \n");
				break;
			case GLFW_KEY_8 : actDrawMode = WORD_DIFF;
				printf("actDrawMode = WORD_DIFF \n");
				break;
			case GLFW_KEY_9 : actDrawMode = DRAW;
				printf("actDrawMode = DRAW \n");
				break;
		}
	}
}

//----------------------------------------------------

SNEnelPalabras::~SNEnelPalabras()
{
	delete ft;
	delete textFbo;
	delete diffFbo;
	delete accumFbo;
	delete redQuad;
	delete threshFbo;
	delete optFlow;
	delete spriteShader;
}

} /* namespace tav */
