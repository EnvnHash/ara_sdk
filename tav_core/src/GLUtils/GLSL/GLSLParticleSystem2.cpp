//
//  GLSLParticleSystem2.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 24.09.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  das partikelsystem wird nur mit texture units gesteuert, standardmaessig werden die
//  units der reihe nach genommen, die textur nummer kommt von der setup xml datei
//  (tex0, tex1, tex2, ...)
//
//  die informationen zum emittieren kommen von den TextureBuffers
//  in emitTbo0 kommt die position, wenn die w Koordinate auf 1 gesetzt ist, wird das Partikel emittiert
//  in emitTbo1 kommt der richtungsvektor (länge = geschwindigkeit), w: angleOffset (von velTex)
//  emitTbo2: x: size, y: farbIndex (0-1), z: angle (z-rotation des quads um die eigene achse), w: textureUnit
//  emitTbo3: a: alpha
//
//  folgende information werden rotativ gelesen/geschrieben:
//  position -> rec_position :
//  velocity -> rec_velocity :
//  aux0 -> rec_aux0: emitTbo2 (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
//  aux1 -> rec_aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

#include "pch.h"
#include "GLSLParticleSystem2.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
GLSLParticleSystem2::GLSLParticleSystem2(ShaderCollector* _shCol,
		int _maxNrParticles, int _screenWidth, int _screenHeight) :
		maxNrParticles(_maxNrParticles), screenWidth(_screenWidth), screenHeight(
				_screenHeight), shCol(_shCol), reposFact(0.f), lifeTime(1.f), doAging(
				1.f), friction(0.f), ageFading(0.f), colorInd(0.f), nrTfos(2), tfoPtr(
				0), maxNrEmitTrig(800), nrEmittedParticles(0), actEmitType(
				PET_DATA), checkBounds(true)
{
	gravity = glm::vec3(0.f, 0.f, 0.f);

	velTexAngleStepSize = 1.f / static_cast<float>(_screenWidth);

	// vao to init the particle system
	part = new VAO("position:4f,velocity:4f,aux0:4f,aux1:4f", GL_STATIC_DRAW);
	part->initData(maxNrParticles);

	// ------------------------------------------------------------------------------
	// trigger vao for the geometry shader, a normalized grid ([0-1;0-1])
	// this vao has a special structure, since when emitting by textures
	// reducing the number of emitted particles is done by reducing the
	// number of emittriggers and limiting in each geoshader instance
	// to not have thousands of vaos all configurations go in this one
	// and are recalled by offset and size (nrEmitTrig=1 => offset=0, size=1
	// nrEmitTrig=4 => offset=1, size=4, nrEmitTrig=9 => offset=5, size=9)

	// make sure maxNrEmitTrig is a square
	maxNrEmitTrig = std::sqrt(maxNrEmitTrig) * std::sqrt(maxNrEmitTrig);

	// get necessary total size of the emitTrigVao
	int emitTrigSize = 0;
	for (int i = 0; i < std::sqrt(maxNrEmitTrig); i++)
		emitTrigSize += (i + 1) * (i + 1);

	GLfloat initEmitPos[emitTrigSize * 4];
	int posOffset = 0;
	for (int i = 0; i < std::sqrt(maxNrEmitTrig); i++)
	{
		int nrEmitBase = (i + 1) * 2;
		for (int y = 0; y < (i + 1); y++)
		{
			for (int x = 0; x < (i + 1); x++)
			{
				initEmitPos[posOffset * 4] = 1.f
						/ static_cast<float>(nrEmitBase)
						* static_cast<float>(x * 2 + 1);
				initEmitPos[posOffset * 4 + 1] = 1.f
						/ static_cast<float>(nrEmitBase)
						* static_cast<float>(y * 2 + 1);
				initEmitPos[posOffset * 4 + 2] = 0.f;
				initEmitPos[posOffset * 4 + 3] = 1.f;
				posOffset++;
			}
		}
	}

	emitTrigVao = new VAO("position:4f", GL_STATIC_DRAW);
	emitTrigVao->initData(emitTrigSize, initEmitPos);

	// ------------------------------------------------------------------------------

	nrEmitTbos = 4;
	nrEmitTbosCoords = 4;

	// prepare record-tfos
	tfoBufAttachTypes = new coordType[nrEmitTbos];
	tfoBufAttachTypes[0] = tav::POSITION;
	tfoBufAttachTypes[1] = tav::VELOCITY;
	tfoBufAttachTypes[2] = tav::AUX0;
	tfoBufAttachTypes[3] = tav::AUX1;

	// create a textureBuffer for the emit information
	GLfloat* emitInit = new GLfloat[maxNrParticles * nrEmitTbosCoords];
	for (unsigned int i = 0; i < maxNrParticles * nrEmitTbosCoords; i++)
		emitInit[i] = 0.0f;

	emitVao = new VAO("position:4f,velocity:4f,aux0:4f,aux1:4f",
			GL_DYNAMIC_DRAW);
	emitVao->initData(maxNrParticles);

	initStateTb = new TextureBuffer*[nrEmitTbos];
	for (unsigned int i = 0; i < nrEmitTbos; i++)
		initStateTb[i] = new TextureBuffer(maxNrParticles, nrEmitTbosCoords,
				emitInit);

	emitPtr = new unsigned int[2];
	for (auto i = 0; i < 2; i++)
		emitPtr[i] = 0;

	emitCntr = new unsigned int[2];
	for (auto i = 0; i < 2; i++)
		emitCntr[i] = 0;

	cam = new GLMCamera(GLMCamera::FRUSTUM, screenWidth, screenHeight, -1.0f,
			1.0f, -1.0f, 1.0f,   // left, right, bottom, top
			0.f, 0.f, 1.f,              // camPos
			0.f, 0.f, -1.f);              // lookAt

	texNrs = new int[MAX_NUM_SIM_TEXS];
	for (auto i = 0; i < MAX_NUM_SIM_TEXS; i++)
		texNrs[i] = i;

	colorPal = new glm::vec4[MAX_NUM_COL_SCENE];
	for (auto i = 0; i < MAX_NUM_SIM_TEXS; i++)
		colorPal[i] = glm::vec4(0.f, 0.f, 0.f, 1.f);

	//------ Setup XFO -----------------------------------------

	// max 4 buffer mit radeon 6490m
	// müssen mit den RecAttribNames übereinstimmen
	for (auto i = 0; i < nrEmitTbos; i++)
		recAttribNames.push_back(stdRecAttribNames[tfoBufAttachTypes[i]]);

	tfos = new TFO*[nrTfos];
	updateShaders = new Shaders*[nrTfos];
	emitShaders = new Shaders*[nrTfos];
	emitTexShaders = new Shaders*[nrTfos];

	for (auto i = 0; i < nrTfos; i++)
	{
		tfos[i] = new TFO(maxNrParticles, recAttribNames);

		updateShaders[i] = initUpdateShdr(i);

		tfos[i]->setVaryingsToRecord(&recAttribNames,
				updateShaders[i]->getProgram());
		updateShaders[i]->link();

		emitShaders[i] = initEmitShdr(i);
		tfos[i]->setVaryingsToRecord(&recAttribNames,
				emitShaders[i]->getProgram());
		emitShaders[i]->link();

		emitTexShaders[i] = initEmitTexShdr(i);

		tfos[i]->setVaryingsToRecord(&recAttribNames,
				emitTexShaders[i]->getProgram());
		emitTexShaders[i]->link();
	}

	quadShader = initQuadShdr();
	quadShader->link();

	// velocity
	recAttribNames.clear();
	recAttribNames.resize(0);
	recAttribNames.push_back(stdRecAttribNames[tav::VELOCITY]);

	velUptTfo = new TFO(maxNrParticles, recAttribNames);
	velTexShader = initVelTexShdr();

	velUptTfo->setVaryingsToRecord(&recAttribNames, velTexShader->getProgram());
	velTexShader->link();

	// velocity and return to org
	recAttribNames.clear();
	recAttribNames.resize(0);
	recAttribNames.push_back(stdRecAttribNames[tav::POSITION]);
	recAttribNames.push_back(stdRecAttribNames[tav::VELOCITY]);

	velOrgUptTfo = new TFO(maxNrParticles, recAttribNames);
	velOrgTexShader = initOrgTexShdr();
	velOrgUptTfo->setVaryingsToRecord(&recAttribNames,
			velOrgTexShader->getProgram());
	velOrgTexShader->link();

	// return To Origin
	recAttribNames.clear();
	recAttribNames.resize(0);
	recAttribNames.push_back(stdRecAttribNames[tav::POSITION]);

	posUptTfo = new TFO(maxNrParticles, recAttribNames);
	returnToOrgShader = initReturnToOrgShdr();
	posUptTfo->setVaryingsToRecord(&recAttribNames,
			returnToOrgShader->getProgram());
	returnToOrgShader->link();
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::init(TFO* _tfo)
{

//        quadSceneNodeBlendShdr = shCol->addCheckShader("partSceneQuad", "shaders/part_rec.vert",
//                                                       "shaders/part_rec.geom", "shaders/part_rec.frag");
	quadSceneNodeBlendShdr = initQuadSceneNodeBlendShdr();

	if (_tfo)
	{
		// standard first 4 parameter to record: position, normal, texCoord, color
		std::vector<std::string> names;
		for (auto i = 0; i < 4; i++)
			names.push_back(stdRecAttribNames[i]);
		_tfo->setVaryingsToRecord(&names, quadSceneNodeBlendShdr->getProgram());
	}

	quadSceneNodeBlendShdr->link();
}

//------------------------------------------------------------------------

GLSLParticleSystem2::~GLSLParticleSystem2()
{
	for (auto i = 0; i < nrTfos; i++)
	{
		delete tfos[i];
		delete updateShaders[i];
	}

	delete part;
	delete quadShader;
	delete velTexShader;
}

//-----------------------------------------------------------------------

void GLSLParticleSystem2::emit(double time, unsigned int nrPart, EmitData& data,
		bool remember, GLint posTex, int texWidth, int texHeight,
		glm::vec3* massCenter)
{
	// methode zum emittieren von Partikeln
	// Es wird direkt per Pointer in TextureBuffers geschrieben
	// in emitTbo0 kommt die position, wenn die w Koordinate auf 1 gesetzt ist, wird das Partikel emittiert
	// in emitTbo1 kommt der richtungsvektor (länge = geschwindigkeit)
	// emitTbo2: x: size, y: farbIndex (0-1), z: angle (z-rotation des quads um die eigene achse), w: textureUnit
	// emitTbo3: a: alpha
	// ES MUSS NACH JEDEM EMIT DAS UPDATE AUFGERUFEN WERDEN!!!!
	// damit zurueck zur original position gegangen weden kann,
	// wird der emitvao sequentiell vollgeschrieben und immer
	// komplett auf den initStateTb kopiert
	// so haben beide buffer korrespondierende indices
	// wenn der emitvao geschrieben wurde, wird beim naechsten update
	// zuerst alles abgespielt, was bisher aufgenommen wurde und danach
	// der emitvao abgefeuert -> zusaetzliche vertices kommen dazu

	if (posTex != 0)
	{
		actEmitType = PET_TEXTURE;
		actEmitTexData.texNr = posTex;
		actEmitTexData.width = texWidth;
		actEmitTexData.height = texHeight;
		actEmitTexData.size = data.size;
		actEmitTexData.speed = data.speed;
		actEmitTexData.angle = data.alpha;
		actEmitTexData.colInd = data.colInd;
		actEmitTexData.alpha = data.alpha;
		actEmitTexData.massCenter = massCenter;
		actEmitTexData.emitLimit = nrPart;

	}
	else
	{
		actEmitType = PET_DATA;

		// setze das "emit-flag" und initialisiere position
		GLfloat* emit = (GLfloat*) emitVao->getMapBuffer(tav::POSITION);

		for (auto i = 0; i < nrPart; i++)
		{
			int ind = ((emitPtr[0] + i) % maxNrParticles) * nrEmitTbosCoords;

			//  random, um rasterungen zu vermeiden
			glm::vec3 rEmitOrg =
					glm::vec3(
							data.emitOrg.x
									+ getRandF(-data.posRand.x, data.posRand.x),
							data.emitOrg.y
									+ getRandF(-data.posRand.t, data.posRand.y),
							data.emitOrg.z
									+ (data.emitOrg.z != 0.f ?
											getRandF(-data.posRand.z,
													data.posRand.z) :
											0.f));

			//std::cout << "emit [" << ind << "] : " << glm::to_string(rEmitOrg) << std::endl;

			emit[ind] = rEmitOrg.x;
			emit[ind + 1] = rEmitOrg.y;
			emit[ind + 2] = rEmitOrg.z;
			emit[ind + 3] = 1.0;			// das emitter "flag", wenn groesser 0 -> emit
		}
		emitVao->unMapBuffer();

		// initialisiere die richtung
		GLfloat* emitVelPtr = (GLfloat*) emitVao->getMapBuffer(tav::VELOCITY);
		for (unsigned int i = 0; i < nrPart; i++)
		{
			int ind = ((emitPtr[0] + i) % maxNrParticles) * nrEmitTbosCoords;

			glm::vec3 resV = glm::vec3(data.emitVel.x, data.emitVel.y,
					data.emitVel.z);
			// necessary, causes errors, wenn inited with only zeros...
			if (resV.x == 0.f && resV.y == 0.f && resV.z == 0)
				resV = glm::vec3(0.f, 1.f, 0.f);
			resV = glm::normalize(resV) * data.speed;

			emitVelPtr[ind] = resV.x;
			emitVelPtr[ind + 1] = resV.y;
			emitVelPtr[ind + 2] = resV.z;
			emitVelPtr[ind + 3] = 0.f;
		}
		emitVao->unMapBuffer();
	}

	// initialisiere die aux0 parameter
	GLfloat* emitAuxPtr = (GLfloat*) emitVao->getMapBuffer(tav::AUX0);
	for (unsigned int i = 0; i < nrPart; i++)
	{
		int ind = ((emitPtr[0] + i) % maxNrParticles) * nrEmitTbosCoords;
		emitAuxPtr[ind] = data.size + getRandF(0.f, data.sizeRand);
		emitAuxPtr[ind + 1] = data.colInd;
		emitAuxPtr[ind + 2] = data.angle
				+ getRandF(-data.angleRand, data.angleRand) * M_PI_2;
		emitAuxPtr[ind + 3] = std::min(
				data.texNr
						+ (int) (getRandF(0, data.texRand) * MAX_NUM_SIM_TEXS),
				MAX_NUM_SIM_TEXS);
	}
	emitVao->unMapBuffer();

	// initialisiere die aux1 parameter
	GLfloat* emitAux2Ptr = (GLfloat*) emitVao->getMapBuffer(tav::AUX1);
	for (unsigned int i = 0; i < nrPart; i++)
	{
		int ind = ((emitPtr[0] + i) % maxNrParticles) * nrEmitTbosCoords;
		emitAux2Ptr[ind + 3] = data.alpha;
	}
	emitVao->unMapBuffer();

	// update counters
	emitPtr[0] = (emitPtr[0] + nrPart) % maxNrParticles;
	nrNewEmit += nrPart;

	// option for saving the emission state
	if (remember)
	{
		// copy the calculated values to the actual tfo velocity buffer
		glBindBuffer(GL_COPY_READ_BUFFER, emitVao->getVBO(tav::POSITION));
		glBindBuffer(GL_COPY_WRITE_BUFFER, initStateTb[tav::POSITION]->getBuf());
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
				maxNrParticles
						* recCoTypeFragSize[tfoBufAttachTypes[tav::POSITION]]
						* sizeof(GLfloat));

		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}
}

//-----------------------------------------------------------------------

void GLSLParticleSystem2::emitFromData(double time, double _medDt)
{
	if (nrNewEmit > 0)
	{
		// bind particle shader, start recording geometry
		emitShaders[tfoPtr]->begin();
		emitShaders[tfoPtr]->setIdentMatrix4fv("m_pvm");
		emitShaders[tfoPtr]->setUniform1f("dt", static_cast<float>(_medDt));
		emitShaders[tfoPtr]->setUniform1f("lifeTime", lifeTime);
		emitShaders[tfoPtr]->setUniform1f("aging", doAging);
		emitShaders[tfoPtr]->setUniform3fv("gravity", &gravity[0], 1);
		emitShaders[tfoPtr]->setUniform1f("ageSizing", ageSizing);

		if (g_transformFeedbackQuery == 0)
			glGenQueries(1, &g_transformFeedbackQuery);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
				g_transformFeedbackQuery);

		// setze den counter nicht zurueck, neue partikel kommen dazu
		tfos[tfoPtr]->bind(false);
		tfos[tfoPtr]->begin(GL_POINTS);

		// emittieren die zuletzt hinzugefuegten partikel mit dem aktuellen offset
		emitVao->draw(GL_POINTS, emitPtr[1], nrNewEmit, tfos[tfoPtr], GL_POINTS);

		tfos[tfoPtr]->end();
		tfos[tfoPtr]->unbind();


		// update emit cntr
		emitPtr[1] = (emitPtr[1] + nrNewEmit) % maxNrParticles;

		// schmutziger zähler, wenn zeit um, subtrahiere die hier hinzugefügten partikel
		nrEmittedParticles += nrNewEmit;
		nrNewEmit = 0;
	}
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::emitFromTex(double time, double _medDt)
{
	// calculate the offset and size of triggerVao in relation to the requested Nr of new Particles
	int nrGeoInstances = 0;

	// make sure actEmitTexData.emitLimit is a square
	actEmitTexData.emitLimit = std::sqrt(actEmitTexData.emitLimit);
	actEmitTexData.emitLimit *= actEmitTexData.emitLimit;

	if (actEmitTexData.emitLimit < maxNrEmitTrig)
	{
		nrGeoInstances = std::sqrt(actEmitTexData.emitLimit);
		actEmitTexData.emitLimit = 1;
	}
	else
	{
		nrGeoInstances = maxNrEmitTrig;
		actEmitTexData.emitLimit = std::ceil(
				actEmitTexData.emitLimit / maxNrEmitTrig);
	}

	int emitTrigSize = 0;
	int emitTrigOffs = 0;
	for (int i = 0; i < nrGeoInstances; i++)
	{
		emitTrigSize = (i + 1) * (i + 1);
		emitTrigOffs += i * i;
	}

	//------------------------------------------------------------------------------------------

	emitTexShaders[tfoPtr]->begin();
	emitTexShaders[tfoPtr]->setIdentMatrix4fv("m_pvm");
	emitTexShaders[tfoPtr]->setUniform1i("posTex", 0);

	emitTexShaders[tfoPtr]->setUniform1i("emitLimit", actEmitTexData.emitLimit);
	emitTexShaders[tfoPtr]->setUniform1f("nrEmitTrig", static_cast<float>(maxNrEmitTrig));
	emitTexShaders[tfoPtr]->setUniform2i("winSize", actEmitTexData.width / maxNrEmitTrig,
			actEmitTexData.height / maxNrEmitTrig);
	emitTexShaders[tfoPtr]->setUniform2f("texSize", static_cast<float>(actEmitTexData.width),
			static_cast<float>(actEmitTexData.height));

	emitTexShaders[tfoPtr]->setUniform1f("speed", actEmitTexData.speed);
	emitTexShaders[tfoPtr]->setUniform1f("size", actEmitTexData.size);
	emitTexShaders[tfoPtr]->setUniform1f("colInd", actEmitTexData.colInd);
	emitTexShaders[tfoPtr]->setUniform1f("angle", actEmitTexData.angle);
	emitTexShaders[tfoPtr]->setUniform1f("texNr", std::min(actEmitTexData.texNr, MAX_NUM_SIM_TEXS));
	emitTexShaders[tfoPtr]->setUniform1f("alpha", actEmitTexData.alpha);

	emitTexShaders[tfoPtr]->setUniform1f("dt", static_cast<float>(_medDt));
	emitTexShaders[tfoPtr]->setUniform1f("lifeTime", lifeTime);
	emitTexShaders[tfoPtr]->setUniform1f("aging", doAging);
	emitTexShaders[tfoPtr]->setUniform1f("ageSizing", ageSizing);
	emitTexShaders[tfoPtr]->setUniform3fv("gravity", &gravity[0], 1);

	emitTexShaders[tfoPtr]->setUniform3fv("massCenter", &(*actEmitTexData.massCenter)[0], 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, actEmitTexData.texNr);

	tfos[tfoPtr]->bind(false);
	tfos[tfoPtr]->begin(GL_POINTS);

	// schicke die emitTrigger Vertices, interner zähler in tfo zählt nur hoch, was an vertices
	// vom emitvao reinkommt, also manuell erhöhen, je nach geoamp
	emitTrigVao->draw(GL_POINTS, emitTrigOffs, emitTrigSize, tfos[tfoPtr], GL_POINTS);

	tfos[tfoPtr]->end();
	tfos[tfoPtr]->unbind();

	tfos[tfoPtr]->decCounters(emitTrigSize);
	tfos[tfoPtr]->incCounters(emitTrigSize * actEmitTexData.emitLimit);

	// schmutziger zähler, wenn zeit um, subtrahiere die hier hinzugefügten partikel
//        if (doAging > 0.f) partDieEvents[time + lifeTime] = nrGeoInstances * actEmitTexData.emitLimit;
//        nrEmittedParticles += nrGeoInstances * actEmitTexData.emitLimit;
}

//-----------------------------------------------------------------------

//    void GLSLParticleSystem2::clearEmitBuf()
//    {
//        emittedNrPart.clear();
//    }

//------------------------------------------------------------------------

void GLSLParticleSystem2::updateVelTex(GLint velTex)
{
	glEnable(GL_RASTERIZER_DISCARD);

	velTexShader->begin();
	velTexShader->setIdentMatrix4fv("m_pvm");
	velTexShader->setUniform1i("velTex", 0);
	velTexShader->setUniform1f("step_width", velTexAngleStepSize);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velTex);

	velUptTfo->bind();
	velUptTfo->begin(GL_POINTS);

	// zeichne den aktuellen tfo, der beim nächsten update als quelle dranhängt
	tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, nullptr, GL_POINTS,
			1);

	velUptTfo->end();
	velUptTfo->unbind();

	glDisable(GL_RASTERIZER_DISCARD);

	// copy the calculated values to the actual tfo velocity buffer
	glBindBuffer(GL_COPY_READ_BUFFER, velUptTfo->getTFOBuf(VELOCITY));
	glBindBuffer(GL_COPY_WRITE_BUFFER,
			tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->getTFOBuf(VELOCITY));
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
			maxNrParticles * recCoTypeFragSize[tav::VELOCITY]
					* sizeof(GLfloat));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::updateVelTexOrg(GLint velTex, double time)
{

	glEnable(GL_RASTERIZER_DISCARD);

	velOrgTexShader->begin();
	velOrgTexShader->setIdentMatrix4fv("m_pvm");
	velOrgTexShader->setUniform1f("reposFact", reposFact);
	velOrgTexShader->setUniform1i("orgPosTbo", 0);
	velOrgTexShader->setUniform1i("velTex", 1);
	velOrgTexShader->setUniform1f("step_width", velTexAngleStepSize);

	glActiveTexture(GL_TEXTURE0);
	initStateTb[0]->bindTex();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, velTex);

	velOrgUptTfo->bind();
	velOrgUptTfo->begin(GL_POINTS);

	// zeichne den aktuellen tfo, der beim nächsten update als quelle dranhängt
	tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, nullptr, GL_POINTS,
			1);

	velOrgUptTfo->end();
	velOrgUptTfo->unbind();

	glDisable(GL_RASTERIZER_DISCARD);

	// copy the calculated values to the actual tfo velocity buffer
	glBindBuffer(GL_COPY_READ_BUFFER, velOrgUptTfo->getTFOBuf(tav::POSITION));
	glBindBuffer(GL_COPY_WRITE_BUFFER,
			tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->getTFOBuf(tav::POSITION));
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
			maxNrParticles * recCoTypeFragSize[tav::POSITION]
					* sizeof(GLfloat));

	glBindBuffer(GL_COPY_READ_BUFFER, velOrgUptTfo->getTFOBuf(tav::VELOCITY));
	glBindBuffer(GL_COPY_WRITE_BUFFER,
			tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->getTFOBuf(tav::VELOCITY));
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
			maxNrParticles * recCoTypeFragSize[tav::VELOCITY]
					* sizeof(GLfloat));

	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::updateReturnToOrg(double time)
{
	glEnable(GL_RASTERIZER_DISCARD);

	returnToOrgShader->begin();
	returnToOrgShader->setIdentMatrix4fv("m_pvm");
	returnToOrgShader->setUniform1f("reposFact", reposFact);

	glActiveTexture(GL_TEXTURE0);
	returnToOrgShader->setUniform1i("orgPosTbo", 0);
	initStateTb[0]->bindTex();

	posUptTfo->bind();
	posUptTfo->begin(GL_POINTS);

	// zeichne den aktuellen tfo, der beim nächsten update als quelle dranhängt
	tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, nullptr, GL_POINTS,
			1);

	posUptTfo->end();
	posUptTfo->unbind();

	glDisable(GL_RASTERIZER_DISCARD);

	// copy the calculated values to the actual tfo velocity buffer
	glBindBuffer(GL_COPY_READ_BUFFER, posUptTfo->getTFOBuf(tav::POSITION));
	glBindBuffer(GL_COPY_WRITE_BUFFER,
			tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->getTFOBuf(tav::POSITION));
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
			maxNrParticles * recCoTypeFragSize[tav::POSITION]
					* sizeof(GLfloat));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

}

//------------------------------------------------------------------------

void GLSLParticleSystem2::update(double time, bool draw, GLint velTex,
		drawType drT)
{

	// automatical save the actual shader program and recall it later!!!
	GLint curShdr;
	glGetIntegerv(GL_CURRENT_PROGRAM, &curShdr);

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

	// ------ first step, update all active particles -----------------------------
	// ------ bind the last vao recorded, draw it and record the output -----------

	// schmutziger zähler, wenn der erste eintrag kleiner als die aktuelle zeit
	// ist müssten die partikel schon tot sein, also zieh sie ab und loesche das
	// element
//        if ((int)partDieEvents.size() > 0 && partDieEvents.begin()->first < time)
//        {
//            nrEmittedParticles -= partDieEvents.begin()->second;
//            partDieEvents.erase(partDieEvents.begin());
//        }

	if (!draw || drT == QUADS)
		glEnable(GL_RASTERIZER_DISCARD);

//        if(g_transformFeedbackQuery == 0)
//            glGenQueries(1, &g_transformFeedbackQuery);
//        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, g_transformFeedbackQuery);

	// bind particle shader, start recording geometry
	updateShaders[tfoPtr]->begin();
	updateShaders[tfoPtr]->setIdentMatrix4fv("m_pvm");
	updateShaders[tfoPtr]->setUniform1f("dt", static_cast<float>(medDt));
	updateShaders[tfoPtr]->setUniform1f("lifeTime", lifeTime);
	updateShaders[tfoPtr]->setUniform1f("aging", doAging);
	updateShaders[tfoPtr]->setUniform1f("friction",
			1.0f - pow(friction * medDt * 10.f, 4.f));
	updateShaders[tfoPtr]->setUniform3fv("gravity", &gravity[0], 1);
	updateShaders[tfoPtr]->setUniform1f("ageFading", ageFading);
	updateShaders[tfoPtr]->setUniform1f("ageSizing", ageSizing);
	updateShaders[tfoPtr]->setUniform1f("colorInd", colorInd);
	updateShaders[tfoPtr]->setUniform1i("checkBounds",
			static_cast<int>(checkBounds));

	tfos[tfoPtr]->bind();
	tfos[tfoPtr]->begin(GL_POINTS);

	// nach erstem durchlauf, zeichne den letzen pingpong buffer und
	// nimm diesen auf den nächsten auf, zeichnet automatisch nur soviel, wie
	// das letzte mal aufgenommen wurde (interne counter)
	tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, tfos[tfoPtr],
			GL_POINTS, 1);

	tfos[tfoPtr]->end();
	tfos[tfoPtr]->unbind();

//        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
//        GLuint pointsWritten;
//        glGetQueryObjectuiv(g_transformFeedbackQuery, GL_QUERY_RESULT, &pointsWritten);
//        std::cout << "feedback query update written: " << pointsWritten << std::endl;

	updateShaders[tfoPtr]->end();

	// ------ second step, check if there are new particles to emit --------------------

	switch (actEmitType)
	{
	case PET_DATA:
		emitFromData(time, medDt);
		break;
	case PET_TEXTURE:
		emitFromTex(time, medDt);
		break;
	default:
		break;
	}

	if (!draw || drT == QUADS)
		glDisable(GL_RASTERIZER_DISCARD);

	tfoPtr = (tfoPtr + 1) % nrTfos;

	if (velTex != 0 && !returnToOrg) 	updateVelTex(velTex);
	if (velTex != 0 && returnToOrg)		updateVelTexOrg(velTex, time);
	if (velTex == 0 && returnToOrg)		updateReturnToOrg(time);

	glUseProgram(curShdr);  // activiert wieder den ursprünglichen shader
	// werte muessen aber neu gesetzt werden...
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::draw(GLenum _mode, TFO* _tfo)
{
	tfos[tfoPtr]->draw(_mode, nullptr, GL_POINTS, 1);
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::drawToBlend(drawType drT, TFO* _tfo)
{
	if (_tfo)
		_tfo->end(); // wichtig !!!

	GLint curShdr = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &curShdr);

	// der record shader vom tfo wird angehalten
	// der Shader zum Geometrie amplifizieren wird gebunden
	quadSceneNodeBlendShdr->begin();
	quadSceneNodeBlendShdr->setUniform1i("useInstancing", 0);

	cam->sendModelM(quadSceneNodeBlendShdr->getProgram(), "modelMatrix");
	cam->sendProjM(quadSceneNodeBlendShdr->getProgram(), "projectionMatrix");

	for (auto i = 0; i < MAX_NUM_COL_SCENE; i++)
		quadSceneNodeBlendShdr->setUniform4fv("auxCol" + std::to_string(i),
				&colorPal[i][0]);

	// der tfo wird wieder gestartet und nimmt auf
	if (drT == POINTS)
	{
		if (_tfo)
			_tfo->begin(GL_POINTS);
		tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, _tfo, GL_POINTS,
				1);

	}
	else if (drT == QUADS)
	{
		if (_tfo)
			_tfo->begin(GL_TRIANGLES);
		tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, _tfo,
				GL_TRIANGLES, 6);
	}

	if (_tfo)
		_tfo->end(); // tfo wird angehalten

	// shader wird wieder auf den record shader vom tfo umgeklemmt
	glUseProgram(curShdr);

	// tfo nimmt weiter auf
	if (_tfo)
		_tfo->begin(GL_TRIANGLES);
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::bindStdShader(drawType drT)
{
	switch (drT)
	{
	case POINTS:
		break;
	case QUADS:
		quadShader->begin();
		quadShader->setUniform1iv("texs", texNrs, MAX_NUM_SIM_TEXS);
		break;
	default:
		break;
	}
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::getStdShader(drawType drT)
{
	Shaders* _shdr = 0;

	switch (drT)
	{
	case QUADS:
		_shdr = quadShader;
		break;
	default:
		break;
	}

	return _shdr;
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initUpdateShdr(unsigned int ind)
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
	layout(location=0) in vec4 position;
	layout(location=5) in vec4 velocity;
	layout(location=6) in vec4 aux0;
	layout(location=7) in vec4 aux1;

	out VS_GS_VERTEX {
		vec4 position;
		vec4 velocity;
		vec4 aux0;
		vec4 aux1;
	} vertex_out;

	uniform mat4 m_pvm;

	void main() {
		vertex_out.position = position;
		vertex_out.velocity = velocity;
		vertex_out.aux0 = aux0;
		vertex_out.aux1 = aux1;

		gl_Position = vertex_out.position;
	});

	vert = "// GLSLParticleSystem2, UpdateShdr vert\n" + shdr_Header + vert;


	std::string geom = STRINGIFY(
	layout (points) in;
	layout (points, max_vertices = 1) out;

	in VS_GS_VERTEX {
		vec4 position;
		vec4 velocity;
		vec4 aux0;
		vec4 aux1;
	} vertex_in[];

	uniform int checkBounds;
	uniform float dt;
	uniform float lifeTime;
	uniform float aging;
	uniform float friction;
	uniform float ageFading;
	uniform float ageSizing;
	uniform vec3 gravity;

	layout(location=0) out vec4 rec_position;
	layout(location=5) out vec4 rec_velocity;
	layout(location=6) out vec4 rec_aux0;
	layout(location=7) out vec4 rec_aux1;

	// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
	// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

	void main() {
		float sizeFact = pow(1.0/(dt+1.0), 2.0);
		float newLifeTime = vertex_in[0].aux1.x - (dt * aging);
		vec4 newPos = vec4(vertex_in[0].position.x,
				vertex_in[0].position.y,
				vertex_in[0].position.z, 1.0) + vec4(vertex_in[0].velocity.rgb * dt, 0.0);

		// if the particle still lives, emit it
		int bounds = int(newPos.x > 1.0) + int(newPos.x < -1.0) + int(newPos.y < -1.0) + int(newPos.y > 1.0) + int(newLifeTime <= 0.0);

		if (bounds * checkBounds == 0) {
			rec_position = newPos;
			rec_velocity = vec4((vertex_in[0].velocity.rgb + gravity) * friction, vertex_in[0].velocity.w);
			rec_aux0 = vec4(max(vertex_in[0].aux0.x * (sizeFact * ageSizing + 1.0 -ageSizing), 0.005), vertex_in[0].aux0.y, vertex_in[0].aux0.z + vertex_in[0].velocity.w, vertex_in[0].aux0.w);
			rec_aux1.x = newLifeTime;
			rec_aux1.a = max(rec_aux1.x, ageFading); // copy initial lifetime, if agefading is disable this will stay 0

			gl_Position = newPos;

			EmitVertex();
			EndPrimitive();
		}
	});

	geom = "// GLSLParticleSystem2 shader, UpdateShdr geom\n" + shdr_Header + geom;

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;
	void main() {
		color = vec4(1.0);
	});

	frag = "// GLSLParticleSystem2 shader, UpdateShdr frag\n" + shdr_Header + frag;

	return shCol->addCheckShaderTextNoLink(
			"GLSLPS2Updt" + std::to_string(ind),
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initEmitShdr(unsigned int ind)
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
	layout(location=0) in vec4 position;
	layout(location=5) in vec4 velocity;
	layout(location=6) in vec4 aux0;
	layout(location=7) in vec4 aux1;

	layout(location=0) out vec4 rec_position;
	layout(location=5) out vec4 rec_velocity;
	layout(location=6) out vec4 rec_aux0;
	layout(location=7) out vec4 rec_aux1;

	uniform float dt;
	uniform float lifeTime;
	uniform float aging;
	uniform float ageSizing;
	uniform vec3 gravity;

	uniform mat4 m_pvm;

	void main() {
		float sizeFact = pow(1.0/(dt+1.0), 2.0);

		rec_position = vec4(position.x, position.y, position.z, 1.0);
		rec_velocity = vec4(velocity.rgb + gravity, velocity.w);
		rec_aux0 = vec4(max(aux0.x * (sizeFact * ageSizing + 1.0 -ageSizing), 0.005), aux0.y, aux0.z, aux0.w);
		rec_aux1 = vec4(lifeTime * position.w, 1.0, 0.0, aux1.a);// aux1.a = alpha

		gl_Position = m_pvm * position;
	});

	vert = "// GLSLParticleSystem2, EmitShdr vert\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		void main(){
			color = vec4(1.0);
	});

	frag = "// GLSLParticleSystem2 shader, EmitShdr frag\n" + shdr_Header + frag;

	return shCol->addCheckShaderTextNoLink(
			"GLSLParticleSystem2EmitShdr" + std::to_string(ind), vert.c_str(),
			frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initEmitTexShdr(unsigned int ind)
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout(location=0) in vec4 position; layout(location=5) in vec4 velocity; layout(location=6) in vec4 aux0; layout(location=7) in vec4 aux1;

					out vec4 pos;

					uniform float nrEmitTrig; uniform mat4 m_pvm;

					void main() { pos = position; gl_Position = m_pvm * position; });

	vert = "// GLSLParticleSystem2, EmitTex vert\n" + shdr_Header + vert;

	// max_vertices war 90...
	std::string geom =
			STRINGIFY(
					layout (points) in; layout (points, max_vertices = 50) out;

					in vec4 pos[];

					uniform int nrEmitTrig; uniform ivec2 winSize; uniform vec2 texSize; uniform sampler2D posTex;

					uniform int emitLimit;

					uniform float speed; uniform float size; uniform float colInd; uniform float angle; uniform float texNr; uniform float alpha;

					uniform float dt; uniform float lifeTime; uniform float aging; uniform float ageSizing; uniform vec3 gravity;

					uniform vec3 massCenter;

					layout(location=0) out vec4 rec_position; layout(location=5) out vec4 rec_velocity; layout(location=6) out vec4 rec_aux0; layout(location=7) out vec4 rec_aux1;

					ivec2 texPos; vec4 posTexFrag; vec4 outV;

					// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
					// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

					void main() { int emitCntr = 0;

					// in dem definierten auschnitt gehe durch die Emit Textur durch
					for (int y=0;y<winSize.y+1;y++) { if (emitCntr > emitLimit) break;

					for (int x=0;x<winSize.x+1;x++) { if (emitCntr > emitLimit) break;

					texPos = ivec2(int(pos[0].x * texSize.x) + x, int(pos[0].y * texSize.y) + y); posTexFrag = texelFetch(posTex, texPos, 0);

					if (posTexFrag.r > 0.1) { outV = vec4(float(texPos.x) / texSize.x * 2.0 - 1.0, (1.0 - (float(texPos.y) / texSize.y)) * 2.0 - 1.0, 0.0, 1.0);

					rec_position = outV; gl_Position = outV;

					rec_velocity = vec4(0.2, 0.0, 0.0, 0.0); rec_aux0 = vec4(size, colInd, angle, texNr); rec_aux1 = vec4(lifeTime, 1.0, 0.0, alpha);

					EmitVertex(); EndPrimitive();

					emitCntr++; } } } });

	geom = "// GLSLParticleSystem2 shader, EmitTex geom\n" + shdr_Header + geom;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; void main(){ color = vec4(1.0, 1.0, 1.0, 1.0); });

	frag = "// GLSLParticleSystem2 shader, frag\n" + shdr_Header + frag;

	return shCol->addCheckShaderTextNoLink(
			"GLSLParticleSystem2EmitTex" + std::to_string(ind), vert.c_str(),
			geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initQuadShdr()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout(location=0) in vec4 position;
		layout(location=5) in vec4 velocity;
		layout(location=6) in vec4 aux0;
		layout(location=7) in vec4 aux1;

		out VS_GS_VERTEX {
			vec4 aux0;
			vec4 aux1;
		} vertex_out;

		void main() {
			vertex_out.aux0 = aux0;
			vertex_out.aux1 = aux1;
			gl_Position = position;
		});

	vert = "// GLSLParticleSystem2, QuadShdr vert\n" + shdr_Header + vert;


	std::string geom = STRINGIFY(
	layout (points) in;
	layout (triangle_strip, max_vertices = 4) out;

	in VS_GS_VERTEX {
		vec4 aux0;
		vec4 aux1;
	} vertex_in[];

	out GS_FS_VERTEX {
		vec2 tex_coord;
		vec4 aux_par0;
	} vertex_out;

	mat4 trans_matrix;
	uniform mat4 m_pvm;
	uniform float numPart;
	vec4 oPos;

	void main()
	{
		if (vertex_in[0].aux1.x > 0.0)
		{
			for (int i=0;i<4;i++)
			{
				// Erstelle eine Matrize zum drehen und verschieben
				// aux_par.b = angle
				vec4 column0 = vec4(cos(vertex_in[0].aux0.z), sin(vertex_in[0].aux0.z), 0.0, 0.0);
				vec4 column1 = vec4(-sin(vertex_in[0].aux0.z), cos(vertex_in[0].aux0.z), 0.0, 0.0);
				vec4 column2 = vec4(0.0, 0.0, 1.0, 0.0);
				vec4 column3 = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0);

				//vec4 column3 = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0);
				trans_matrix = mat4(column0, column1, column2, column3);

				// Ermittle die Position der Koordinaten aux_par.r = size, gl_Position ist der Mittelpunkt
				oPos = vec4( (i == 1 || i == 3) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x,
							 (i == 0 || i == 1) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x, 0.0, 1.0);

				gl_Position = m_pvm * trans_matrix * oPos;
				vertex_out.aux_par0 = vertex_in[0].aux0;

				// aus effizienzgründen wird hier die aux_par.b koordinate mit dem alpha wert in life.a überschrieben
				vertex_out.aux_par0.b = vertex_in[0].aux1.a;
				vertex_out.tex_coord = vec2((i == 1 || i == 3) ? 1.0 : 0.0, (i == 0 || i == 1) ? 1.0 : 0.0);

				EmitVertex();
			}
			EndPrimitive();
		}
	});

	geom = "// GLSLParticleSystem2 shader, QuadShdr geom\n" + shdr_Header + geom;

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;

	in GS_FS_VERTEX {
		vec2 tex_coord;
		vec4 aux_par0; // aux_par0: (r: size, g: farbIndex (0-1), b: angle, a: textureUnit)
	} vertex_in;

	uniform sampler2D texs[8];

	void main() {
		vec4 tex = texture(texs[int(vertex_in.aux_par0.a)], vertex_in.tex_coord);
		if (tex.a < 0.05){
			discard;
		} else {
			color = vec4( tex.rgb, tex.a );
//		color = vec4( tex.rgb, tex.a * vertex_in.aux_par0.b );
		}
	});


	frag = "// GLSLParticleSystem2 shader, QuadShdr frag\n" + shdr_Header + frag;

	return shCol->addCheckShaderTextNoLink("GLSLParticleSystem2QuadShdr",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initVelTexShdr()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout(location=0) in vec4 position;
		layout(location=5) in vec4 velocity;
		layout(location=6) in vec4 aux0;
		layout(location=7) in vec4 aux1;

		out vec2 tex_coord;
		out vec2 tex_coord2;

		uniform float step_width;
		uniform mat4 m_pvm;

		void main() {
			tex_coord = vec2(position.x * 0.5 + 0.5, position.y * 0.5 + 0.5);
			tex_coord2 = vec2(position.x * 0.5 + 0.5 + step_width, position.y * 0.5 + 0.5 + step_width);
			gl_Position = m_pvm * position;
		});

	vert = "// GLSLParticleSystem2, VelTex vert\n" + shdr_Header + vert;

	std::string geom = STRINGIFY(
		layout (points) in;
		layout (points, max_vertices = 1) out;

		in vec2 tex_coord[];
		in vec2 tex_coord2[];

		uniform sampler2D velTex;
		layout(location=5) out vec4 rec_velocity;

		void main() {
			vec4 velTexFrag = texture(velTex, tex_coord[0]);
			vec4 velTexFragRight = texture(velTex, tex_coord2[0]);
			float angleOffset = dot(velTexFrag.xy, velTexFragRight.xy);

			rec_velocity = vec4(velTexFrag.rgb * 0.1, 1.0);
//					rec_velocity = vec4(velTexFrag.rgb * 0.1, angleOffset * 0.0005);

			// Copy the input position to the output
			//    gl_Position = gl_in[0].gl_Position;
			gl_Position = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0);

			EmitVertex();
			EndPrimitive();
		});

	geom = "// GLSLParticleSystem2 shader, VelTex geom\n" + shdr_Header + geom;


	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		void main() {
			color = vec4(1.0, 0.0, 0.0, 1.0);
		});

	frag = "// GLSLParticleSystem2 shader, VelTex frag\n" + shdr_Header + frag;

	return shCol->addCheckShaderTextNoLink("GLSLParticleSystem2VelTex",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initOrgTexShdr()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout(location=0) in vec4 position;
		layout(location=5) in vec4 velocity;
		layout(location=6) in vec4 aux0;
		layout(location=7) in vec4 aux1;

		out vec2 tex_coord;
		out vec2 tex_coord2;

		uniform float step_width;
		uniform float reposFact;
		uniform samplerBuffer orgPosTbo;
		uniform mat4 m_pvm;

		void main() {
			tex_coord = vec2(position.x * 0.5 + 0.5, position.y * 0.5 + 0.5);
			tex_coord2 = vec2(position.x * 0.5 + 0.5 + step_width, position.y * 0.5 + 0.5 + step_width);

			vec4 orgPos = texelFetch(orgPosTbo, gl_VertexID);

			gl_Position = m_pvm * (position * (1.0 - reposFact) + orgPos * reposFact);
	});

	vert = "// GLSLParticleSystem2, OrgTex vert\n" + shdr_Header + vert;


	std::string geom = STRINGIFY(
		layout (points) in;
		layout (points, max_vertices = 1) out;

		in vec2 tex_coord[];
		in vec2 tex_coord2[];

		uniform sampler2D velTex;

		layout(location=0) out vec4 rec_position;
		layout(location=5) out vec4 rec_velocity;

		void main() {
			vec4 velTexFrag = texture(velTex, tex_coord[0]);
			vec4 velTexFragRight = texture(velTex, tex_coord2[0]);
			float angleOffset = dot(velTexFrag.xy, velTexFragRight.xy);

			rec_velocity = vec4(velTexFrag.rgb * 0.1, angleOffset * 0.0005);

			// Copy the input position to the output
			gl_Position = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0);
			rec_position = gl_Position;

			EmitVertex();
			EndPrimitive();
	});

	geom = "// GLSLParticleSystem2 shader, OrgTex geom\n" + shdr_Header + geom;


	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		void main(){
			color = vec4(1.0, 0.0, 0.0, 1.0);
		});

	frag = "// GLSLParticleSystem2 shader, OrgTex frag\n" + shdr_Header + frag;


	return shCol->addCheckShaderTextNoLink("GLSLParticleSystem2OrgTex",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initReturnToOrgShdr()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout(location=0) in vec4 position;
		layout(location=0) out vec4 rec_position;

		uniform samplerBuffer orgPosTbo;
		uniform float reposFact;
		uniform mat4 m_pvm;

		void main() {
			vec4 orgPos = texelFetch(orgPosTbo, gl_VertexID);
			rec_position = position * (1.0 - reposFact) + orgPos * reposFact;

			gl_Position = m_pvm * position;
		});

	vert = "// GLSLParticleSystem2, ReturnToOrg vert\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		void main(){
			color = vec4(1.0, 0.0, 0.0, 1.0);
		});


	frag = "// GLSLParticleSystem2 shader, ReturnToOrg frag\n" + shdr_Header
			+ frag;

	return shCol->addCheckShaderTextNoLink("GLSLParticleSystem2ReturnToOrg",
			vert.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

Shaders* GLSLParticleSystem2::initQuadSceneNodeBlendShdr()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout(location=0) in vec4 position; layout(location=4) in vec4 texCorMod; layout(location=6) in vec4 aux0; layout(location=7) in vec4 aux1; layout(location=10) in mat4 modMatr;

					uniform int useInstancing; uniform int texNr;

					uniform mat4 modelMatrix; uniform mat4 projectionMatrix;

					out VS_GS_VERTEX { vec4 position; vec4 aux0; vec4 aux1; } vertex_out;

					mat4 MVM;

					void main(void) { vertex_out.aux0 = aux0; vertex_out.aux1 = aux1;
					//    vertex_out.texCoord = useInstancing == 0 ? texCoord : texCoord * vec2(texCorMod.b, texCorMod.a) + vec2(texCorMod.r, texCorMod.g);

					MVM = (useInstancing == 0 ? modelMatrix : modMatr); vertex_out.position = MVM * position;
					// vertex_out.normal = normalize((MVM * vec4(normal, 0.0)).xyz);

					gl_Position = vertex_out.position; });

	vert = "// GLSLParticleSystem2, QuadSceneNodeBlend vert\n" + shdr_Header
			+ vert;

	std::string geom =
			STRINGIFY(
					layout (points) in; layout (triangle_strip, max_vertices = 4) out;

					in VS_GS_VERTEX { vec4 position; vec4 aux0; vec4 aux1; } vertex_in[];

					uniform vec4 auxCol0; uniform vec4 auxCol1; uniform vec4 auxCol2; uniform vec4 auxCol3;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					mat4 trans_matrix;

					// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
					// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

					void main() { for (int i=0;i<4;i++) {
					// aux0.b = angle
					vec4 column0 = vec4(cos(vertex_in[0].aux0.b), sin(vertex_in[0].aux0.b), 0.0, 0.0); vec4 column1 = vec4(-sin(vertex_in[0].aux0.b), cos(vertex_in[0].aux0.b), 0.0, 0.0); vec4 column2 = vec4(0.0, 0.0, 1.0, 0.0); vec4 column3 = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0); trans_matrix = mat4(column0, column1, column2, column3);

					// aux0.x = size
					gl_Position = vec4((i == 1 || i == 3) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x, (i == 0 || i == 1) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x, 0.0, 1.0);

					rec_position = trans_matrix * gl_Position; gl_Position = rec_position;

					// hack: die texCoord hat 4 koordinaten, die beiden letzen koordinaten
					// werden für rec_texCoord.z <- aux0.w (texUnit)
					// und rec_texCoord.w <- aux0.y (farbIndex) werwendet
					rec_texCoord = vec4((i == 1 || i == 3) ? 1.0 : 0.0, (i == 0 || i == 1) ? 1.0 : 0.0, vertex_in[0].aux0.w, vertex_in[0].aux0.y); rec_normal = vec3(0.0, 0.0, -1.0);

					// hier wird die lebenszeit auf den alpha wert gemappt
					// und mit dem alpha wert von aux1.a multipliziert
					// pseudo tiefeneffekt: die z Koordinate zieht das alpha runter
					float depth = max(2.0 + gl_in[0].gl_Position.z, 0.0); vec4 particCol = max(1.0 - vertex_in[0].aux0.y *2.0, 0.0) *auxCol0 + max(1.0 - abs(vertex_in[0].aux0.y *2.0 -1.0), 0.0) *auxCol1 + max((vertex_in[0].aux0.y -0.5) *2.0, 0.0) *auxCol2; rec_color = vec4(particCol.rgb, vertex_in[0].aux1.y * vertex_in[0].aux1.a);
					//        rec_color = vec4(depth, depth, depth, vertex_in[0].aux1.y * vertex_in[0].aux1.a );
					EmitVertex(); } EndPrimitive(); });

	geom = "// GLSLParticleSystem2 shader, QuadSceneNodeBlend geom\n"
			+ shdr_Header + geom;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; void main() { color = vec4(1); });

	frag = "// GLSLParticleSystem2 shader, QuadSceneNodeBlend frag\n"
			+ shdr_Header + frag;

	return shCol->addCheckShaderTextNoLink(
			"GLSLParticleSystem2QuadSceneNodeBlend", vert.c_str(), geom.c_str(),
			frag.c_str());
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setCheckBounds(float _check)
{
	checkBounds = _check;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setLifeTime(float _lifeTime)
{
	lifeTime = _lifeTime;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setGravity(glm::vec3 _gravity)
{
	gravity = _gravity;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setAging(bool _age)
{
	doAging = _age ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setFriction(float _friction)
{
//        friction = 1.0f - pow(_friction, 4.f);
	friction = _friction;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setAgeFading(bool _set)
{
	ageFading = _set ? 0.f : 1.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setAgeSizing(bool _set)
{
	ageSizing = _set ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setColorPal(glm::vec4* chanCols)
{
	for (auto i = 0; i < MAX_NUM_COL_SCENE; i++)
		colorPal[i] = chanCols[i];
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setVelTexAngleStepSize(float _size)
{
	velTexAngleStepSize = _size;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::setReturnToOrg(bool _set, float _reposFact)
{
	returnToOrg = _set;
	reposFact = _reposFact;
}

//------------------------------------------------------------------------

void GLSLParticleSystem2::copyEmitData(EmitData& fromData, EmitData& toData)
{
	toData.emitOrg = fromData.emitOrg;
	toData.emitVel = fromData.emitVel;
	toData.speed = fromData.speed;
	toData.life = fromData.life;
	toData.size = fromData.size;
	toData.angle = fromData.angle;
	toData.texUnit = fromData.texUnit;
	toData.texNr = fromData.texNr;
	toData.posRand = fromData.posRand;
	toData.dirRand = fromData.dirRand;
	toData.speedRand = fromData.speedRand;
	toData.sizeRand = fromData.sizeRand;
	toData.angleRand = fromData.angleRand;
	toData.texRand = fromData.texRand;
	toData.lifeRand = fromData.lifeRand;
	toData.colInd = fromData.colInd;
	toData.alpha = fromData.alpha;
}

}
