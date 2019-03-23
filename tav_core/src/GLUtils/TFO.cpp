//
//  TFO.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 25.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLUtils/TFO.h"

namespace tav
{

// on amd radeon 6490M osx 4 seems to be max with separate buffers...

//----------------------------------------------------------------------

TFO::TFO(int _bufSize, std::vector<std::string>& _parNames) :
		bufSize(_bufSize), parNames(_parNames)
{
	init();
}

//----------------------------------------------------------------------

TFO::TFO() :
		bufSize(524288)
{
	for (auto i = 0; i < MAX_SEP_REC_BUFS; i++)
		parNames.push_back(stdRecAttribNames[i]);
	init();
}

//----------------------------------------------------------------------

void TFO::init()
{
	nrPar = static_cast<unsigned int>(parNames.size());

	if (nrPar > MAX_SEP_REC_BUFS)
		printf(
				"TFO Error: More than 4 separate Buffer requested!!! Actually only 4 supported by Hardware (radeon 6490m)\n");

	glGenTransformFeedbacks(1, &tfo); // id 0 is the default TFO
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo);

	buffers = new GLuint[nrPar];
	bufferSizes = new GLuint[nrPar];
	for (unsigned int i = 0; i < nrPar; i++)
		bufferSizes[i] = bufSize;
	tbos = new GLuint[nrPar];

	glGenBuffers(nrPar, &buffers[0]);     // generate buffers for all attributes

	unsigned int i = 0;
	for (std::vector<std::string>::iterator it = parNames.begin();
			it != parNames.end(); it++)
	{
		unsigned int fragSize = static_cast<unsigned int>(std::find(
				stdRecAttribNames.begin(), stdRecAttribNames.end(), (*it))
				- stdRecAttribNames.begin());

		locations.push_back(fragSize);
		fragSize = recCoTypeFragSize.at(fragSize);

		// Bind it to the TRANSFORM_FEEDBACK binding to create it
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffers[i]);

		// Call glBufferData to allocate 1MB of space
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,                  // target
				bufSize * fragSize * sizeof(float),   			// size
				NULL,                                         // no initial data
				GL_DYNAMIC_DRAW);                              // usage

		// Now we can bind it to indexed buffer binding points.
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, // target
				i,                            // index 0
				buffers[i],                   // buffer name
				0,                            // start of range
				bufSize * fragSize * sizeof(float));

		// fragSizes need below
		fragSizes.push_back(fragSize);
		i++;
	}

	// generate texture buffer for later access
	glGenTextures(nrPar, &tbos[0]);

	for (unsigned int i = 0; i < nrPar; i++)
	{
		glBindTexture(GL_TEXTURE_BUFFER, tbos[i]);
		switch (fragSizes[i])
		{
		case 1:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffers[i]);
			break;
		case 2:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, buffers[i]);
			break;
		case 3:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, buffers[i]);
			break;
		case 4:
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers[i]);
			break;
		}
	}

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

	// generate VAO for replay
	glGenVertexArrays(1, &resVAO);
	glBindVertexArray(resVAO);

	for (unsigned int i = 0; i < nrPar; i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[i]);
		glVertexAttribPointer(locations[i], fragSizes[i], GL_FLOAT, GL_FALSE, 0,
				0);
		glEnableVertexAttribArray(locations[i]);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	statColor = new GLfloat[4];
	statColor[0] = 0.f;
	statColor[1] = 0.f;
	statColor[2] = 0.f;
	statColor[3] = 1.f;

	recColors = new glm::vec4[MAX_NUM_COL_SCENE];
	for (int i = 0; i < MAX_NUM_COL_SCENE; i++)
		recColors[i] = glm::vec4(0.f, 0.f, 0.f, 1.f);

	srcMode = GL_SRC_ALPHA;
	dstMode = GL_ONE_MINUS_SRC_ALPHA;
}

//----------------------------------------------------------------------

TFO::~TFO()
{
	glDeleteTransformFeedbacks(1, &tfo);
	glDeleteBuffers(nrPar, tbos);
	glDeleteBuffers(nrPar, buffers);
}

//----------------------------------------------------------------------

// tfo doesn´t exist until this is called
void TFO::bind(bool _resetCounters)
{
	if (_resetCounters)
	{
		recVertOffs = 0;
		primitivesWritten = 0;
		totalPrimsWritten = 0;
		textures.clear();
		resetObjOffset();
	}

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo);
}

//----------------------------------------------------------------------

void TFO::unbind()
{
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
}

//----------------------------------------------------------------------

void TFO::setVaryingsToRecord(std::vector<std::string>* names, GLuint _prog)
{
	const char* vars[names->size()];

	int ind = 0;
	for (std::vector<std::string>::iterator it = names->begin();
			it != names->end(); ++it)
	{
		vars[ind] = (*it).c_str(), ind++;
	};

	glTransformFeedbackVaryings(_prog, (int) names->size(), (const char**) vars,
	GL_SEPARATE_ATTRIBS);
}

//----------------------------------------------------------------------

void TFO::setVaryingsToRecordInternNames(GLuint _prog)
{
	const char* vars[varyingsNames.size()];

	int ind = 0;
	for (std::vector<std::string>::iterator it = varyingsNames.begin();
			it != varyingsNames.end(); ++it)
	{
		vars[ind] = (*it).c_str(), ind++;
	};

	glTransformFeedbackVaryings(_prog, (int) varyingsNames.size(),
			(const char**) vars,
			GL_SEPARATE_ATTRIBS);
}

//----------------------------------------------------------------------

void TFO::begin(GLenum _mode)
{
	lastMode = _mode;
	glBeginTransformFeedback(_mode);
}

//----------------------------------------------------------------------

// tfo will still be active!!!! may cause gl errors
void TFO::pause()
{
	glPauseTransformFeedback();
}

//----------------------------------------------------------------------

void TFO::resume()
{
	glResumeTransformFeedback();
}

//----------------------------------------------------------------------

void TFO::end()
{
	glEndTransformFeedback();

	// read values from recorded buffer
	//            glm::vec4* pos = (glm::vec4*) glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	//            for (int i=0;i<20;i++)  std::cout << glm::to_string(pos[i]) <<  std::endl;
	//            glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
}

//----------------------------------------------------------------------

void TFO::pauseAndOffsetBuf(GLenum _mode)
{
	int offs = recVertOffs;

	end();

	for (unsigned int i = 0; i < nrPar; i++)
	{
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, // target
				i,                            // index 0
				buffers[i],                   // buffer name
				offs * fragSizes[i] * sizeof(float),         // start of range
				bufSize * fragSizes[i] * sizeof(float));

		// if ((bufSize * sizeof(float)) < (offs * coTypeFragSize[i] * sizeof(float))) printf("der \n");
	}

	begin(_mode);
}

//----------------------------------------------------------------------

void TFO::offsetBuf()
{
	for (unsigned int i = 0; i < nrPar; i++)
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, // target
				i,                            // index 0
				buffers[i],                   // buffer name
				recVertOffs * fragSizes[i] * sizeof(float),    // start of range
				bufSize * sizeof(float));
}

//----------------------------------------------------------------------

void TFO::incCounters(int nrVertices)
{
	recVertOffs += nrVertices;
	totalPrimsWritten += nrVertices;
	setObjOffset();
}

//----------------------------------------------------------------------

void TFO::decCounters(int nrVertices)
{
	recVertOffs -= nrVertices;
	totalPrimsWritten -= nrVertices;
}

//----------------------------------------------------------------------

// verschiedene draw and recToMode, z.B. fuer Geo-Amplification
void TFO::draw(GLenum _mode, TFO* _tfo, GLenum _recToMode, int geoAmpAmt)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recToMode);

	glBindVertexArray(resVAO);
	glDrawArrays(_mode, 0, totalPrimsWritten);
	glBindVertexArray(0);

	// wenn für transform feedback gerendert wird
	if (_tfo)
		_tfo->incCounters(totalPrimsWritten * geoAmpAmt);
}

//----------------------------------------------------------------------

// verschiedene draw and recToMode, z.B. fuer Geo-Amplification
void TFO::draw(GLenum _mode, GLuint offset, GLuint count, TFO* _tfo,
		GLenum _recToMode, int geoAmpAmt)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recToMode);

	glBindVertexArray(resVAO);
	glDrawArrays(_mode, offset, count);
	glBindVertexArray(0);

	// wenn für transform feedback gerendert wird
	if (_tfo)
		_tfo->incCounters(count * geoAmpAmt);
}

//----------------------------------------------------------------------

void TFO::setIndices(GLuint _count, GLenum _type, GLvoid* _indices)
{
	if (resElementBuffer == 0)
	{
		glGenBuffers(1, &resElementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _count * sizeof(_type), _indices,
				GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		resElementBufferSize = _count * sizeof(_type);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);

	// wenn die groesse sich geändert hat, erweitere den buffer
	if (resElementBufferSize != _count * sizeof(_type))
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _count * sizeof(_type), _indices,
				GL_DYNAMIC_DRAW);
	}
	else
	{
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _count * sizeof(_type),
				_indices);
	}
}

//----------------------------------------------------------------------

// verschiedene draw and recToMode, z.B. fuer Geo-Amplification
void TFO::drawElements(GLenum _mode, GLuint _count, GLenum _type, TFO* _tfo,
		GLenum _recToMode, int geoAmpAmt)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recToMode);

	if (resElementBuffer)
	{
		glBindVertexArray(resVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);
		glDrawElements(_mode, _count, _type, NULL);
		glBindVertexArray(0);
	}
	else
	{
		printf(
				"TFO::drawElements Error: no GL_ELEMENT_ARRAY_BUFFER created!!! \n");
	}

	// wenn für transform feedback gerendert wird
	if (_tfo)
		_tfo->incCounters(_count * geoAmpAmt);
}

//----------------------------------------------------------------------

void TFO::drawElementsBaseVertex(GLenum _mode, GLuint _count, GLenum _type,
		GLint basevertex, TFO* _tfo, GLenum _recToMode, int geoAmpAmt)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recToMode);

	if (resElementBuffer)
	{
		glBindVertexArray(resVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);
		glDrawElementsBaseVertex(_mode, _count, _type, NULL, basevertex);
		glBindVertexArray(0);
	}
	else
	{
		printf(
				"TFO::drawElementsBaseVertex Error: no GL_ELEMENT_ARRAY_BUFFER created!!! \n");
	}

	// wenn für transform feedback gerendert wird
	if (_tfo)
		_tfo->incCounters(_count * geoAmpAmt);
}

//----------------------------------------------------------------------

void TFO::resizeTFOBuf(coordType _nr, unsigned int size)
{
	std::vector<std::string>::iterator it = std::find(parNames.begin(),
			parNames.end(), stdRecAttribNames[_nr]);
	if (it != parNames.end())
	{
		unsigned int fragSize = recCoTypeFragSize.at(_nr);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[it - parNames.begin()]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,                  // target
				size * fragSize * sizeof(GLfloat),             // size
				NULL,                                         // no initial data
				GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		bufferSizes[it - parNames.begin()] = size;
	}
	else
		printf("TFO::resizeTFOBuf Error: index out of range \n");
}

//----------------------------------------------------------------------

GLuint TFO::getTFOBufSize(coordType _nr)
{
	GLuint out = 0;
	std::vector<std::string>::iterator it = std::find(parNames.begin(),
			parNames.end(), stdRecAttribNames[_nr]);
	if (it != parNames.end())
		out = bufferSizes[it - parNames.begin()];
	else
		printf("TFO::getTFOBufSize Error: index out of range \n");

	return out;
}

//----------------------------------------------------------------------

GLuint TFO::getTFOBuf(coordType _nr)
{
	GLuint out = 0;
	std::vector<std::string>::iterator it = std::find(parNames.begin(),
			parNames.end(), stdRecAttribNames[_nr]);
	if (it != parNames.end())
		out = buffers[it - parNames.begin()];
	else
		printf("TFO::getTFOBuf Error: index out of range \n");

	return out;
}

//----------------------------------------------------------------------

unsigned int TFO::getTFOBufFragSize(coordType _nr)
{
	unsigned int out = 0;
	std::vector<std::string>::iterator it = std::find(parNames.begin(),
			parNames.end(), stdRecAttribNames[_nr]);
	if (it != parNames.end())
	{
		out = recCoTypeFragSize.at(_nr);
	}
	else
		printf("TFO::resizeTFOBuf Error: index out of range \n");

	return out;
}

//----------------------------------------------------------------------

GLuint TFO::getId()
{
	return tfo;
}

//----------------------------------------------------------------------

GLuint TFO::getTbo(coordType _coord)
{
	return tbos[_coord];
}

//----------------------------------------------------------------------

GLuint TFO::getRecVertOffs()
{
	return recVertOffs;
}

//----------------------------------------------------------------------

GLuint TFO::getTotalPrimWritten()
{
	return totalPrimsWritten;
}

//----------------------------------------------------------------------

GLuint TFO::getBufSize()
{
	return bufSize;
}

//----------------------------------------------------------------------

GLuint TFO::getVao()
{
	return resVAO;
}

//----------------------------------------------------------------------

glm::vec4 TFO::getColor(int ind)
{
	return recColors[ind];
}

//----------------------------------------------------------------------

GLenum TFO::getLastMode()
{
	return lastMode;
}

//----------------------------------------------------------------------

void TFO::setBlendMode(GLenum _srcMode, GLenum _dstMode)
{
	srcMode = _srcMode;
	dstMode = _dstMode;
}

//----------------------------------------------------------------------

void TFO::setSceneNodeColors(glm::vec4* _cols)
{
	for (int i = 0; i < MAX_NUM_COL_SCENE; i++)
		recColors[i] = _cols[i];
}

//----------------------------------------------------------------------

void TFO::setSceneProtoName(std::string* _protoName)
{
	recProtoName = _protoName;
}

//----------------------------------------------------------------------

void TFO::setVaryingsNames(std::vector<std::string>& _names)
{
	for (std::vector<std::string>::iterator it = _names.begin();
			it != _names.end(); ++it)
		varyingsNames.push_back((*it));
}

//----------------------------------------------------------------------

void TFO::setRecNrVert(int _nr)
{
	recNrVert = _nr;
}

//----------------------------------------------------------------------

void TFO::setRecVertOffs(int _nr)
{
	recVertOffs = _nr;
}

//----------------------------------------------------------------------

void TFO::setObjOffset()
{
	if (static_cast<int>(objOffset.size()) == 0)
	{
		objOffset.push_back(std::make_pair(0, totalPrimsWritten));
	}
	else
	{
		unsigned int lastEnd = objOffset.back().first + objOffset.back().second;
		objOffset.push_back(
				std::make_pair(lastEnd, totalPrimsWritten - lastEnd));
	}
}

//----------------------------------------------------------------------

std::vector<std::pair<unsigned int, unsigned int> >* TFO::getObjOffsets()
{
	return &objOffset;
}

//----------------------------------------------------------------------

void TFO::resetObjOffset()
{
	objOffset.clear();
}

//----------------------------------------------------------------------

void TFO::incRecVertOffs(int _nr)
{
	recVertOffs += _nr;
}

//----------------------------------------------------------------------

void TFO::addTexture(int _unit, int _texInd, GLenum _target, std::string _name)
{
	// printf("TFO::AddTexture \n");
	bool found = false;
	for (std::vector<auxTexPar>::iterator it = textures.begin();
			it != textures.end(); ++it)
		if ((*it).unitNr == _unit)
		{
			found = true;
			//printf("TFO::AddTexture overwriting unit %d \n", _unit);
			(*it).target = _target;
			(*it).texNr = _texInd;
			(*it).unitNr = _unit;
			(*it).name = _name;
		}

	if (!found)
	{
		//  printf("TFO::AddTexture adding unit %d name %s \n", _unit, _name.c_str());

		textures.push_back(auxTexPar());
		textures.back().target = _target;
		textures.back().texNr = _texInd;
		textures.back().unitNr = _unit;
		textures.back().name = _name;
	}
}

//----------------------------------------------------------------------

void TFO::enableDepthTest()
{
	depthTest = true;
}

//----------------------------------------------------------------------

void TFO::disableDepthTest()
{
	depthTest = false;
}

//----------------------------------------------------------------------

void TFO::recallDepthTestState()
{
	if (!depthTest)
	{
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
	}
}

//----------------------------------------------------------------------

void TFO::recallBlendMode()
{
	glBlendFunc(srcMode, dstMode);
}

}
