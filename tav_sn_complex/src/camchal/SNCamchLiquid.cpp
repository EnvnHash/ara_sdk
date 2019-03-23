//
//  SNGLSLFluidTest.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNCamchLiquid.h"

namespace tav
{
SNCamchLiquid::SNCamchLiquid(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	// --- init fluid  ---

	scrScale = 1;
	nrEmitters = 16;

	forcePos = new glm::vec2[nrEmitters];
	oldPos = new glm::vec2[nrEmitters];

	flWidth = _scd->screenWidth / scrScale;
	flHeight = _scd->screenHeight / scrScale;
	propo = _scd->roomDim->x / _scd->roomDim->y;

	fluidSize =  glm::vec2(static_cast<float>(flWidth), static_cast<float>(flHeight));
	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.5f, static_cast<float>(flHeight) * 0.85f);

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));
	toLeft = glm::vec2(-0.01f, 0.f);


	// color interpolation
	colSpline = new Spline3D();
	colSpline->push_back( glm::vec3(chanCols[0]) );
	colSpline->push_back( glm::vec3(chanCols[1]) );
	colSpline->push_back( glm::vec3(chanCols[2]) );
	colSpline->push_back( glm::vec3(chanCols[3]) );

	// blur fuer normals
	blur = new FastBlurMem(0.6f, shCol, flWidth, flHeight,  GL_RGBA8);


	// --- Geo Primitives ---

	quad = new Quad(-propo * 0.5f, -1.f, propo, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);

	rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on

	hmGrid = new QuadArray(125, 125);

	// --- OSC---

	// Seting the gravity set up & injecting the background image
	addPar("alpha", &alpha);
	addPar("diss", &diss);
	addPar("veldiss", &veldiss);
	addPar("xPos", &xPos);
	addPar("rad", &rad);
	addPar("oscSpeed", &oscSpeed);
	addPar("oscAmt", &oscAmt);
	addPar("posRandAmtX", &posRandAmtX);
	addPar("posRandAmtY", &posRandAmtY);
	addPar("posRandSpd", &posRandSpd);
	addPar("velScale", &velScale);
	addPar("timeStep", &timeStep);
	addPar("bouy", &bouy);
	addPar("bouyWeight", &bouyWeight);
	addPar("colorSpd", &colorSpd);
	addPar("drawAlpha", &drawAlpha);
	addPar("normHeightAdj", &normHeightAdj);
	addPar("heightOffs", &heightOffs);
	addPar("heightScale", &heightScale);
	addPar("reflAmt", &reflAmt);
	addPar("blurAlpha", &blurAlpha);
	addPar("brightScale", &brightScale);
	addPar("velBlendPos", &velBlendPos);
	addPar("fluidScaleX", &fluidScaleX);
	addPar("fluidScaleY", &fluidScaleY);
	addPar("nrEmit", &nrEmit);
	addPar("yScale", &yScale);


	// --- Shaders---

	colShader = shCol->getStdCol();
	normShader = shCol->getStdHeightMapSobel();
	initShader();
	initFluidHeightShader();

	// --- Fbo zum generieren der normalen karte  ---

	normFbo = new FBO(shCol, flWidth, flHeight,
			GL_RGB16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	normFbo->clear();

	//        fluidAndShape = new FBO(shCol, flWidth, flHeight,
	//                                GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER);
	//        normFbo->clear();

	// --- Texturen fuer den Litsphere Shader  ---

	litsphereTex = new TextureManager();
	litsphereTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/Unknown-28.jpeg");

	bumpMap = new TextureManager();
	bumpMap->loadTexture2D(*scd->dataPath+"/textures/bump_maps/Unknown-2.jpeg");

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube( ((*scd->dataPath)+"textures/skybox_camch.png").c_str() );

}

//---------------------------------------------------------------

void SNCamchLiquid::initShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	uniform mat4 m_pvm;\n

	out TO_FS {
		vec2 tex_coord;\n
	} vertex_out;

	void main() {
		vertex_out.tex_coord = texCoord;
		gl_Position = m_pvm * position;
	});

	vert = "// basic texture shader, vert\n" + shdr_Header +vert;



	std::string frag = STRINGIFY(
			in TO_FS {
		vec2 tex_coord;\n
	} vertex_in;

	layout (location = 0) out vec4 color;\n

	uniform sampler2D tex;
	uniform float alpha;

	void main() {
		color = vec4(texture(tex, vertex_in.tex_coord).rgb, alpha);
	});

	frag = "// basic texture shader, frag\n" +shdr_Header +frag;

	texShdr = shCol->addCheckShaderText("SNCamchLiquid_tex", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNCamchLiquid::initFluidHeightShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;

	uniform sampler2D rgbHeightMap;

	uniform mat4 m_proj;
	uniform mat4 m_vm;

	uniform float heightOffs;
	uniform float heightScale;

	out TO_FS {
		vec2 tex_coord;
		vec3 eye_pos;
		float height;
	} vertex_out;

	void main()
	{

		vec4 rgbHmPix = texture(rgbHeightMap, texCoord);
		float height = (rgbHmPix.r + rgbHmPix.g + rgbHmPix.b) * heightScale;
		vec4 modPos = vec4(position.xy, height, 1.0);

		vertex_out.tex_coord = texCoord;
		vertex_out.height = height;
		vertex_out.eye_pos = normalize( vec3( m_vm * modPos ) );

		// gl_Position = m_pvm * vec4(position.xy * scaleXY, min(height, 0.5), 1.0);
		gl_Position = m_proj * m_vm * vec4(modPos.xy, modPos.z + heightOffs, 1.0);
	});

	stdVert = "// SNTFluidDepthHeightMapfluid Heightmap vertex shader\n" +shdr_Header +stdVert;

	std::string frag = STRINGIFY(
			uniform sampler2D rgbHeightMap;
	uniform sampler2D normMap;
	uniform sampler2D litSphereTex;
	uniform samplerCube cubeMap;

	uniform mat3 m_normal;
	uniform float reflAmt;
	uniform float brightScale;

	const float Eta = 0.15; // Water

	in TO_FS {
		vec2 tex_coord;
		vec3 eye_pos;
		float height;
	} vertex_in;

	vec4 orgCol;
	vec4 litColor;
	float outVal;

	layout (location = 0) out vec4 color;

	void main()
	{
		vec3 n = normalize(m_normal * texture(normMap, vertex_in.tex_coord).xyz);

		// litsphere
		vec3 reflection = reflect( vertex_in.eye_pos, n );
		vec3 refraction = refract( vertex_in.eye_pos, n, Eta );

		float m = 2.0 * sqrt(
				pow( reflection.x, 2.0 ) +
				pow( reflection.y, 2.0 ) +
				pow( reflection.z + 1.0, 2.0 )
		);
		vec2 vN = reflection.xy / m + 0.5;

		orgCol = texture(rgbHeightMap, vertex_in.tex_coord);
		float orgBright = dot(orgCol.rgb, vec3(0.2126, 0.7152, 0.0722));

		litColor = texture(litSphereTex, vN);
		float litBright = litColor.r * litColor.g * litColor.b *1.5;

		vec4 reflectionCol = texture( cubeMap, reflection );
		vec4 refractionCol = texture( cubeMap, refraction );

		float fresnel = Eta + (1.0 - Eta) * pow(max(0.0, 1.0 - dot(-vertex_in.eye_pos, n)), 5.0);

		vec4 reflCol = reflAmt * mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0));
		reflCol.a *= min(orgBright, 0.05) / 0.05;
		//                                         reflCol.a *= vertex_in.height;

		color = orgCol * litBright * brightScale + reflCol;
	});

	frag = "// SNTFluidDepthHeightMap fluid Heightmap fragment shader\n"+shdr_Header+frag;

	fluidHeightShdr = shCol->addCheckShaderText("SNCamchLiquid_hm", stdVert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNCamchLiquid::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// translate model back to origin
	glm::mat4 rotMatr = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, heightOffs))
	* glm::scale(glm::mat4(1.f), glm::vec3(propo * fluidScaleX,
			fluidScaleY,
			1.f));

	glm::mat3 normalMat = glm::mat3( glm::transpose( glm::inverse( rotMatr ) ) );

	glm::mat4 vm = cp->view_matrix_mat4 * rotMatr;

	fluidHeightShdr->begin();
	fluidHeightShdr->setUniformMatrix4fv("m_proj", cp->projection_matrix);
	fluidHeightShdr->setUniformMatrix4fv("m_vm", &vm[0][0]);
	fluidHeightShdr->setUniformMatrix3fv("m_normal", &normalMat[0][0]);
	fluidHeightShdr->setUniform1f("heightOffs", heightOffs);
	fluidHeightShdr->setUniform1f("heightScale", heightScale);
	fluidHeightShdr->setUniform1f("reflAmt", reflAmt);
	fluidHeightShdr->setUniform1f("brightScale", brightScale);
	fluidHeightShdr->setUniform1i("rgbHeightMap", 0);
	fluidHeightShdr->setUniform1i("normMap", 1);
	fluidHeightShdr->setUniform1i("litSphereTex", 2);
	fluidHeightShdr->setUniform1i("cubeMap", 3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

	glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, blur->getResult());
	glBindTexture(GL_TEXTURE_2D, normFbo->getColorImg());

	litsphereTex->bind(2);
	cubeTex->bind(3);

	hmGrid->draw();
	//rawQuad->draw();

	/*
        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        texShdr->begin();
        texShdr->setUniformMatrix4fv("m_pvm", cp->mvp);
        texShdr->setUniform1i("tex", 0);
        texShdr->setUniform1f("alpha", alpha"));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blur->getResult() );
//        glBindTexture(GL_TEXTURE_2D, normFbo->getColorImg());
//        glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

        quad->draw();
        //fluidSim->draw();
        //fluidSim->drawVelocity();
	 */
}

//---------------------------------------------------------------

glm::vec2 SNCamchLiquid::toFluidCoord(glm::vec2 normPos)
{
	// convert from -1 | 1 to 0 | 5
	glm::vec2 out = normPos * 0.5f + 0.5f;
	// scale with fluidSize
	out *= fluidSize;
	return out;
}

//---------------------------------------------------------------

void SNCamchLiquid::update(double time, double dt)
{
	fluidSim->dissipation = diss;
	fluidSim->velocityDissipation =  veldiss;
	fluidSim->setTimeStep( timeStep );
	fluidSim->setSmokeWeight( bouyWeight );
	fluidSim->setSmokeBuoyancy( bouy );
	fluidSim->setUseBuoyancy(true);



	oscPhase += dt * oscSpeed; // muss so sein, sonst ungewollte bewegungen, wenn sich die Geschwindigkeit aendert
	float tm = static_cast<float>(std::sin(oscPhase)) * oscAmt;

	float velBlendPos = velBlendPos;
	float forceScale = velScale;

	int nrEm = static_cast<int>( nrEmit );

	for (int j=0;j<nrEm;j++)
	{
		oldPos[j] = forcePos[j];

		forcePos[j] = glm::vec2( xPos, 0.f );
		// distribute from y=-1 to y = 1
				float fInd = static_cast<float>(j) / static_cast<float>(nrEm);
				forcePos[j].y = (fInd * 2.f - 1.f) * yScale;

				// Adding temporal Force, in pixel relative to flWidth and flHeight
				// range -0.5 | 0.5
				perlinPhase += dt * posRandSpd;
				glm::vec2 perlOffs = glm::vec2(
						glm::perlin( glm::vec4(perlinPhase, 0.2f, fInd, fInd) ),
						glm::perlin( glm::vec4(perlinPhase * 1.4f, 0.1f, fInd, fInd) ) ) * 2.f;

				forcePos[j] += glm::vec2(posRandAmtX * perlOffs.x,
						posRandAmtY * perlOffs.y);

				forcePos[j].y += tm; // add oscillation


				glm::vec3 col = colSpline->sampleAt( static_cast<float>( std::sin(time * colorSpd) * 0.5 + 0.5 ) );
				float drawAlpha = drawAlpha;
				glm::vec4 forceCol = glm::vec4(col * drawAlpha, drawAlpha);
				float radRandScale = glm::perlin( glm::vec4(perlinPhase * 1.5f, 0.4f, fInd, fInd) );

				// emit always 3 forces spread in 45 Deg
				int sphereSegs = 3;
				glm::vec2 d = forcePos[j] - oldPos[j];

				float velLength = glm::length(d);
				d = glm::normalize(d);

				double angle = std::atan2(d.y, d.x);

				// add not only in one direction but spherical
				for (int i=0;i<sphereSegs;i++)
				{
					// vary angle
					float fInd = static_cast<float>(i) / static_cast<float>(sphereSegs);
					float newAngle = angle + (fInd - 0.5f) * M_PI * 0.5f; // angle variation

					glm::vec2 newD = glm::vec2( float( std::cos(newAngle) ), float( std::sin(newAngle) ) ) * forceScale;
					newD *= forceScale;
					newD *= velLength;

					fluidSim->addTemporalForce(toFluidCoord(forcePos[j]),                  	// pos
							glm::mix(newD, toLeft * forceScale, velBlendPos),			 // vel
							forceCol, // col
							rad + radRandScale * 0.7f);							// rad
				}
	}

	fluidSim->update();



	// generate normals
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	normFbo->bind();
	normFbo->clear();

	normShader->begin();
	normShader->setUniform1i("heightMap", 0);
	normShader->setUniform2f("texGridStep", 1.f / float(flWidth), 1.f / float(flHeight));
	normShader->setUniform1f("heightFact", normHeightAdj);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

	rawQuad->draw();

	normFbo->unbind();

	blur->setAlpha(blurAlpha);
	blur->proc(normFbo->getColorImg());
}

//---------------------------------------------------------------

void SNCamchLiquid::onKey(int key, int scancode, int action, int mods)
{}

//---------------------------------------------------------------

void SNCamchLiquid::onCursor(double xpos, double ypos)
{}

//---------------------------------------------------------------

SNCamchLiquid::~SNCamchLiquid()
{
	fluidSim->cleanUp();
	delete fluidSim;
	delete circle;
	delete colShader;
}

}
