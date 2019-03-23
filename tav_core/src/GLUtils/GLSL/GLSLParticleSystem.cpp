//
//  GLSLParticleSystem.cpp
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
//
//  DIESE KLASSE SOLLTE REDUNDANT SEIN, GLSLParticleSystem2 benutzen!!!!
//

#include "pch.h"
#include "GLSLParticleSystem.h"

namespace tav
{
GLSLParticleSystem::GLSLParticleSystem(ShaderCollector* _shCol,
		int _maxNrParticles, int _screenWidth, int _screenHeight) :
		maxNrParticles(_maxNrParticles), screenWidth(_screenWidth), screenHeight(
				_screenHeight), shCol(_shCol), reposFact(0.f), lifeTime(1.f), doAging(
				1.f), friction(0.f), ageFading(0.f), colorInd(0.f), nrTfos(2), tfoPtr(
				0), nrEmitTrig(64)
{
	velTexAngleStepSize = 1.f / static_cast<float>(_screenWidth);

	// vao to init the particle system
	part = new VAO("position:4f,velocity:4f,aux0:4f,aux1:4f", GL_STATIC_DRAW);
	part->initData(maxNrParticles);

	// trigger vao for the geometry shader, just a normalized grid ([0-1;0-1])
	GLfloat initEmitPos[nrEmitTrig * nrEmitTrig * 4];
	for (int y = 0; y < nrEmitTrig; y++)
	{
		for (int x = 0; x < nrEmitTrig; x++)
		{
			initEmitPos[(y * nrEmitTrig + x) * 4] = 1.f
					/ static_cast<float>(nrEmitTrig) * static_cast<float>(x);
			initEmitPos[(y * nrEmitTrig + x) * 4 + 1] = 1.f
					/ static_cast<float>(nrEmitTrig) * static_cast<float>(y);
			initEmitPos[(y * nrEmitTrig + x) * 4 + 2] = 0.f;
			initEmitPos[(y * nrEmitTrig + x) * 4 + 3] = 1.f;
		}
	}

	emitTrigVao = new VAO("position:4f", GL_STATIC_DRAW);
	emitTrigVao->initData(nrEmitTrig * nrEmitTrig, initEmitPos);

	nrEmitTbos = 4;
	nrEmitTbosCoords = 4;

	tfoBufAttachTypes = new coordType[nrEmitTbos];
	tfoBufAttachTypes[0] = tav::POSITION;
	tfoBufAttachTypes[1] = tav::VELOCITY;
	tfoBufAttachTypes[2] = tav::AUX0;
	tfoBufAttachTypes[3] = tav::AUX1;

	// create a textureBuffer for the emit information
	GLfloat* emitInit = new GLfloat[maxNrParticles * nrEmitTbosCoords];
	for (unsigned int i = 0; i < maxNrParticles * nrEmitTbosCoords; i++)
		emitInit[i] = 0.0f;

	emitTbs = new TextureBuffer*[nrEmitTbos];
	for (unsigned int i = 0; i < nrEmitTbos; i++)
		emitTbs[i] = new TextureBuffer(maxNrParticles, nrEmitTbosCoords,
				emitInit);

	initStateTb = new TextureBuffer*[nrEmitTbos];
	for (unsigned int i = 0; i < nrEmitTbos; i++)
		initStateTb[i] = new TextureBuffer(maxNrParticles, nrEmitTbosCoords,
				emitInit);

	//velUpdtTb = new TextureBuffer(maxNrParticles, nrEmitTbosCoords, emitInit);

	emitPtr = new unsigned int[2];
	for (auto i = 0; i < 2; i++)
		emitPtr[i] = 0;

	emitCntr = new unsigned int[2];
	for (auto i = 0; i < 2; i++)
		emitCntr[i] = 0;

	cam = new GLMCamera(GLMCamera::FRUSTUM, screenWidth, screenHeight - 1.0f,
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
	for (unsigned int i = 0; i < nrEmitTbos; i++)
		recAttribNames.push_back(stdRecAttribNames[tfoBufAttachTypes[i]]);

	tfos = new TFO*[nrTfos];
	partRecShaders = new Shaders*[nrTfos];

	for (unsigned int i = 0; i < nrTfos; i++)
	{
		tfos[i] = new TFO(maxNrParticles, recAttribNames);
		// muss leider mehrfach geladen werden, wegen den TFOs...

		// aus irgendwelchen Gründen funktioniert das lesen der velocity textur nicht,
		// wenn der Shader mit Tfo läuft... vermutlich eine beschränkung der maximal
		// gebundenen Texturen...
		partRecShaders[i] = new Shaders("shaders/partEmit.vert",
				"shaders/partEmit.frag", true);

		tfos[i]->setVaryingsToRecord(&recAttribNames,
				partRecShaders[i]->getProgram());
		partRecShaders[i]->link();
	}

	quadShader = shCol->addCheckShader("part2Quad", "shaders/part_quad.vert",
			"shaders/part_quad.geom", "shaders/part_quad.frag");
	quadShader->link();

	// setup
	recAttribNames.clear();
	recAttribNames.resize(0);
	recAttribNames.push_back(stdRecAttribNames[tav::VELOCITY]);

	velUptTfo = new TFO(maxNrParticles, recAttribNames);
	velTexShader = new Shaders("shaders/part_velTex.vert",
			"shaders/part_velTex.geom", "shaders/part_velTex.frag", true);
	velUptTfo->setVaryingsToRecord(&recAttribNames, velTexShader->getProgram());
	velTexShader->link();

	// return To Origin
	recAttribNames.clear();
	recAttribNames.resize(0);
	recAttribNames.push_back(stdRecAttribNames[tav::POSITION]);

	posUptTfo = new TFO(maxNrParticles, recAttribNames);
	returnToOrgShader = new Shaders("shaders/part_returnToOrg.vert",
			"shaders/part_rec.frag", true);
	posUptTfo->setVaryingsToRecord(&recAttribNames,
			returnToOrgShader->getProgram());
	returnToOrgShader->link();

	recAttribNames.push_back(stdRecAttribNames[tav::VELOCITY]);
	emitPosUptTfo = new TFO(maxNrParticles, recAttribNames);
	emitTexShader = new Shaders("shaders/part_emitTex.vert",
			"shaders/part_emitTex.geom", "shaders/part_emitTex.frag", true);
	emitPosUptTfo->setVaryingsToRecord(&recAttribNames,
			emitTexShader->getProgram());
	emitTexShader->link();
}

//------------------------------------------------------------------------

void GLSLParticleSystem::init(TFO* _tfo)
{
	quadSceneNodeBlendShdr = shCol->addCheckShader("partSceneQuad",
			"shaders/part_rec.vert", "shaders/part_rec.geom",
			"shaders/part_rec.frag");

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

GLSLParticleSystem::~GLSLParticleSystem()
{
	for (unsigned int i = 0; i < nrTfos; i++)
	{
		delete tfos[i];
		delete partRecShaders[i];
	}

	delete[] emitTbs;
	delete part;
	delete quadShader;
	delete velTexShader;
}

//-----------------------------------------------------------------------

void GLSLParticleSystem::emit(unsigned int nrPart, EmitData& data,
		bool remember, GLint posTex, int texWidth, int texHeight)
{
	// methode zum emittieren von Partikeln
	// Es wird direkt per Pointer in TextureBuffers geschrieben
	// in emitTbo0 kommt die position, wenn die w Koordinate auf 1 gesetzt ist, wird das Partikel emittiert
	// in emitTbo1 kommt der richtungsvektor (länge = geschwindigkeit)
	// emitTbo2: x: size, y: farbIndex (0-1), z: angle (z-rotation des quads um die eigene achse), w: textureUnit
	// emitTbo3: a: alpha

	emitPtr[1] = emitPtr[0];

	if (posTex != 0)
	{
		updateEmitPosTex(posTex, texWidth, texHeight);
	}
	else
	{
//            std::cout << "--------------------------------------------------------------------------------" << std::endl;
//            std::cout << "emit Pos " << std::endl;

		// setze das "emit-flag" und initialisiere position
		GLfloat* emit = emitTbs[0]->getMapBuffer();

		for (unsigned int i = 0; i < nrPart; i++)
		{
			int ind = ((emitPtr[0] + i) % maxNrParticles) * nrEmitTbosCoords;

			//  random, um rasterungen zu vermeiden
			glm::vec3 rEmitOrg =
					glm::vec3(
							data.emitOrg.x
									+ getRandF(-data.posRand, data.posRand),
							data.emitOrg.y
									+ getRandF(-data.posRand, data.posRand),
							data.emitOrg.z
									+ (data.emitOrg.z != 0.f ?
											getRandF(-data.posRand,
													data.posRand) :
											0.f));

			emit[ind] = rEmitOrg.x;
			emit[ind + 1] = rEmitOrg.y;
			emit[ind + 2] = rEmitOrg.z;
			// das emitter "flag", wenn groesser 0 -> emit
			emit[ind + 3] = data.life;
			// std::cout << glm::to_string(rEmitOrg) << std::endl;
		}
		emitTbs[0]->unMapBuffer();

//            std::cout << "--------------------------------------------------------------------------------" << std::endl;
//            std::cout << "emit vel " << std::endl;

		// initialisiere die richtung
		GLfloat* emitVelPtr = emitTbs[1]->getMapBuffer();
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
			// std::cout << glm::to_string(resV) << std::endl;
		}
		emitTbs[1]->unMapBuffer();
	}

//        std::cout << "--------------------------------------------------------------------------------" << std::endl;
//        std::cout << "aux0 " << std::endl;

	// initialisiere die aux parameter
	GLfloat* emitAuxPtr = emitTbs[2]->getMapBuffer();
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

		// std::cout << glm::to_string(glm::vec4(emitAuxPtr[ind], emitAuxPtr[ind+1], emitAuxPtr[ind+2], emitAuxPtr[ind+3])) << std::endl;

	}
	emitTbs[2]->unMapBuffer();

//        std::cout << "--------------------------------------------------------------------------------" << std::endl;
//        std::cout << "aux1 " << std::endl;

	// initialisiere die aux2 parameter
	GLfloat* emitAux2Ptr = emitTbs[3]->getMapBuffer();
	for (unsigned int i = 0; i < nrPart; i++)
	{
		int ind = ((emitPtr[0] + i) % maxNrParticles) * nrEmitTbosCoords;
		emitAux2Ptr[ind + 3] = data.alpha;
	}
	emitTbs[3]->unMapBuffer();

	emitPtr[0] = (emitPtr[0] + nrPart) % maxNrParticles;
	// std::cout << "emitPtr[0] : " << emitPtr[0] << std::endl;

	emittedNrPart.push_back(std::make_pair(nrPart, emitPtr[1]));

	// option for saving the emission state
	if (remember)
	{
		for (unsigned short i = 0; i < nrEmitTbos; i++)
		{
			// copy the calculated values to the actual tfo velocity buffer
			glBindBuffer(GL_COPY_READ_BUFFER, emitTbs[i]->getBuf());
			glBindBuffer(GL_COPY_WRITE_BUFFER, initStateTb[i]->getBuf());
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
					maxNrParticles * recCoTypeFragSize[tfoBufAttachTypes[i]]
							* sizeof(GLfloat));
		}

		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}
}

//-----------------------------------------------------------------------

void GLSLParticleSystem::clearEmitBuf()
{
	emitTbs[0]->clear();
	emittedNrPart.clear();
}

//------------------------------------------------------------------------

void GLSLParticleSystem::updateVelTex(GLint velTex)
{
	//glDisable(GL_RASTERIZER_DISCARD);

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
	glBindBuffer(GL_COPY_READ_BUFFER, velUptTfo->getTFOBuf(tav::VELOCITY));
	glBindBuffer(GL_COPY_WRITE_BUFFER,
			tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->getTFOBuf(tav::VELOCITY));
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
			maxNrParticles * recCoTypeFragSize[tav::VELOCITY]
					* sizeof(GLfloat));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

//------------------------------------------------------------------------

void GLSLParticleSystem::updateEmitPosTex(GLint emitTex, int texWidth,
		int texHeight)
{
	//glEnable(GL_RASTERIZER_DISCARD);

	emitTexShader->begin();
	emitTexShader->setIdentMatrix4fv("m_pvm");
	emitTexShader->setUniform1i("posTex", 0);
	emitTexShader->setUniform1f("nrEmitTrig", static_cast<float>(nrEmitTrig));
	emitTexShader->setUniform2i("winSize", texWidth / nrEmitTrig,
			texHeight / nrEmitTrig);
	emitTexShader->setUniform2f("texSize", static_cast<float>(texWidth),
			static_cast<float>(texHeight));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, emitTex);

	emitPosUptTfo->bind();
	emitPosUptTfo->begin(GL_POINTS);

	// schicke die emitTrigger Vertices
	emitTrigVao->draw(GL_POINTS);

	emitPosUptTfo->end();
	emitPosUptTfo->unbind();

	// glDisable(GL_RASTERIZER_DISCARD);

	/*
	 // copy the calculated values to the actual tfo position buffer
	 glBindBuffer(GL_COPY_READ_BUFFER, emitPosUptTfo->getTFOBuf(0));
	 glBindBuffer(GL_COPY_WRITE_BUFFER, tfos[(tfoPtr -1 +nrTfos) % nrTfos]->getTFOBuf(0));
	 glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
	 maxNrParticles * recCoTypeFragSize[tav::POSITION] * sizeof(GLfloat));

	 // copy the calculated values to the actual tfo velocity buffer
	 glBindBuffer(GL_COPY_READ_BUFFER, emitPosUptTfo->getTFOBuf(1));
	 glBindBuffer(GL_COPY_WRITE_BUFFER, tfos[(tfoPtr -1 +nrTfos) % nrTfos]->getTFOBuf(1));
	 glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
	 maxNrParticles * recCoTypeFragSize[tav::VELOCITY] * sizeof(GLfloat));

	 glBindBuffer(GL_COPY_READ_BUFFER, 0);
	 glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	 */
}

//------------------------------------------------------------------------

void GLSLParticleSystem::updateReturnToOrg(double time)
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

void GLSLParticleSystem::update(double time, bool draw, GLint velTex,
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

	if (!draw || drT == QUADS)
		glEnable(GL_RASTERIZER_DISCARD);

	// bind particle shader, start recording geometry
	partRecShaders[tfoPtr]->begin();
	partRecShaders[tfoPtr]->setUniform1f("dt", static_cast<float>(medDt));
	partRecShaders[tfoPtr]->setUniform1f("lifeTime", lifeTime);
	partRecShaders[tfoPtr]->setUniform1f("aging", doAging);
	partRecShaders[tfoPtr]->setUniform1f("friction", friction);
	partRecShaders[tfoPtr]->setUniform3fv("gravity", &gravity[0], 1);
	partRecShaders[tfoPtr]->setUniform1f("ageFading", ageFading);
	partRecShaders[tfoPtr]->setUniform1f("ageSizing", ageSizing);
	partRecShaders[tfoPtr]->setUniform1f("colorInd", colorInd);

	for (unsigned int i = 0; i < nrEmitTbos; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		partRecShaders[tfoPtr]->setUniform1i("emitTbo" + std::to_string(i), i);
		emitTbs[i]->bindTex();
	}

	cam->sendMVP(partRecShaders[tfoPtr]->getProgram(), "m_pvm");

	tfos[tfoPtr]->bind();
	tfos[tfoPtr]->begin(GL_POINTS);

	// erster durchlauf, zeichne partikel zum initialisieren des
	// ersten ping-pong buffers
	if (!isInited)
	{
		isInited = true;
		part->draw(GL_POINTS, tfos[tfoPtr], GL_POINTS);
	}
	else
	{
		// loesche initalen vao

		// nach erstem durchlauf, zeichne den letzen pingpong buffer und
		// nimm diesen auf den nächsten auf
		tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(GL_POINTS, tfos[tfoPtr],
				GL_POINTS, 1);
	}

	tfos[tfoPtr]->end();
	tfos[tfoPtr]->unbind();

	partRecShaders[tfoPtr]->end();

	if (!draw || drT == QUADS)
		glDisable(GL_RASTERIZER_DISCARD);

	tfoPtr = (tfoPtr + 1) % nrTfos;

	if (velTex != 0)
		updateVelTex(velTex);
	if (returnToOrg)
		updateReturnToOrg(time);

	if (emittedNrPart.size() > 0)
		clearEmitBuf();

	glUseProgram(curShdr);  // activiert wieder den ursprünglichen shader
	// werte muessen aber neu gesetzt werden...
}

//------------------------------------------------------------------------

void GLSLParticleSystem::draw(GLenum _mode, TFO* _tfo)
{
	tfos[(tfoPtr - 1 + nrTfos) % nrTfos]->draw(_mode, _tfo, _mode, 1);
}

//------------------------------------------------------------------------

void GLSLParticleSystem::drawToBlend(drawType drT, TFO* _tfo)
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

void GLSLParticleSystem::bindStdShader(drawType drT)
{
	switch (drT)
	{
	case POINTS:
		break;
	case QUADS:
		glDisable(GL_DEPTH_TEST);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		quadShader->begin();
		cam->sendMVP(quadShader->getProgram(), "m_pvm");
		quadShader->setUniform1iv("texs", texNrs, MAX_NUM_SIM_TEXS);

		break;
	default:
		break;
	}
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setLifeTime(float _lifeTime)
{
	lifeTime = _lifeTime;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setGravity(glm::vec3 _gravity)
{
	gravity = _gravity;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setAging(bool _age)
{
	doAging = _age ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setFriction(float _friction)
{
	friction = 1.0f - pow(_friction, 4.f);
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setAgeFading(bool _set)
{
	ageFading = _set ? 0.f : 1.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setAgeSizing(bool _set)
{
	ageSizing = _set ? 1.f : 0.f;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setColorPal(glm::vec4* chanCols)
{
	for (auto i = 0; i < MAX_NUM_COL_SCENE; i++)
		colorPal[i] = chanCols[i];
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setVelTexAngleStepSize(float _size)
{
	velTexAngleStepSize = _size;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::setReturnToOrg(bool _set, float _reposFact)
{
	returnToOrg = _set;
	reposFact = _reposFact;
}

//------------------------------------------------------------------------

void GLSLParticleSystem::copyEmitData(EmitData& fromData, EmitData& toData)
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
