//
//  GLSLParticleSystemFbo.cpp
//  tav_core
//
//  Created by Sven Hahne on 15/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//
//  Die Partikel werden alle in einer Textur gespeichert
//  Aktualisiert wird das System über einen PingPongFBO mit 8 Attachments
//  [0 / TBT_POSITION] = Position
//  [1 / TBT_VELOCITY] = Velocity
//  [2 / TBT_COLOR] = Farbe
//  [3 / TBT_AUX0] = x:lifeTime, y:Size, z: angle, w: texNr
//
//  zum emittieren gibt es entsprechend der Anzahl an Fbo-Attachments TexturBuffers
//  diese Buffer sind genauso gross wie die Textur, aber eindimensional
//  zum emittieren werden diese TexturBuffer gebunden und direkt in den PingPongFbo geschrieben
//  
//  es gibt verschiedene Möglichkeiten zu emittieren:
//  1: über emitData
//  2: über positions und velocity texturen
//
//  new particles are written gridwise. for each gridcell there is a counter which
//  offsets the writing position within the cell in x,y. if the counter reaches
//  the beginning, existing data is overwritten. ...brute force, but the fastest way
//

#include "pch.h"
#include "GLSLParticleSystemFbo.h"

#define STRINGIFY(A) #A

namespace tav
{
// emitGridSize MUSS VON HAND MIT DEM GEOMETRY SHADER ABGEGLICHEN WERDEN == max_vertices = nrPartPerCell
// AUCH WENN DIE IMPLEMENTATION IN DEBIAN 1024 ALS GL_MAX_GEOMETRY_OUTPUT_VERTICES rausgibt
// sind nur 128 moeglich
GLSLParticleSystemFbo::GLSLParticleSystemFbo(ShaderCollector* _shCol,
		int _nrParticles, float _scrAspect) :
		shCol(_shCol), nrAttachments(TBT_COUNT), lastNrEmitPart(0), nrTexBufCoord(4),
		scrAspect(_scrAspect), maxGeoAmpPoints(64), maxGeoAmpTriStrip(70),
		doAging(false), maxNrCellSqrtQuadAmp(2)
{
	initUpdateShdr();

	// get gl version
	GLint majVer;
	GLint minVer;
	glGetIntegerv(GL_MAJOR_VERSION, &majVer);
	glGetIntegerv(GL_MAJOR_VERSION, &minVer);
	if (majVer >= 4 && minVer >= 2)
		useAtomicCounters = true;

	if (useAtomicCounters)
	{
		glGenBuffers(1, &atomicBuffer);
		if (atomicBuffer <= 0)
			printf("could not create atomic buffer \n");

		// bind the buffer and define its initial storage capacity
		GLuint a[1] =
		{ 0 };
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), a,
				GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}

	// das wäre das Maximum, was der GeoShader an Geometry amplifizieren kann
	// heisst, aber nicht, dass das das Schnellste ist. besser von Hand tunen.
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &glMaxGeoAmpPoints);
	printf("GL_MAX_GEOMETRY_OUTPUT_VERTICES: %d ", glMaxGeoAmpPoints);

	maxGeoAmpPoints = std::min(maxGeoAmpPoints, glMaxGeoAmpPoints - 1);
	printf("maxGeoAmpPoints: %d ", maxGeoAmpPoints);

	stdColShdr = _shCol->getStdCol();
	stdTexShdr = _shCol->getStdTex();

	initPartTex(&basePTex, _nrParticles, maxGeoAmpPoints);

	// emit TextureBuffers
	GLfloat* initData = new GLfloat[basePTex.maxNrPart * nrTexBufCoord];
	memset(initData, 0, basePTex.maxNrPart * nrTexBufCoord);

	emitBufs = new TextureBuffer*[nrAttachments];
	for (short i = 0; i < TBT_COUNT; i++)
		emitBufs[i] = new TextureBuffer(basePTex.maxNrPart, nrTexBufCoord,
				initData);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);

	// check if there are enough texture units:
	GLint texture_units;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
	if (texture_units < TBT_COUNT + 1)
		printf(
				"GLSLParticleSystemFbo Warning: not enough texture units for external velocity textures. Max: %d Needed: %d",
				texture_units, TBT_COUNT + 1);

	emitTexIndices = std::vector<GLuint>();
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initPartTex(partTex* ptex,
		unsigned int _nrParticles, unsigned int _maxGeoAmpPoints)
{
	// get cell size
	float divisor = 2.f;
	while (divisor < (static_cast<float>(_maxGeoAmpPoints) / divisor))
		divisor += 2.f;

	ptex->cellSize.x = divisor;
	ptex->cellSize.y =
			static_cast<unsigned int>(static_cast<float>(_maxGeoAmpPoints)
					/ divisor);
	//  std::cout << "ptex->cellSize : " << glm::to_string(ptex->cellSize) << std::endl;

	ptex->nrPartPerCell = ptex->cellSize.x * ptex->cellSize.y;
	//  std::cout << "ptex->nrPartPerCell : " << ptex->nrPartPerCell << std::endl;

	ptex->fNrPartPerCell = static_cast<float>(ptex->nrPartPerCell);

	// berechne die anzahl benötigter zellen, nächst höheres vielfaches von 2
	ptex->totNrCells = static_cast<unsigned int>(std::ceil(
			static_cast<float>(_nrParticles)
					/ static_cast<float>(ptex->nrPartPerCell)));
	ptex->totNrCells = static_cast<unsigned int>(std::ceil(
			static_cast<float>(ptex->totNrCells * 0.5f)) * 2.f);
	//  std::cout << " temp ptex->totNrCells : " << ptex->totNrCells << std::endl;

	// anzahl zellen in x und y aufteilen
	divisor = 2.f;
	while (divisor < (static_cast<float>(ptex->totNrCells) / divisor))
		divisor += 2.f;

	ptex->nrCells.x = divisor;
	ptex->nrCells.y = static_cast<unsigned int>(std::ceil(
			static_cast<float>(ptex->totNrCells) / divisor));
	//  std::cout << "ptex->nrCells : " << glm::to_string(ptex->nrCells) << std::endl;

	ptex->fNrCells = glm::vec2(ptex->nrCells);
//    std::cout << "ptex->fNrCells : " << glm::to_string(ptex->fNrCells) << std::endl;

	ptex->totNrCells = ptex->nrCells.x * ptex->nrCells.y;
	//   std::cout << "fin ptex->totNrCells : " << ptex->totNrCells << std::endl;

	ptex->maxNrPart = ptex->totNrCells * ptex->nrPartPerCell;
	//   std::cout << "fin ptex->maxNrPart : " << ptex->maxNrPart << std::endl;

	ptex->texSize = ptex->nrCells * ptex->cellSize;
//    std::cout << "ptex->texSize : " << glm::to_string(ptex->texSize) << std::endl;

	ptex->fTexSize = glm::vec2(ptex->texSize);
	//   std::cout << "ptex->fTexSize : " << glm::to_string(ptex->fTexSize) << std::endl;

	ptex->fTexSizeInv = glm::vec2(1.f / ptex->fTexSize.x,
			1.f / ptex->fTexSize.y);
//    std::cout << "ptex->fTexSizeInv : " << glm::to_string(ptex->fTexSizeInv) << std::endl;

	//---- write offsets ------------------------------------------------

	// baue einen TexturBuffer mit nur einer Ebene für die SchreibOffsets der Zellen
	ptex->cellOffsTexBuf = new TextureBuffer(ptex->totNrCells, 1);

	// die Host-Repräsentation der Schreiboffsets
	ptex->cellWriteOffs = new unsigned int[ptex->totNrCells];
	memset(ptex->cellWriteOffs, 0, ptex->totNrCells * sizeof(unsigned int));

	ptex->fCellWriteOffs = new float[ptex->totNrCells];
	memset(ptex->fCellWriteOffs, 0, ptex->totNrCells * sizeof(float));

	ptex->cellPtr = glm::ivec2(0, 0);

	//---- Inst Quads ------------------------------------------------

	instQuads = new Quad(-0.1f, -0.1f, 0.1f, 0.1f, glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	//---- Texturen ------------------------------------------------

	// baue den PingPongBuffer, der letzen Endes die Texturen enthaelt
	// GL_RGBA16F ist leider zu ungenau...
	ptex->ppBuf = new PingPongFbo(shCol, ptex->texSize.x, ptex->texSize.y,
			GL_RGBA32F,
			GL_TEXTURE_2D, false, nrAttachments, 1, GL_CLAMP_TO_EDGE);
	ptex->ppBuf->setMinFilter(GL_NEAREST);
	ptex->ppBuf->setMagFilter(GL_NEAREST);
	ptex->ppBuf->src->clear();
	ptex->ppBuf->dst->clear();

	//---- VAOs ------------------------------------------------

	// VAO mit den Daten der neuen Partikel, wenn per Data emittiert wird
	ptex->emitDataVao = new VAO(
			"position:4f,velocity:4f,color:4f,aux0:4f,aux1:4f",
			GL_DYNAMIC_DRAW);
	ptex->emitDataVao->initData(ptex->maxNrPart);

	// VAO mit den Trigger, die den aktuellen Schreibpositionen der Zellen entsprechen
	ptex->cellStep.x = 1.f / static_cast<float>(ptex->nrCells.x);
	ptex->cellStep.y = 1.f / static_cast<float>(ptex->nrCells.y);
	// std::cout << "ptex->cellStep : " << glm::to_string(ptex->cellStep) << std::endl;

	// relevant ist immer die Mitte des Pixels, deswegen skalierung um groesse - 1pix
	// und offset um 1/2 pixel
	ptex->texIndScaleF = glm::vec2((ptex->fTexSize.x - 1.0) / ptex->fTexSize.x,
			(ptex->fTexSize.y - 1.0) / ptex->fTexSize.y);
	//  std::cout << "ptex->texIndScaleF : " << glm::to_string(ptex->texIndScaleF) << std::endl;

	ptex->texIndOffs = 0.5f / ptex->fTexSize;
	//  std::cout << "ptex->texIndOffs : " << glm::to_string(ptex->texIndOffs) << std::endl;

	//---- trigVaoPoints ------------------------------------------------

	// emitPos beziehen sich auf textur koordinaten in integer deshalb ([0|tex_width], [0|tex_height])
	GLfloat initEmitPos[ptex->totNrCells * 4];
	for (int y = 0; y < ptex->nrCells.y; y++)
	{
		for (int x = 0; x < ptex->nrCells.x; x++)
		{
			initEmitPos[(y * ptex->nrCells.x + x) * 4] = static_cast<float>(x
					* ptex->cellSize.x);
			initEmitPos[(y * ptex->nrCells.x + x) * 4 + 1] =
					static_cast<float>(y * ptex->cellSize.y);
			initEmitPos[(y * ptex->nrCells.x + x) * 4 + 2] = 0.f;
			initEmitPos[(y * ptex->nrCells.x + x) * 4 + 3] = 1.f;
		}
	}

	ptex->trigVaoPoints = new VAO("position:4f", GL_DYNAMIC_DRAW);
	ptex->trigVaoPoints->initData(ptex->totNrCells, initEmitPos);

	//---- trigVaoQuads -----------------------------------------------
	// hierfuer braucht es eine andere zellen groesse
	// pro trig werden maximal 4 vertices emitiert (tri-strip quad)

	// get cell size
	divisor = 2.f;
	while (divisor < (static_cast<float>(maxGeoAmpTriStrip / 4) / divisor))
		divisor += 2.f;

	ptex->tqCellSize.x = divisor;
	ptex->tqCellSize.y =
			static_cast<unsigned int>(static_cast<float>(maxGeoAmpTriStrip / 4)
					/ divisor);
	// std::cout << "ptex->tqCellSize : " << glm::to_string(ptex->tqCellSize) << std::endl;

	ptex->nrPartPerTqCell = ptex->tqCellSize.x * ptex->tqCellSize.y;
	// std::cout << "ptex->nrPartPerTqCell : " << ptex->nrPartPerTqCell << std::endl;

	ptex->fNrPartPerTqCell = static_cast<float>(ptex->fNrPartPerTqCell);

	// berechne die anzahl benötigter zellen, nächst höheres vielfaches von 2
	ptex->totNrTqCells = static_cast<unsigned int>(std::ceil(
			static_cast<float>(_nrParticles)
					/ static_cast<float>(ptex->nrPartPerTqCell)));
	ptex->totNrTqCells = static_cast<unsigned int>(std::ceil(
			static_cast<float>(ptex->totNrTqCells * 0.5f)) * 2.f);
	// std::cout << " temp ptex->totNrTqCells : " << ptex->totNrTqCells << std::endl;

	// anzahl zellen in x und y aufteilen
	divisor = 2.f;
	while (divisor < (static_cast<float>(ptex->totNrTqCells) / divisor))
		divisor += 2.f;

	ptex->nrTqCells.x = divisor;
	ptex->nrTqCells.y = static_cast<unsigned int>(std::ceil(
			static_cast<float>(ptex->totNrTqCells) / divisor));
	// std::cout << "ptex->nrCells : " << glm::to_string(ptex->nrTqCells) << std::endl;

	// emitPos beziehen sich auf textur koordinaten in integer deshalb ([0|tex_width], [0|tex_height])
	GLfloat initTqEmitPos[ptex->totNrTqCells * 4];
	for (int y = 0; y < ptex->nrTqCells.y; y++)
	{
		for (int x = 0; x < ptex->nrTqCells.x; x++)
		{
			initTqEmitPos[(y * ptex->nrTqCells.x + x) * 4] =
					static_cast<float>(x * ptex->tqCellSize.x);
			initTqEmitPos[(y * ptex->nrTqCells.x + x) * 4 + 1] =
					static_cast<float>(y * ptex->tqCellSize.y);
			initTqEmitPos[(y * ptex->nrTqCells.x + x) * 4 + 2] = 0.f;
			initTqEmitPos[(y * ptex->nrTqCells.x + x) * 4 + 3] = 1.f;
		}
	}

	ptex->trigVaoQuads = new VAO("position:4f", GL_DYNAMIC_DRAW);
	ptex->trigVaoQuads->initData(ptex->totNrTqCells, initTqEmitPos);
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::update(double time)
{
	// get dt and smooth it, -> verändert sich stark bei Programmstart
	if (medDt == 0.0)
	{
		if (lastTime != 0.0)
			medDt = time - lastTime;
	}
	else
	{
		medDt = ((time - lastTime) + (medDt * 20.0)) / 21.0;
	}
	lastTime = time;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	basePTex.ppBuf->dst->bind();
	basePTex.ppBuf->dst->clear(); // kann nicht wegoptimiert werden... und loeschen per shader ist auch nicht schneller

	updateShader->begin();
	updateShader->setUniform1f("dt", (float) medDt);
	updateShader->setUniform1f("doAging", (float) doAging);
	updateShader->setUniform1f("doAgeFading", ageFading);
	updateShader->setUniform1f("ageFadeCurv", actEmitData->ageFadeCurv);
	updateShader->setUniform1f("friction", friction);
	updateShader->setUniform1f("lifeTime", lifeTime);

	// bind particle data
	for (short i = 0; i < TBT_COUNT; i++)
	{
		updateShader->setUniform1i(samplerNames[i], i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, basePTex.ppBuf->getSrcTexId(i));
	}

	quad->draw();

	basePTex.ppBuf->dst->unbind();
	basePTex.ppBuf->swap();
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::update(double time, GLint velTex, GLint extForceTex)
{
	// get dt and smooth it, -> verändert sich stark bei Programmstart
	if (medDt == 0.0)
	{
		if (lastTime != 0.0)
			medDt = time - lastTime;
	}
	else
	{
		medDt = ((time - lastTime) + (medDt * 20.0)) / 21.0;
	}
	lastTime = time;

	if (!updateVelTexShader)
		initVelTexShdr();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	basePTex.ppBuf->dst->bind();
	basePTex.ppBuf->dst->clear(); // kann nicht wegoptimiert werden... und loeschen per shader ist auch nicht schneller

	updateVelTexShader->begin();
	updateVelTexShader->setIdentMatrix4fv("m_pvm");
	updateVelTexShader->setUniform1f("dt", (float) medDt);
	updateVelTexShader->setUniform1f("doAging", (float) doAging);
	updateVelTexShader->setUniform1f("doAgeFading", ageFading);
	updateVelTexShader->setUniform1f("friction", friction);
	updateVelTexShader->setUniform1f("lifeTime", lifeTime);
	updateVelTexShader->setUniform1f("extVelScale", 0.25f);
	updateVelTexShader->setUniform2f("extFOffs", 0.0, 0.0);
	updateVelTexShader->setUniform1f("extVelAmt", actEmitData->extVelAmt);
	updateVelTexShader->setUniform1f("ageFadeCurv", actEmitData->ageFadeCurv);
//        updateVelTexShader->setUniform2f("extFOffs", time * 0.5, time * 0.5);

	// bind particle data
	for (short i = 0; i < TBT_COUNT; i++)
	{
		updateVelTexShader->setUniform1i(samplerNames[i], i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, basePTex.ppBuf->getSrcTexId(i));
	}

	updateVelTexShader->setUniform1i("extVel_tex", TBT_COUNT);
	glActiveTexture(GL_TEXTURE0 + TBT_COUNT);
	glBindTexture(GL_TEXTURE_2D, velTex);

	if (extForceTex != -1)
	{
		updateVelTexShader->setUniform1i("extForce_tex", TBT_COUNT + 1);
		glActiveTexture(GL_TEXTURE0 + TBT_COUNT + 1);
		glBindTexture(GL_TEXTURE_2D, extForceTex);
	}

	quad->draw();

	basePTex.ppBuf->dst->unbind();
	basePTex.ppBuf->swap();
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::bindTexBufs(Shaders* _shader)
{
	// bind particle data
	for (short i = 0; i < TBT_COUNT; i++)
	{
		_shader->setUniform1i(samplerNames[i], i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, basePTex.ppBuf->getSrcTexId(i));
	}
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::draw()
{
	basePTex.trigVaoPoints->draw(POINTS);
}

//------------------------------------------------------------------------

// muss eine m_pvm matrix sein, die hier reinkommt!!!
void GLSLParticleSystemFbo::draw(GLfloat* matPtr)
{
	if (!drawShader)
		initDrawPointShdr();

	drawShader->begin();
	drawShader->setUniformMatrix4fv("m_pvm", matPtr);
	drawShader->setUniform2i("cellSize", basePTex.cellSize.x,
			basePTex.cellSize.y);
	drawShader->setUniform1f("velBright", veloBright);

	// bind particle data
	for (short i = 0; i < TBT_COUNT; i++)
	{
		drawShader->setUniform1i(samplerNames[i], i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, basePTex.ppBuf->getSrcTexId(i));
	}

	basePTex.trigVaoPoints->draw(POINTS);
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::drawMult(int nrCam, glm::mat4* _modMatr,
		glm::vec4* _viewPorts)
{
	if (!drawMultShader)
		initDrawMultShdr();

	for (short i = 0; i < nrCam; i++)
		glViewportIndexedf(i, _viewPorts[i].x, _viewPorts[i].y, _viewPorts[i].z,
				_viewPorts[i].w);      // left top

	drawMultShader->begin();
	drawMultShader->setUniformMatrix4fv("model_matrix_g", &_modMatr[0][0][0],
			nrCam);
	//drawMultShader->setUniform1i("pixPerGrid", nrPartPerCellSqrt);
	drawMultShader->setUniform1f("invNrPartSqrt",
			1.f / static_cast<float>(nrPartSqrt));

	// bind particle data
	for (short i = 0; i < TBT_COUNT; i++)
	{
		drawMultShader->setUniform1i(samplerNames[i], i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, basePTex.ppBuf->getSrcTexId(i));
	}

	basePTex.trigVaoQuads->draw(POINTS);
}

//---- emit with different data for each particle ------------------------
//---- emitDataVao enthaelt schon maximale moegliche anzahl partikel -----
//---- kein neuskalieren noetigt -----------------------------------------
void GLSLParticleSystemFbo::emit(unsigned int nrPart)
{
	int nrEmitTbosCoords = 4;
	unsigned int newCellWriteOffs;
	unsigned int numCompWriteLoops;
	unsigned int ind;

	glm::ivec2 pixOffs;
	glm::vec4 emitCol;
	glm::vec3 tempCol;

	actEmitData->emitNrPart = std::min(nrPart, basePTex.maxNrPart);

	std::cout << "emit " << actEmitData->emitNrPart << std::endl;

	if (actEmitData)
	{
		// setze das "emit-flag" und initialisiere position
		GLfloat* emit = (GLfloat*) basePTex.emitDataVao->getMapBuffer(
				tav::POSITION);
		for (unsigned short i = 0; i < actEmitData->emitNrPart; i++)
		{
			ind = i * nrEmitTbosCoords;

			//  random, um rasterungen zu vermeiden
			glm::vec3 rEmitOrg = glm::vec3(
					actEmitData->emitOrg.x
							+ getRandF(-actEmitData->posRand,
									actEmitData->posRand),
					actEmitData->emitOrg.y
							+ getRandF(-actEmitData->posRand,
									actEmitData->posRand),
					actEmitData->emitOrg.z
							+ getRandF(-actEmitData->posRand,
									actEmitData->posRand));

			emit[ind] = rEmitOrg.x;
			emit[ind + 1] = rEmitOrg.y;
			emit[ind + 2] = 0.f;
			//emit[ind + 2] = rEmitOrg.z;
			emit[ind + 3] = 1.f;
		}
		basePTex.emitDataVao->unMapBuffer();

		// initialisiere die richtung
		GLfloat* emitVelPtr = (GLfloat*) basePTex.emitDataVao->getMapBuffer(
				tav::VELOCITY);
		for (unsigned int i = 0; i < actEmitData->emitNrPart; i++)
		{
			ind = i * nrEmitTbosCoords;
			glm::vec3 rEmitVel = glm::vec3(
					actEmitData->emitVel.x
							+ getRandF(-actEmitData->dirRand,
									actEmitData->dirRand),
					actEmitData->emitVel.y
							+ getRandF(-actEmitData->dirRand,
									actEmitData->dirRand),
					actEmitData->emitVel.z
							+ getRandF(-actEmitData->dirRand,
									actEmitData->dirRand));

			// necessary, causes errors, wenn inited with only zeros...
			if (rEmitVel.x == 0.f && rEmitVel.y == 0.f && rEmitVel.z == 0)
				rEmitVel = glm::vec3(0.f, 1.f, 0.f);

			rEmitVel = glm::normalize(rEmitVel)
					* (actEmitData->speed
							+ getRandF(0.f, actEmitData->speedRand));

			emitVelPtr[ind] = rEmitVel.x;
			emitVelPtr[ind + 1] = rEmitVel.y;
			emitVelPtr[ind + 2] = rEmitVel.z;
			emitVelPtr[ind + 3] = 1.f; // velocity has to be written with a > 0.f  (a == 0.f is ignored by the shader)
		}
		basePTex.emitDataVao->unMapBuffer();

		// init the colors
		GLfloat* emitColorPtr = (GLfloat*) basePTex.emitDataVao->getMapBuffer(
				tav::COLOR);
		for (unsigned int i = 0; i < actEmitData->emitNrPart; i++)
		{
			ind = i * nrEmitTbosCoords;

			if (actEmitData->colRand > 0.f)
			{
				tempCol = RGBtoHSV(actEmitData->emitCol.r,
						actEmitData->emitCol.g, actEmitData->emitCol.b);

				tempCol.r = std::min(
						std::max(
								tempCol.r
										+ getRandF(-actEmitData->colRand,
												actEmitData->colRand), 0.f),
						1.f);

				tempCol = HSVtoRGB(tempCol.r, tempCol.g, tempCol.b);

				emitCol.r = tempCol.r;
				emitCol.g = tempCol.g;
				emitCol.b = tempCol.b;

				emitCol.a = 1.f - getRandF(0.f, actEmitData->colRand);
			}
			else
			{
				emitCol = actEmitData->emitCol;
			}

			emitColorPtr[ind] = emitCol.r;
			emitColorPtr[ind + 1] = emitCol.g;
			emitColorPtr[ind + 2] = emitCol.b;
			emitColorPtr[ind + 3] = emitCol.a;
		}
		basePTex.emitDataVao->unMapBuffer();

		// initialisiere die aux0 parameter
		GLfloat* emitAuxPtr = (GLfloat*) basePTex.emitDataVao->getMapBuffer(
				tav::AUX0);
		for (unsigned short i = 0; i < actEmitData->emitNrPart; i++)
		{
			ind = i * nrEmitTbosCoords;

			emitAuxPtr[ind] = lifeTime;
			emitAuxPtr[ind + 2] = actEmitData->angle
					+ getRandF(-actEmitData->angleRand, actEmitData->angleRand)
							* M_PI_2;

			emitAuxPtr[ind + 1] = actEmitData->size
					* std::pow(
							getRandF(1.f - actEmitData->sizeRand,
									1.f + actEmitData->sizeRand), 2.0);

			emitAuxPtr[ind + 3] = float(
					int(
							float(actEmitData->texNr)
									+ getRandF(0.f, actEmitData->texRand)
											* float(actEmitData->maxNrTex))
							% std::max(actEmitData->maxNrTex, 1));

		}
		basePTex.emitDataVao->unMapBuffer();

		// Setze die Schreibpositionen in der Partikel-Textur
		GLfloat* emitAux1Ptr = (GLfloat*) basePTex.emitDataVao->getMapBuffer(
				tav::AUX1);
		for (unsigned int i = 0; i < actEmitData->emitNrPart; i++)
		{
			ind = i * nrEmitTbosCoords;

			// berechne die Zellen-Position (bei jedem Partikel eine Zelle weiter)
			pixOffs = glm::ivec2((basePTex.cellPtr.x + i) % basePTex.nrCells.x,
					(basePTex.cellPtr.y + (i / basePTex.nrCells.x))
							% basePTex.nrCells.y);

			// hole den aktuellen schreibeoffset der zelle
			newCellWriteOffs = basePTex.cellWriteOffs[pixOffs.y
					* basePTex.nrCells.x + pixOffs.x];

			// berechne wie oft alle zellen bis zu diesem Pixel durchlaufen wurden
			numCompWriteLoops = i / basePTex.totNrCells;

			// addiere das auf den aktuellen schreiboffset dazu
			newCellWriteOffs += numCompWriteLoops;
			newCellWriteOffs %= basePTex.nrPartPerCell;

			// konvertiere die Zellenposition in eine Pixelposition
			pixOffs *= basePTex.cellSize;

			// addiere Zellen-SchreibOffset
			pixOffs.x += newCellWriteOffs % basePTex.cellSize.x;
			pixOffs.y += (newCellWriteOffs / basePTex.cellSize.x)
					% basePTex.cellSize.y;

			// konvertiere die Pixel-Position in eine Normalisierte Position
			emitAux1Ptr[ind] = static_cast<float>(pixOffs.x)
					/ basePTex.texSize.x + basePTex.texIndOffs.x;
			emitAux1Ptr[ind] = emitAux1Ptr[ind] * 2.f - 1.f;
			emitAux1Ptr[ind + 1] = static_cast<float>(pixOffs.y)
					/ basePTex.texSize.y + basePTex.texIndOffs.y;
			emitAux1Ptr[ind + 1] = emitAux1Ptr[ind + 1] * 2.f - 1.f;
			emitAux1Ptr[ind + 2] = 0.f;
			emitAux1Ptr[ind + 3] = 1.f;
		}
		basePTex.emitDataVao->unMapBuffer();
	}

	procEmission();
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::emit(unsigned int nrPart, GLint emitTex, int width,
		int height, GLint velTex)
{
	bool uBlockUpdate = false;
	bool rewriteVao = false;

	if (!emitShaderTexRec)
		initEmitTexRecShdr();
	if (!emitShaderTex)
		initEmitTexShdr();

	if (actEmitData)
	{
		// es koennen nicht mehr partikel emittiert werden, als maximal vorhanden
		actEmitData->emitNrPart = std::min(nrPart, basePTex.maxNrPart);

		if (actEmitData->texNr != emitTex || actEmitData->width != width
				|| actEmitData->height != height
				|| actEmitData->emitNrPart != nrPart)
			uBlockUpdate = true;

		uBlockUpdate = true;

		actEmitData->width = width;
		actEmitData->height = height;
		actEmitData->texNr = emitTex;
		actEmitData->velTexNr = velTex;

		// es muss unbedingt die gesamte Textur erfasst werden
		// berechne wie viele Zellen dafür nötig sind
		if (std::ceil(actEmitData->width / basePTex.cellSize.x) != actEmitData->nrCells.x
				|| std::ceil(actEmitData->height / basePTex.cellSize.y) != actEmitData->nrCells.y)
		{
			actEmitData->nrCells = glm::ivec2(
					std::ceil(actEmitData->width / basePTex.cellSize.x),
					std::ceil(actEmitData->height / basePTex.cellSize.y));
			uBlockUpdate = true;
		}

		actEmitData->totNrCells = actEmitData->nrCells.x * actEmitData->nrCells.y;
		actEmitData->texIndOffs = glm::vec2(0.5f / actEmitData->width, 0.5f / actEmitData->height);

		// wenn der emitTexVao nicht initialisiert ist, initialisiere jetzt
		if (!basePTex.emitTexVao)
		{
			basePTex.emitTexVao = new VAO("position:4f,velocity:4f", GL_DYNAMIC_DRAW);
			basePTex.emitTexVao->initData(actEmitData->totNrCells);
			rewriteVao = true;

			// max emitTexturGroesse
			// prepare emitTexTfo par
			tfoBufAttachTypes.push_back(tav::POSITION);
			tfoBufAttachTypes.push_back(tav::VELOCITY);
			for (auto i = 0; i < int(tfoBufAttachTypes.size()); i++)
				recAttribNames.push_back( stdRecAttribNames[tfoBufAttachTypes[i]] );

			// initialisiere tfo mit der maximalen anzahl möglicher partikel, die aus der textur gelesen werden können
			basePTex.emitTexTfo = new TFO(actEmitData->width * actEmitData->height, recAttribNames);
			basePTex.emitTexTfo->setVaryingsToRecord(&recAttribNames, emitShaderTexRec->getProgram());
			glLinkProgram(emitShaderTexRec->getProgram());

			// initialisiere einen UniformBlock dafür
			emitShaderTexRecUBlock = new UniformBlock(emitShaderTexRec->getProgram(), "emitDataRec");
			emitShaderTexRecUBlock->addVarName("texWidth", &actEmitData->width, GL_INT);
			emitShaderTexRecUBlock->addVarName("texHeight", &actEmitData->height, GL_INT);
			emitShaderTexRecUBlock->addVarName("cellSize", &basePTex.cellSize[0], GL_INT_VEC2);
			emitShaderTexRecUBlock->addVarName("emitTexThres", &emitTexThres, GL_FLOAT);

			emitShaderTexUBlock = new UniformBlock(emitShaderTex->getProgram(), "emitData");
			emitShaderTexUBlock->addVarName("lifeTime", &lifeTime, GL_FLOAT);
			emitShaderTexUBlock->addVarName("posRand", &actEmitData->posRand, GL_FLOAT);
			emitShaderTexUBlock->addVarName("dirRand", &actEmitData->dirRand, GL_FLOAT);
			emitShaderTexUBlock->addVarName("emitVel", &actEmitData->emitVel[0], GL_FLOAT_VEC3);
			emitShaderTexUBlock->addVarName("speed", &actEmitData->speed, GL_FLOAT);
			emitShaderTexUBlock->addVarName("emitCol", &actEmitData->emitCol[0], GL_FLOAT_VEC4);
			emitShaderTexUBlock->addVarName("colRand", &actEmitData->colRand, GL_FLOAT);
			emitShaderTexUBlock->addVarName("angle", &actEmitData->angle, GL_FLOAT);
			emitShaderTexUBlock->addVarName("angleRand", &actEmitData->angleRand, GL_FLOAT);
			emitShaderTexUBlock->addVarName("size", &actEmitData->size, GL_FLOAT);
			emitShaderTexUBlock->addVarName("sizeRand", &actEmitData->sizeRand, GL_FLOAT);
			emitShaderTexUBlock->addVarName("texNr", &actEmitData->texNr, GL_INT);
			emitShaderTexUBlock->addVarName("texRand", &actEmitData->texRand, GL_FLOAT);
			emitShaderTexUBlock->addVarName("maxNrTex", &actEmitData->maxNrTex, GL_INT);
			emitShaderTexUBlock->addVarName("cellStep", &basePTex.cellStep[0], GL_FLOAT_VEC2);
			emitShaderTexUBlock->addVarName("cellSize", &basePTex.cellSize[0], GL_INT_VEC2);
			emitShaderTexUBlock->addVarName("nrCells", &basePTex.nrCells[0], GL_INT_VEC2);
			emitShaderTexUBlock->addVarName("texSize", &basePTex.fTexSize[0], GL_FLOAT_VEC2);
			emitShaderTexUBlock->addVarName("texOffs", &basePTex.texIndOffs[0], GL_FLOAT_VEC2);

			uBlockUpdate = true;
		}
		else
		{
			// wenn mehr emit Particle benoetigt werden, vergroessere den Buffer
			if (basePTex.emitTexVao->getNrVertices() < actEmitData->totNrCells)
			{
				basePTex.emitTexVao->resize(actEmitData->totNrCells);
				rewriteVao = true;
			}
		}

		// setze die EmitVao Positionen neu
		if (rewriteVao)
		{
			GLfloat* emitPos = (GLfloat*) basePTex.emitTexVao->getMapBuffer(tav::POSITION);

			for (int y = 0; y < actEmitData->nrCells.y; y++)
			{
				for (int x = 0; x < actEmitData->nrCells.x; x++)
				{
					unsigned int ind = (y * actEmitData->nrCells.x + x) * 4;

					emitPos[ind] = static_cast<float>(x) * static_cast<float>(basePTex.cellSize.x)
							/ actEmitData->width + actEmitData->texIndOffs.x;
					emitPos[ind + 1] = static_cast<float>(y) * static_cast<float>(basePTex.cellSize.y)
							/ actEmitData->height + actEmitData->texIndOffs.y;
					emitPos[ind + 2] = 0.f;
					emitPos[ind + 3] = 1.f;

					//std::cout << emitPos[ind] << ", " << emitPos[ind + 1]
					//		<< ", " << emitPos[ind + 2] << std::endl;
				}
			}

			basePTex.emitTexVao->unMapBuffer();
		}

		if (emitShaderTexRecUBlock && uBlockUpdate) emitShaderTexRecUBlock->update();
		if (emitShaderTexUBlock && uBlockUpdate) 	emitShaderTexUBlock->update();

		procEmissionTex();

		//lastNrEmitPart = actEmitData->emitNrPart;
	}
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::procEmission()
{
	if (!emitShader)
		initEmitShdr();

	glDisable(GL_BLEND);

	basePTex.ppBuf->src->bind();

	emitShader->begin();
	emitShader->setIdentMatrix4fv("m_pvm");
	basePTex.emitDataVao->draw(GL_POINTS, 0, actEmitData->emitNrPart, nullptr);

	basePTex.ppBuf->src->unbind();

	updateWriteOffsets();
}

//---------with emit texture ---------------------------------------------------------------

void GLSLParticleSystemFbo::procEmissionTex()
{
	primitives = 0;

	// ---- scan the whole emit texture -----
	// ---- and record the result -----------

	glDisable(GL_BLEND);
	glEnable(GL_RASTERIZER_DISCARD);

	if (useAtomicCounters)
	{
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);
	}
	else
	{
		if (query == 0) glGenQueries(1, &query);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
	}

	// record emited vertices
	emitShaderTexRec->begin();
	emitShaderTexRecUBlock->bind();
	emitShaderTexRec->setUniform1i("emitTex", 0);
	emitShaderTexRec->setUniform1f("time", lastTime);
	emitShaderTexRec->setUniform1i("useVelTex", int(actEmitData->velTexNr != 0));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, actEmitData->texNr);

	if (actEmitData->velTexNr != 0)
	{
		emitShaderTexRec->setUniform1i("velTex", 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, actEmitData->velTexNr);
	}

	basePTex.emitTexTfo->bind();
	basePTex.emitTexTfo->begin(GL_POINTS);
	basePTex.emitTexVao->draw(GL_POINTS, 0, actEmitData->totNrCells, basePTex.emitTexTfo);
	basePTex.emitTexTfo->end();
	basePTex.emitTexTfo->unbind();

	emitShaderTexRecUBlock->unbind();


	// get atomic counter result
	if (useAtomicCounters)
	{
		GLuint *userCounters;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		// again we map the buffer to userCounters, but this time for read-only access
		userCounters = (GLuint*) glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0,
				sizeof(GLuint),
				GL_MAP_READ_BIT);

		// copy the values to other variables because...
		primitives = userCounters[0];

		// ... as soon as we unmap the buffer
		// the pointer userCounters becomes invalid.
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	}
	else
	{
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
	}

//	printf("recorder %d \n", primitives);

	glDisable(GL_RASTERIZER_DISCARD);

	// ---- now playback the tfo and write the new emits to the ppBuf -----------
	if (primitives > 0)
	{
		glDisable(GL_BLEND);

		basePTex.ppBuf->src->bind();

		emitShaderTex->begin();
		emitShaderTexUBlock->bind();
		emitShaderTex->setIdentMatrix4fv("m_pvm");
		emitShaderTex->setUniform1i("cellOffs", 0);
		emitShaderTex->setUniform1f("time", lastTime);
		emitShaderTex->setUniform1i("useVelTex", int(actEmitData->velTexNr != 0));
		emitShaderTex->setUniform2iv("cellPtr", &basePTex.cellPtr[0]);

		glActiveTexture(GL_TEXTURE0);
		basePTex.cellOffsTexBuf->bindTex();

		//printf("primitives: %d actEmitData->emitNrPart %d \n", primitives, actEmitData->emitNrPart);

		// if there are more primitives recorded than needed
		if (primitives > actEmitData->emitNrPart)
		{
			// if the size of requested emits changes, change also the size of the indices
			if (emitTexNrIndices != std::min(std::max(primitives, actEmitData->emitNrPart), actEmitData->emitNrPart)
					|| lastNrPrimitives != primitives)
			{
				emitTexNrIndices = std::min( std::max(primitives, actEmitData->emitNrPart), actEmitData->emitNrPart);
				lastNrPrimitives = primitives;

				if (static_cast<unsigned int>(emitTexIndices.size()) < emitTexNrIndices)
					emitTexIndices.resize(emitTexNrIndices);

				// create a even distribution over the recorded emits
				float fEmitTexNrIndices = static_cast<float>(emitTexNrIndices);
				if (fEmitTexNrIndices > 0)
					emitIndStep = std::max( static_cast<int>(static_cast<float>(primitives) / fEmitTexNrIndices), 1);

				for (unsigned int i = 0; i < emitTexNrIndices; i++)
					emitTexIndices[i] = static_cast<unsigned int>((static_cast<float>(i) / fEmitTexNrIndices)
									* static_cast<float>(primitives));

				basePTex.emitTexTfo->setIndices(emitTexNrIndices, GL_UNSIGNED_INT, &emitTexIndices[0]);
			}

			//printf("emitLimOffs: %d \n", emitLimOffs);
			basePTex.emitTexTfo->drawElementsBaseVertex(GL_POINTS, emitTexNrIndices, GL_UNSIGNED_INT, emitLimOffs,
					nullptr, GL_POINTS, 1);

			// random offset um rasterungen zu vermeiden
			emitLimOffs = (emitLimOffs + static_cast<int>(getRandF(0.f, static_cast<float>(emitIndStep)))) % emitIndStep;
			actEmitData->emitNrPart = emitTexNrIndices;

		}
		else
		{
			// less primitives recorded than needed, copy the content of the recorded buffers
			// until there are enough. -> koennte auch per setzten derselben inidices in einen
			// elementBuffer gehen, im Shader haben diese dann aber alle diesselbe ID, so dass
			// die Random funktionen alle dasselbe Ergebnis liefern...
			// wenn mehr emit Particle benoetigt werden, vergroessere den Buffer

			for (int i = 0; i < int(tfoBufAttachTypes.size()); i++)
				if (basePTex.emitTexTfo->getTFOBufSize(tfoBufAttachTypes[i]) < actEmitData->emitNrPart)
					basePTex.emitTexTfo->resizeTFOBuf(tfoBufAttachTypes[i], actEmitData->emitNrPart);

			glBindBuffer(GL_COPY_READ_BUFFER, basePTex.emitTexTfo->getTFOBuf(POSITION));
			glBindBuffer(GL_COPY_WRITE_BUFFER, basePTex.emitTexTfo->getTFOBuf(POSITION));

			unsigned int availablePrims = primitives;
			GLintptr readOffset = 0;
			GLintptr writeOffset;
			GLsizei copySize;

			unsigned int fragSize = basePTex.emitTexTfo->getTFOBufFragSize(POSITION);

			while (availablePrims < actEmitData->emitNrPart)
			{
				writeOffset = availablePrims * fragSize * sizeof(GLfloat);
				copySize = std::min(actEmitData->emitNrPart - availablePrims, availablePrims) * fragSize * sizeof(GLfloat);

				glCopyBufferSubData(GL_COPY_READ_BUFFER,    // read
						GL_COPY_WRITE_BUFFER,   // write
						readOffset, writeOffset, copySize);

				availablePrims += std::min( actEmitData->emitNrPart - availablePrims, availablePrims);
			}

			glBindBuffer(GL_COPY_READ_BUFFER, 0);
			glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

			basePTex.emitTexTfo->draw(GL_POINTS, 0, actEmitData->emitNrPart, nullptr, GL_POINTS, 1);
		}

		emitShaderTexUBlock->unbind();
		basePTex.ppBuf->src->unbind();
		updateWriteOffsets();
	}

	//  glDeleteQueries(1, &query);

	if (useAtomicCounters)
	{

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
		GLuint a[1] = { 0 };
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), a);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}
}

//---------with emit texture ---------------------------------------------------------------

void GLSLParticleSystemFbo::procEmissionDepthTex(unsigned int _nrEmitPart)
{
	if (!emitShaderDepthTex)
		initEmitDepthTexShdr();

	glDisable(GL_BLEND);

	basePTex.ppBuf->src->bind();

	emitShaderDepthTex->begin();
	emitShaderDepthTex->setIdentMatrix4fv("m_pvm");
	emitShaderDepthTex->setUniform1i("emitTex", 0);
	emitShaderDepthTex->setUniform1i("depthTex", 1);
	//emitShaderDepthTex->setUniform1i("nrPartPerCellSqrt",		nrPartPerCellSqrt);
	emitShaderDepthTex->setUniform1i("maxEmit",
			std::max(int(float(_nrEmitPart) / 100.0), 1));
	emitShaderDepthTex->setUniform1i("texWidth", actEmitData->width);
	emitShaderDepthTex->setUniform1i("texHeight", actEmitData->height);
	emitShaderDepthTex->setUniform2f("invGridSizeF", basePTex.cellStep.x,
			basePTex.cellStep.y);
	emitShaderDepthTex->setUniform2i("gridSizeSqrt", basePTex.cellSize.x,
			basePTex.cellSize.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, actEmitData->texNr);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, actEmitData->depthTexNr);

	basePTex.emitDataVao->draw(GL_POINTS, 0, actEmitData->emitNrPart, nullptr);

	basePTex.ppBuf->src->unbind();
}

//------- update cellWriteOffs -------------------------------------------------

void GLSLParticleSystemFbo::updateWriteOffsets()
{
	glm::ivec2 actCellPtr;
	glm::ivec2 writePos;
	unsigned int numCompWriteLoops;
	unsigned int over;
	unsigned int absCellPtr;

	for (unsigned int i = 0;
			i < std::min(actEmitData->emitNrPart, basePTex.totNrCells); i++)
	{
		// bestimme dementsprechen die aktuelle Zelle
		absCellPtr = basePTex.cellPtr.x
				+ basePTex.nrCells.y * basePTex.nrCells.x;
		actCellPtr = glm::ivec2(
				(basePTex.cellPtr.x + (i % basePTex.nrCells.x))
						% basePTex.nrCells.x,
				(basePTex.cellPtr.y
						+ ((i / basePTex.nrCells.x) % basePTex.nrCells.y))
						% basePTex.nrCells.y);

		// berechne wie oft diese zelle durchlaufen wurde
		numCompWriteLoops = actEmitData->emitNrPart / basePTex.totNrCells;

		// berechne wie weit nach den kompletten durchläufen weitergesrungen wurde
		over = actEmitData->emitNrPart % basePTex.totNrCells;
		over = int(
				(i - absCellPtr + basePTex.totNrCells) % basePTex.totNrCells
						< over);

		// erhoehe den writeOffset fuer diese Zelle
		basePTex.cellWriteOffs[actCellPtr.x + actCellPtr.y * basePTex.nrCells.x] =
				(basePTex.cellWriteOffs[actCellPtr.x
						+ actCellPtr.y * basePTex.nrCells.x] + numCompWriteLoops
						+ over) % basePTex.nrPartPerCell;

		// convert to float for upload to TextureBuffer
		basePTex.fCellWriteOffs[actCellPtr.y * basePTex.nrCells.x + actCellPtr.x] =
				static_cast<float>(basePTex.cellWriteOffs[actCellPtr.y
						* basePTex.nrCells.x + actCellPtr.x]);
	}

	// erhoehe den zellen pointer
	basePTex.cellPtr =
			glm::ivec2(
					(basePTex.cellPtr.x
							+ (actEmitData->emitNrPart % basePTex.nrCells.x))
							% basePTex.nrCells.x,
					(basePTex.cellPtr.y
							+ ((actEmitData->emitNrPart / basePTex.nrCells.x)
									% basePTex.nrCells.y))
							% basePTex.nrCells.y);

	// copy the new offset to the texture buffer
	basePTex.cellOffsTexBuf->upload(basePTex.totNrCells,
			(GLfloat*) &basePTex.fCellWriteOffs[0], 0);
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setEmitData(GLSLParticleSystemFbo::EmitData* _data)
{
	actEmitData = _data;

	if (emitShaderTexRecUBlock) emitShaderTexRecUBlock->update();
	if (emitShaderTexUBlock)	emitShaderTexUBlock->update();
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setEmitTexThres(float _emitTexThres)
{
	emitTexThres = _emitTexThres;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setLifeTime(float _lifeTime)
{
	lifeTime = _lifeTime;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setGravity(glm::vec3 _gravity)
{
	gravity = _gravity;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setAging(bool _age)
{
	doAging = _age ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setFriction(float _friction)
{
	friction = 1.0f - pow(_friction, 4.f);
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setAgeFading(bool _set)
{
	ageFading = _set ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setAgeSizing(bool _set)
{
	ageSizing = _set ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::setColorPal(glm::vec4* chanCols)
{
	for (auto i = 0; i < MAX_NUM_COL_SCENE; i++)
		colorPal[i] = chanCols[i];
}

void GLSLParticleSystemFbo::setVeloBright(float _val)
{
	veloBright = _val;
}

//---------------------------------------------------------

// r,g,b values are from 0 to 1
glm::vec3 GLSLParticleSystemFbo::RGBtoHSV(float r, float g, float b)
{
	glm::vec3 out;
	double min, max, delta;

	r *= 100.f;
	g *= 100.f;
	b *= 100.f;

	min = r < g ? r : g;
	min = min < b ? min : b;

	max = r > g ? r : g;
	max = max > b ? max : b;

	out.b = max;                                // v
	delta = max - min;
	if (max > 0.0)
	{ // NOTE: if Max is == 0, this divide would cause a crash
		out.g = (delta / max);                  // s
	}
	else
	{
		// if max is 0, then r = g = b = 0
		// s = 0, v is undefined
		out.g = 0.0;
		out.r = NAN;                            // its now undefined
		return out;
	}
	if (r >= max)                       // > is bogus, just keeps compilor happy
		out.r = (g - b) / delta;        // between yellow & magenta
	else if (g >= max)
		out.r = 2.0 + (b - r) / delta;  // between cyan & yellow
	else
		out.r = 4.0 + (r - g) / delta;  // between magenta & cyan

	out.r *= 60.0;                              // degrees

	if (out.r < 0.0)
		out.r += 360.0;

	out.r /= 360.f;
	out.g /= 100.f;
	out.b /= 100.f;

	return out;
}

//---------------------------------------------------------

glm::vec3 GLSLParticleSystemFbo::HSVtoRGB(float h, float s, float v)
{
	glm::vec3 out;

	h *= 360.f;
	s *= 100.f;
	v *= 100.f;

	double hh, p, q, t, ff;
	long i;

	if (s <= 0.0)
	{       // < is bogus, just shuts up warnings
		out.r = v;
		out.g = v;
		out.b = v;
		return out;
	}
	hh = h;
	if (hh >= 360.0)
		hh = 0.0;
	hh /= 60.0;
	i = (long) hh;
	ff = hh - i;
	p = v * (1.0 - s);
	q = v * (1.0 - (s * ff));
	t = v * (1.0 - (s * (1.0 - ff)));

	switch (i)
	{
	case 0:
		out.r = v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = v;
		break;
	case 5:
	default:
		out.r = v;
		out.g = p;
		out.b = q;
		break;
	}

	out.r /= 100.f;
	out.g /= 100.f;
	out.b /= 100.f;

	return out;
}

//------------------------------------------------------------------------

GLint* GLSLParticleSystemFbo::getCellSizePtr()
{
	return &basePTex.cellSize[0];
}

//------------------------------------------------------------------------

GLint GLSLParticleSystemFbo::getMaxGeoAmpPoints()
{
	return maxGeoAmpPoints;
}

//------------------------------------------------------------------------

GLuint GLSLParticleSystemFbo::getActPosTex()
{
	return basePTex.ppBuf->src->getColorImg(0);;
}

//------------------------------------------------------------------------

GLuint GLSLParticleSystemFbo::getActVelTex()
{
	return basePTex.ppBuf->src->getColorImg(1);;
}

//------------------------------------------------------------------------

VAO* GLSLParticleSystemFbo::getDrawTrig()
{
	return basePTex.trigVaoPoints;
}

//------------------------------------------------------------------------

GLuint GLSLParticleSystemFbo::getPTexSrcId(unsigned int _attNr)
{
	return basePTex.ppBuf->getSrcTexId(_attNr);
}

//------------------------------------------------------------------------

GLuint GLSLParticleSystemFbo::getEmitTex()
{
	if (actEmitData)
		return actEmitData->texNr;
	return 0;
}

//------------------------------------------------------------------------

unsigned int GLSLParticleSystemFbo::getNrAttachments()
{
	return TBT_COUNT;
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initUpdateShdr()
{
	if (useAtomicCounters)
		shdr_Header = "#version 420 core\n";
	else
		shdr_Header = "#version 410 core\n";
	shdr_Header += "#pragma optimize(on)\n";

	stdVert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; out vec2 tex_coord;

					void main() { tex_coord = texCoord; gl_Position = position; });

	stdVert = "// GLSLParticleSystemFbo vertex shader\n" + shdr_Header
			+ stdVert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D pos_tex; uniform sampler2D vel_tex; uniform sampler2D col_tex; uniform sampler2D aux0_tex;

					uniform float dt; uniform float doAging; uniform float doAgeFading; uniform float friction; uniform float lifeTime; uniform float ageFadeCurv;

					in vec2 tex_coord;

					layout (location = 0) out vec4 pos; layout (location = 1) out vec4 vel; layout (location = 2) out vec4 color; layout (location = 3) out vec4 aux0;

					vec4 getPos; vec4 getCol; vec4 getVel; vec4 getAux0; ivec2 iTexCoord;

					void main() { iTexCoord = ivec2(gl_FragCoord.xy); getAux0 = texelFetch(aux0_tex, iTexCoord, 0);

					// border check, limit is higher than 1.02 because of double buffering and discard (0 won´t be written twice)
					//	int drawCond = int(getPos.x >= 1.02) + int(getPos.x <= -1.02) + int(getPos.y >= 1.02) + int(getPos.y <= -1.02);

					if (getAux0.x > 0.0001)// ist eigentlich unnötig, macht der shader von allein...
					{ getPos = texelFetch(pos_tex, iTexCoord, 0);

					// velocity has to be written with a > 0.f  (a == 0.f is ignored by the shader)
					getVel = texelFetch(vel_tex, iTexCoord, 0) * friction; vel = getVel;

					// movement
					pos = getPos + vec4(getVel.xyz, 0.0) * dt;

					getCol = texelFetch(col_tex, iTexCoord, 0); aux0 = vec4(getAux0.x - (dt * doAging),// lifetime
					getAux0.y,// size
					getAux0.z,// angle
					getAux0.w);// texNr

					color = getCol;
					// lifetime agefading
					color.a *= min(getAux0.x / lifeTime, ageFadeCurv) / ageFadeCurv * doAgeFading + (1.0 - doAgeFading); } else { discard; } });
	frag = "// GLSLParticleSystemFBO Update Shader\n" + shdr_Header + frag;

	updateShader = shCol->addCheckShaderText("partFboUpdate", stdVert.c_str(),
			frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initVelTexShdr()
{
	stdVert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; out vec2 tex_coord; void main() { tex_coord = texCoord; gl_Position = position; });

	stdVert = "// GLSLParticleSystemFbo vertex shader\n" + shdr_Header
			+ stdVert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D pos_tex; uniform sampler2D vel_tex; uniform sampler2D col_tex; uniform sampler2D aux0_tex; uniform sampler2D extVel_tex; // emitvel text is supposed to have only valid x and y values
					uniform sampler2D extForce_tex;// external force, like wind...

					uniform float dt; uniform float doAging; uniform float doAgeFading; uniform float friction; uniform float lifeTime; uniform float extVelAmt; uniform float extVelScale; uniform float ageFadeCurv;

					uniform vec2 extFOffs; in vec2 tex_coord;

					layout (location = 0) out vec4 pos; layout (location = 1) out vec4 vel; layout (location = 2) out vec4 color; layout (location = 3) out vec4 aux0;

					vec4 getCol; vec4 getPos; vec4 getVel; vec4 getAux0;

					void main() { ivec2 iTexCoord = ivec2(gl_FragCoord.xy); getAux0 = texelFetch(aux0_tex, iTexCoord, 0);

					// border check, limit is higher than 1.02 because of double buffering
					// and discard (0 won´t be written twice)
					// int drawCond = int(pos.x >= 1.02) + int(pos.x <= -1.02) + int(pos.y >= 1.02) + int(pos.y <= -1.02);

					// switching doesn´t work, since we are double buffering...
					//int drawCond = int(pos.a < 0.001);

					if (getAux0.x > 0.0001)// ist eigentlich unnötig, macht der shader von allein...
					{ getPos = texelFetch(pos_tex, iTexCoord, 0); getVel = texelFetch(vel_tex, iTexCoord, 0);

					// movement
					// convert Particel position to velocity_texture_coordinate
					vec2 velTexCoord = vec2(getPos.x * 0.5 + 0.5, 1.0 - (getPos.y * 0.5 + 0.5)); vec4 extVel = vec4(texture(extVel_tex, velTexCoord).xy * extVelScale, 0.0, 0.0);

//							vec2 extForceCoord = vec2((pos.x + extFOffs.x) * 0.1, (pos.y + extFOffs.y) * 0.1);
//							vec4 extForce = texture(extForce_tex, extForceCoord);

					// mix particle velocity and externel velocity
					getVel = mix(getVel, extVel, extVelAmt);

					// write new velocity
					vel = getVel * friction;

					// apply velocity movement
					pos = getPos + vec4(getVel.xyz, 0.0) * dt;

					getCol = texelFetch(col_tex, iTexCoord, 0); aux0 = vec4(getAux0.x - (dt * doAging),// lifetime
					getAux0.y,// size
					getAux0.z,// angle
					getAux0.w);// texNr

					// angle offset
					// zum winkel rotieren, hol die benachbarte velocity
//                            vec4 extVelRight = texture(extVel_tex, vec2(velTexCoord.x + 0.05, velTexCoord.y));
//                            float angleOffset = dot(extVel.xy, extVelRight.xy);
//                            aux0.z += angleOffset * 0.05;

					// lifetime agefading
					color = getCol; color.a *= min(getAux0.x / lifeTime, ageFadeCurv) / ageFadeCurv * doAgeFading + (1.0 - doAgeFading); } else { discard; } });
	frag =
			"// GLSLParticleSystemFBO Update Shader with external Velocity Texture\n"
					+ shdr_Header + frag;

	updateVelTexShader = shCol->addCheckShaderText("partFboUpdateVelTex",
			stdVert.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initEmitShdr()
{
	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position; layout(location = 3) in vec4 color; layout(location = 5) in vec4 velocity; layout(location = 6) in vec4 aux0; layout(location = 7) in vec4 aux1; uniform mat4 m_pvm;

					out VS_FS_VERTEX { vec4 pos; vec4 vel; vec4 col; vec4 aux0; } vertex_out;

					void main() { vertex_out.pos = position; vertex_out.vel = velocity; vertex_out.col = color; vertex_out.aux0 = aux0; gl_Position = aux1; });
	vert = "// GLSLParticleSystemFBO Emit Shader vertex shader\n" + shdr_Header
			+ vert;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 pos; layout (location = 1) out vec4 vel; layout (location = 2) out vec4 color; layout (location = 3) out vec4 aux0;

					in VS_FS_VERTEX { vec4 pos; vec4 vel; vec4 col; vec4 aux0; } vertex_in;

					void main(){ pos = vertex_in.pos; vel = vertex_in.vel; color = vertex_in.col; aux0 = vertex_in.aux0; });

	frag = "// GLSLParticleSystemFBO Emit Shader fragment shader\n"
			+ shdr_Header + frag;

	emitShader = shCol->addCheckShaderText("partFboEmit", vert.c_str(),
			frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initEmitTexRecShdr()
{
	if (useAtomicCounters)
		shdr_Header = "#version 420 core\n";
	else
		shdr_Header = "#version 410 core\n";

	shdr_Header += "#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout(location = 0) in vec4 position;
		void main(){
			gl_Position = position; // textur Koordinaten [0 | 1]
		});

	vert = "// GLSLParticleSystemFBO Emit Tex Rec Shader vertex shader\n" + shdr_Header + vert;

	//----------------------------------------------

	std::string geom;

	if (useAtomicCounters)
		geom += "layout (binding = 0, offset = 0) uniform atomic_uint primCnt;\n";

	geom += STRINGIFY(
		layout (points) in;\n

		out vec4 rec_position;\n
		out vec4 rec_velocity;\n

		uniform sampler2D emitTex;\n
		uniform sampler2D velTex;\n
		uniform float time;\n
		uniform int useVelTex;\n

		uniform emitDataRec {\n
			ivec2 cellSize;\n
			int texWidth;\n
			int texHeight;\n
			float emitTexThres;\n
		};\n

		vec2 tex_coord;\n
		vec4 texCol;\n
		vec2 baseTexCoord;\n
		vec2 readOffset;\n

		void main()\n {\n
			// base coordinate (zellen position 0) von EmitData Aux1 [0|1]
			baseTexCoord = gl_in[0].gl_Position.xy;\n

			// in dem definierten auschnitt gehe durch die Emit Textur durch
			for (int y=0;y<cellSize.y;y++)\n
			{\n
				for (int x=0;x<cellSize.x;x++)\n
				{\n
					// position zum lesen aus der Emit Textur, offset in textur-Koordinaten [0-1]
					readOffset = vec2(
							float(x) / float(texWidth),\n
							float(y) / float(texHeight));\n

					tex_coord = baseTexCoord + readOffset;\n
					texCol = texture(emitTex, tex_coord);\n
					if (((abs(texCol.r) + abs(texCol.g) + abs(texCol.b)) * 0.3 * texCol.a) > emitTexThres)\n
					{\n
						rec_position = vec4(tex_coord * 2.0 - 1.0, 0.0, 1.0);\n
						if(useVelTex > 0){
							rec_velocity = texture(velTex, tex_coord);
						}
	\n);

	if(useAtomicCounters)
		geom += "atomicCounterIncrement(primCnt);\n";

geom += STRINGIFY(
						EmitVertex();\n
						EndPrimitive();\n
					}\n
				}\n
			}\n
		}
	);

	geom = "// GLSLParticleSystemFBO Emit Tex Rec Shader geom shader\n"
			+ shdr_Header + "layout (points, max_vertices = "
			+ std::to_string(maxGeoAmpPoints) + ") out;\n" + geom;

	//std::cout << geom << std::endl;

	//----------------------------------------------

	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		void main(){
			color = vec4(1.0);
		});


	frag = "// GLSLParticleSystemFBO Emit Tex Rec Shader fragment shader\n"
			+ shdr_Header + frag;

	emitShaderTexRec = shCol->addCheckShaderTextNoLink("partFboEmitTexRec",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initEmitTexShdr()
{
	std::string vert = STRINGIFY(
		layout(location = 0) in vec4 position;
		layout(location = 5) in vec4 velocity;

		uniform samplerBuffer cellOffs;
		uniform ivec2 cellPtr;
		uniform mat4 m_pvm;
		uniform float time;
		uniform int useVelTex;

		uniform emitData {
			vec2 cellStep;  // im bezug auf [0 | 1]
			ivec2 cellSize;
			ivec2 nrCells;
			vec2 texSize;
			vec2 texOffs;
			int texNr;
			int maxNrTex;
			float lifeTime;
			float posRand;
			float dirRand;
			float texRand;
			float colRand;
			float speed;
			float size;
			float sizeRand;
			float angle;
			float angleRand;
			vec3 emitVel;
			vec4 emitCol;
		};

		out VS_FS_VERTEX{ vec4 pos; vec4 vel; vec4 col; vec4 aux0; } vertex_out;

		ivec2 writePos;
		vec2 fWritePos;
		ivec2 actCellPtr;
		int writeOffs;
		int cellLoop;
		float noiseX = 0.0;
		float randAngle = 0.0;
		float sizeRandProc;
		float pi = 3.1415926535897932384626433832795;
		vec2 noiseOffs = vec2(0.0);
		vec4 resEmitVel;

		// @appas  noise function from stackoverflow.xom
		float snoise(vec2 co){
			return (fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453) - 0.5) * 2.0;
		}

		void main() {
			// bestimmte die Schreibposition des Partikels
			// bei jedem Vertice wird eine Zelle weitergesprungen
			// bestimme dementsprechen die aktuelle Zelle
			actCellPtr = ivec2((cellPtr.x + (gl_VertexID % nrCells.x)) % nrCells.x,
					(cellPtr.y + ((gl_VertexID / nrCells.x) % nrCells.y)) % nrCells.y);

			// bestimme die Position der Zelle in Pixel
			writePos = actCellPtr * cellSize;

			// lese den writeOffset fuer diese Zelle
			writeOffs = int(texelFetch(cellOffs, actCellPtr.x + actCellPtr.y * nrCells.x).r);

			// bestimme wie viel mal bis jetzt alle zellen durchlaufen wurden
			cellLoop = gl_VertexID / (nrCells.x * nrCells.y);

			// addiere den writeOffset und den cellLoop in tex_coord dazu
			writePos = ivec2(writePos.x + ((writeOffs + cellLoop) % cellSize.x),
							writePos.y + (((writeOffs + cellLoop) / cellSize.x) % cellSize.y));

			fWritePos = vec2(writePos.xy) / texSize;

			if (posRand > 0) {
				noiseX = snoise(fWritePos + time);
				noiseOffs = vec2(noiseX, snoise(fWritePos + noiseX));
			}

			// wieso nicht von der velocity textur???
			resEmitVel = vec4(emitVel, 1.0);

			if (dirRand > 0) {
				randAngle = snoise(fWritePos * time + 0.1) * pi;
				resEmitVel = vec4(resEmitVel.xyz + vec3(cos(randAngle), sin(randAngle), 0.0) * dirRand, 1.0);
			}

			resEmitVel *= speed;

			vertex_out.pos = position + vec4(noiseOffs * posRand, 0.0, 0.0);
			vertex_out.vel = useVelTex > 0 ? velocity * speed : resEmitVel;
			vertex_out.col = emitCol + colRand * vec4(snoise(fWritePos * time + 0.834),
													 snoise(fWritePos * time + 0.524),
													 snoise(fWritePos * time + 0.1344),
													 snoise(fWritePos * time + 0.444));

			sizeRandProc = mix(1.0 - sizeRand, 1.0 + sizeRand, snoise(fWritePos * time + 0.124) * 0.5 + 0.5);
			sizeRandProc *= sizeRandProc;

			// texNr geht hnicht!!!!!
			vertex_out.aux0 = vec4(lifeTime,
									size * sizeRandProc,
									angle + angleRand * snoise(fWritePos * time + 0.344), // angle
									texRand * float( int(snoise(fWritePos * time + 0.27) * float(maxNrTex) ) )
//									float(texNr) + texRand * float(int(snoise(fWritePos * time + 0.27) * float(maxNrTex)))
			);

			gl_Position = vec4(( fWritePos + texOffs ) * 2.0 - 1.0, 0.0, 1.0);// textur Koordinaten [0 | 1]
	});

	vert = "// GLSLParticleSystemFBO Emit Shader vertex shader\n" + shdr_Header + vert;

	//------------------------------------------------------------

	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 pos;
		layout (location = 1) out vec4 vel;
		layout (location = 2) out vec4 color;
		layout (location = 3) out vec4 aux0;

		in VS_FS_VERTEX {
			vec4 pos;
			vec4 vel;
			vec4 col;
			vec4 aux0;
		} vertex_in;

		void main(){
			pos = vertex_in.pos;
			vel = vertex_in.vel;
			color = vertex_in.col;
			aux0 = vertex_in.aux0;
		});

	frag = "// GLSLParticleSystemFBO Emit Shader fragment shader\n" + shdr_Header + frag;

	emitShaderTex = shCol->addCheckShaderText("partFboEmitTex", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initEmitDepthTexShdr()
{
	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position; layout(location = 3) in vec4 color; layout(location = 5) in vec4 velocity; layout(location = 6) in vec4 aux0; layout(location = 7) in vec4 aux1;

					uniform mat4 m_pvm;

					out VS_GS_VERTEX { vec4 pos; vec4 vel; vec4 col; vec4 aux0; } vertex_out;

					void main() { vertex_out.pos = position; vertex_out.vel = velocity; vertex_out.col = color; vertex_out.aux0 = aux0; gl_Position = aux1; // speicherposition
					});

	vert = "// GLSLParticleSystemFbo emitShaderDepthTex vertex shader\n"
			+ shdr_Header + vert;

	std::string geom =
			STRINGIFY(layout (points) in;
			//layout (points, max_vertices = 80) out; hardware limit 51 geforce 970 TO DO CHECKEN!!!
					layout (points, max_vertices = 50) out;

					in VS_GS_VERTEX { vec4 pos; vec4 vel; vec4 col; vec4 aux0; // speicherposition
					} vertex_in[];

					out GS_FS_VERTEX { vec4 oPos; vec4 oVel; vec4 oCol; vec4 oAux0; } vertex_out;

					uniform sampler2D emitTex; uniform sampler2D depthTex;

					uniform int nrPartPerCellSqrt; uniform vec2 invGridSizeF; uniform int maxEmit; uniform int texWidth; uniform int texHeight; uniform int gridSizeSqrt;

					void main() { int emitCount = 0;

					int nrHPix = texWidth / gridSizeSqrt; int nrVPix = texHeight / gridSizeSqrt;

					// in dem definierten auschnitt gehe durch die Emit Textur durch
					for (int y=0;y<nrVPix;y++) { for (int x=0;x<nrHPix;x++) { if (emitCount >= maxEmit) { break; } else { vec2 writeOffset = vec2(float(x) / float(nrHPix -1), float(y) / float(nrVPix -1));

					vec2 readOffset = vec2(float(x) / float(texWidth), float(y) / float(texHeight));

					vec2 tex_coord = vec2(gl_in[0].gl_Position.x * 0.5 + 0.5 + readOffset.x, 1.0 - (gl_in[0].gl_Position.y * 0.5 + 0.5 + readOffset.y));

					vec4 texCol = texture(emitTex, tex_coord);

					if (texCol.r > 0.0) { vec4 depthVal = texture(depthTex, tex_coord);

					vertex_out.oPos = gl_in[0].gl_Position + vec4(writeOffset.xy * invGridSizeF * 2.0, depthVal.r * -2.0 + 1.0, 0.0) + vertex_in[0].pos; vertex_out.oVel = vertex_in[0].vel; vertex_out.oCol = vertex_in[0].col; vertex_out.oAux0 = vertex_in[0].aux0;

					gl_Position = gl_in[0].gl_Position + vec4(writeOffset.xy * invGridSizeF * 2.0, 0.0, 0.0);

					EmitVertex(); EndPrimitive();

					emitCount++; } } } } });
	geom = "// GLSLParticleSystemFBO emitShaderDepthTex, pos.w = lifetime\n"
			+ shdr_Header + geom;

	std::string frag =
			STRINGIFY(  // GLSLParticleSystemFBO Emit Shader
					layout (location = 0) out vec4 pos; layout (location = 1) out vec4 vel; layout (location = 2) out vec4 color; layout (location = 3) out vec4 aux0;

					in GS_FS_VERTEX { vec4 oPos; vec4 oVel; vec4 oCol; vec4 oAux0; } vertex_in;

					void main() { pos = vertex_in.oPos; vel = vertex_in.oVel; color = vertex_in.oCol; aux0 = vertex_in.oAux0; });

	frag = "// GLSLParticleSystemFBO emitShaderDepthTex, pos.w = lifetime\n"
			+ shdr_Header + frag;

	emitShaderDepthTex = shCol->addCheckShaderText("partFboEmitDepthTex",
			vert.c_str(), geom.c_str(), frag.c_str());

//	emitShaderDepthTex = shCol->addCheckShader("partFboEmitDepthTex",
//			"shaders/partFboEmitDepthTex.vert",
//			"shaders/partFboEmitDepthTex.geom",
//			"shaders/partFboEmitDepthTex.frag");
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initDrawPointShdr()
{
	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position; uniform mat4 m_pvm; void main() { gl_Position = position; });

	vert = "// GLSLParticleSystemFbo Draw Point Shader vertex shader\n"
			+ shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					uniform sampler2D pos_tex; uniform sampler2D vel_tex; uniform sampler2D col_tex; uniform sampler2D aux0_tex;

					uniform ivec2 cellSize; uniform mat4 m_pvm; uniform float velBright;

					out vec4 fsColor;

					vec4 pPos; vec4 pVel; vec4 pCol; vec4 pAux0; ivec2 texCoord;

					void main() { ivec2 baseTexCoord = ivec2(gl_in[0].gl_Position.xy);

					// read the textures
					for (int y=0;y<cellSize.y;y++) { for (int x=0;x<cellSize.x;x++) { texCoord = baseTexCoord + ivec2(x, y); pAux0 = texelFetch(aux0_tex, texCoord, 0);

					// switching doesn´t work, since we are double buffering...
					// check if the particle is living 
					if ( pAux0.x > 0.001 ) { pPos = texelFetch(pos_tex, texCoord, 0); pVel = texelFetch(vel_tex, texCoord, 0); pCol = texelFetch(col_tex, texCoord, 0); fsColor = vec4(pCol.rgb + min(abs(pVel.x + pVel.y) * 0.5, 1.0) * velBright, pCol.a); gl_Position = m_pvm * pPos;

					EmitVertex(); EndPrimitive(); } } } });

	geom = "// GLSLParticleSystemFBO Draw Point Shader\n" + shdr_Header
			+ "layout (points) in;\n layout (points, max_vertices = "
			+ std::to_string(maxGeoAmpPoints) + ") out;" + geom;

	std::string frag =
			STRINGIFY(                      // GLSLParticleSystemFBO Emit Shader
					layout (location = 0) out vec4 color; in vec4 fsColor; void main() { color = fsColor; });

	frag = "// GLSLParticleSystemFBO Draw Point Shader\n" + shdr_Header + frag;

	drawShader = shCol->addCheckShaderText("partFboDraw", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------
// per geo shader ist VIEL schneller als per instance drawing...

void GLSLParticleSystemFbo::initDrawQuadShdr()
{
	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position; void main() { gl_Position = position; });
	vert = "// GLSLParticleSystemFbo  Draw Quad Shader vertex shader\n"
			+ shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					layout (points) in;

					uniform sampler2D pos_tex; uniform sampler2D vel_tex; uniform sampler2D col_tex; uniform sampler2D aux0_tex;

					uniform int nrSegColTex; uniform float scaleX; uniform float scaleY; uniform ivec2 cellSize;

					uniform mat4 m_pvm; mat4 trans_matrix; vec4 column0; vec4 column1; vec4 column2; vec4 column3;

					vec4 pPos; vec4 pVel; vec4 pCol; vec4 pAux0;

					vec4 center; vec4 outPos; ivec2 texCoord;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					void main() {
					// in pixel
					float size = 0.05; float sqrtNrSegColTex = sqrt( float(nrSegColTex) ); float nrSegColSideLen = 1.0 / sqrt( float(nrSegColTex) );

					// read the textures
					for (int y=0;y<cellSize.y;y++) { for (int x=0;x<cellSize.x;x++) { texCoord = ivec2(gl_in[0].gl_Position.xy) + ivec2(x, y); pAux0 = texelFetch(aux0_tex, texCoord, 0);

					if (pAux0.x > 0.0004) { pPos = texelFetch(pos_tex, texCoord, 0); pCol = texelFetch(col_tex, texCoord, 0); rec_color = pCol; center = vec4(pPos.xyz, 1.0);

					// Erstelle eine Matrize zum drehen und verschieben
					column0 = vec4(cos(pAux0.z), sin(pAux0.z), 0.0, 0.0); column1 = vec4(-sin(pAux0.z), cos(pAux0.z), 0.0, 0.0); column2 = vec4(0.0, 0.0, 1.0, 0.0); column3 = vec4(0.0, 0.0, 0.0, 1.0); trans_matrix = mat4(column0, column1, column2, column3); vec2 texCoordOff = vec2(float(int(pAux0.w) % int(sqrtNrSegColTex)) / sqrtNrSegColTex, float(int(pAux0.w / sqrtNrSegColTex)) / sqrtNrSegColTex );

					// erstelle die vier ecken eins quad, gezeichnet durch ein tri-strip
					for (int i=0;i<4;i++) {
					// Ermittle die Position der Koordinaten aux_par.r = size, gl_Position ist der Mittelpunkt
					outPos = center + vec4(((i > 1) ? pAux0.y : -pAux0.y), (i == 0 || i == 2) ? pAux0.y : -pAux0.y, 0.0, 1.0); rec_texCoord = vec4((i > 1) ? texCoordOff.x + nrSegColSideLen : texCoordOff.x, (i == 0 || i == 2) ? texCoordOff.y + nrSegColSideLen : texCoordOff.y, 0.0, 1.0); rec_position = m_pvm * trans_matrix * outPos; rec_normal = vec3(0.0, 0.0, 1.0); gl_Position = rec_position; EmitVertex(); } EndPrimitive(); } } } });

	geom = "// GLSLParticleSystemFBO Draw Quad Shader\n" + shdr_Header
			+ "layout (triangle_strip, max_vertices = "
			+ std::to_string(maxGeoAmpTriStrip) + ") out;\n" + geom;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; void main() { color = vec4(1.0); // + color;
					});

	frag = "// GLSLParticleSystemFBO Draw Quad Shader\n" + shdr_Header + frag;

	drawShaderQuad = shCol->addCheckShaderTextNoLink("partFboDrawQuad",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------
// per geo shader ist VIEL schneller als per instance drawing...

void GLSLParticleSystemFbo::initDrawQuadShdrDirect()
{
	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position; void main() { gl_Position = position; });

	vert = "// GLSLParticleSystemFbo  Draw Quad Shader vertex shader\n"
			+ shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					layout (points) in;

					uniform sampler2D pos_tex; uniform sampler2D vel_tex; uniform sampler2D col_tex; uniform sampler2D aux0_tex;

					uniform int nrSegColTex; uniform float scaleX; uniform float scaleY; uniform ivec2 cellSize;

					uniform mat4 m_pvm; mat4 trans_matrix; vec4 column0; vec4 column1; vec4 column2; vec4 column3;

					out GS_FS_VERTEX { vec4 fsColor; vec4 pos; vec2 tex_coord; } vertex_out;

					vec4 pPos; vec4 pVel; vec4 pCol; vec4 pAux0;

					vec4 center; vec4 outPos; ivec2 texCoord;

					void main() {
					// in pixel
					float size = 0.05; float sqrtNrSegColTex = sqrt( float(nrSegColTex) ); float nrSegColSideLen = 1.0 / sqrt( float(nrSegColTex) );

					// read the textures
					for (int y=0;y<cellSize.y;y++) { for (int x=0;x<cellSize.x;x++) { texCoord = ivec2(gl_in[0].gl_Position.xy) + ivec2(x, y); pAux0 = texelFetch(aux0_tex, texCoord, 0);

					if (pAux0.x > 0.0004) { pPos = texelFetch(pos_tex, texCoord, 0); pCol = texelFetch(col_tex, texCoord, 0);

					vertex_out.fsColor = pCol; center = vec4(pPos.xyz, 1.0); vertex_out.pos = center;

					// Erstelle eine Matrize zum drehen und verschieben
					column0 = vec4(cos(pAux0.z), sin(pAux0.z), 0.0, 0.0); column1 = vec4(-sin(pAux0.z), cos(pAux0.z), 0.0, 0.0); column2 = vec4(0.0, 0.0, 1.0, 0.0); column3 = vec4(0.0, 0.0, 0.0, 1.0); trans_matrix = mat4(column0, column1, column2, column3);

					vec2 texCoordOff = vec2(float(int(pAux0.w) % int(sqrtNrSegColTex)) / sqrtNrSegColTex, float(int(pAux0.w / sqrtNrSegColTex)) / sqrtNrSegColTex );

					// erstelle die vier ecken eins quad, gezeichnet durch ein tri-strip
					for (int i=0;i<4;i++) {
					// Ermittle die Position der Koordinaten aux_par.r = size, gl_Position ist der Mittelpunkt
					outPos = center + vec4(((i > 1) ? pAux0.y : -pAux0.y), (i == 0 || i == 2) ? pAux0.y : -pAux0.y, 0.0, 1.0); vertex_out.tex_coord = vec2((i > 1) ? texCoordOff.x + nrSegColSideLen : texCoordOff.x, (i == 0 || i == 2) ? texCoordOff.y + nrSegColSideLen : texCoordOff.y);

					gl_Position = m_pvm * trans_matrix * outPos;

					EmitVertex(); } EndPrimitive(); } } } });

	geom = "// GLSLParticleSystemFBO Draw Quad Shader\n" + shdr_Header
			+ "layout (triangle_strip, max_vertices = "
			+ std::to_string(maxGeoAmpTriStrip) + ") out;\n" + geom;

	std::string frag =
			STRINGIFY(layout (location = 0) out vec4 color;

			in GS_FS_VERTEX { vec4 fsColor; // aux_par0: (r: size, g: farbIndex (0-1), b: angle, a: textureUnit)
					vec4 pos; vec2 tex_coord; } vertex_in;

					uniform sampler2D colTex; uniform sampler2D litsphereTexture; uniform sampler2D normalTex;

					uniform vec3 lightPos; uniform float shininess; uniform vec4 specular; uniform vec4 diffuse; uniform vec4 ambient; uniform vec4 sceneColor;

					void main() { vec4 tex = texture(colTex, vertex_in.tex_coord); vec4 normal = texture(normalTex, vertex_in.tex_coord);
					//    color = vec4(tex.rgb, vertex_in.fsColor.a * tex.a);
					//    color = vertex_in.fsColor;

					vec3 eyeNormal = normal.xyz; vec3 L = normalize(lightPos - vertex_in.pos.xyz); vec3 E = normalize(-vertex_in.pos.xyz);// we are in Eye Coordinates, so EyePos is (0,0,0)
					vec3 R = normalize(-reflect(L, eyeNormal));

					//calculate Ambient Term:
					vec4 Iamb = ambient;

					//calculate Diffuse Term:
					vec4 Idiff = diffuse * max(dot(eyeNormal, L), 0.0); Idiff = clamp(Idiff, 0.0, 1.0);

					// calculate Specular Term:
					vec4 Ispec = specular * pow(max(dot(R, E), 0.0),0.3 * shininess); Ispec = clamp(Ispec, 0.0, 1.0);

					vec4 shading = texture(litsphereTexture, vec2(normal.xyz * vec3(0.495) + vec3(0.5)));
					//    vec4 shading = texture2DRect(litsphereTexture, texCoord.st * 256);
					//    vec4 shading = texture2DRect(litsphereTexture, texCoord.xy);
					//    vec4 shading = texture2DRect(litsphereTexture, normal.xy * 256);
					color = ((sceneColor + Iamb + Idiff + Ispec) * shading + Ispec) * tex * 0.7;// + color;
					color *= vertex_in.fsColor.a;

					});

	frag = "// GLSLParticleSystemFBO Draw Quad Shader\n" + shdr_Header + frag;

	drawShaderQuadDirect = shCol->addCheckShaderText("partFboDrawQuadDirect",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystemFbo::initDrawMultShdr()
{
	std::string vert =
			STRINGIFY(
					layout(location = 0) in vec4 position; uniform mat4 m_pvm; void main() { gl_Position = position; });

	vert = "// GLSLParticleSystemFbo  Draw Mult Shader vertex shader\n"
			+ shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					layout(points, invocations=3) in; layout (points, max_vertices = 110) out; // hardware limit geforce 113
//			layout (points, max_vertices = 144) out;

					uniform sampler2D pos_tex; uniform sampler2D vel_tex; uniform sampler2D col_tex; uniform sampler2D aux0_tex;

					uniform int pixPerGrid; uniform float invNrPartSqrt;

					uniform mat4 model_matrix_g[3];

					out vec4 fsColor;

					void main() { vec4 pPos; vec4 pVel; vec4 pCol; vec4 pAux0; vec2 texCoord; vec2 baseTexCoord = gl_in[0].gl_Position.xy;

					gl_ViewportIndex = gl_InvocationID;

					// read the textures
					for (int y=0;y<pixPerGrid;y++) { for (int x=0;x<pixPerGrid;x++) { texCoord = baseTexCoord + vec2(float(x) * invNrPartSqrt, float(y) * invNrPartSqrt); pPos = texture(pos_tex, texCoord);

					// switching doesn´t work, since we are double buffering...
					if (pPos.w > 0.008) {
					//pVel = texture(vel_tex, texCoord);
					pCol = texture(col_tex, texCoord); pAux0 = texture(aux0_tex, texCoord);

					fsColor = pCol; gl_Position = model_matrix_g[gl_InvocationID] * vec4(pPos.xyz, 1.0);

					EmitVertex(); EndPrimitive(); } } } });

	geom = "// GLSLParticleSystemFBO Draw Mult Shader\n" + shdr_Header + geom;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; in vec4 fsColor; void main() { color = fsColor; });

	frag = "// GLSLParticleSystemFBO Draw Mult Shader, pos.w = lifetime\n"
			+ shdr_Header + frag;

	drawMultShader = shCol->addCheckShaderText("partFboDrawMult", vert.c_str(),
			geom.c_str(), frag.c_str());

//	drawMultShader = shCol->addCheckShader("partFboDrawMult",
//			"shaders/partFboDrawMult.vert", "shaders/partFboDrawMult.geom",
//			"shaders/partFboDrawMult.frag");

}

//------------------------------------------------------------------------

GLSLParticleSystemFbo::~GLSLParticleSystemFbo()
{
	delete basePTex.ppBuf;
	delete quad;
}
}
