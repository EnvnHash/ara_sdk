//
//  VAO.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 07.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  aims the attribute indices
//  VERTEX = 0
//  NORMAL = 1
//  TEXCOORD = 2
//  COLOR = 3
//  TEXCORMOD = 4,
//  MOD_MATR = 5
//  AUX0 = 6
//  AUX1 = 7
//  AUX2 = 8
//  AUX3 = 9
//
//  standardmaessig werden separate buffer für alle attribute gemacht, weil
//  praktischer für instancing
//
//  the data is supposed to be organized e.g. (e.g. vncvncvncvnc for 4 vertices, 4 normals, 4 colors)
//  shader locations are supposed to be
//  layout(location = 0) in vec4 vertex;
//  layout(location = 1) in vec4 normal;
//  layout(location = 2) in vec4 color;
//  layout(location = 3) in vec4 texCoords;
//  layout(location = 4) in vec4 model_matrix;
//  where the later consumes 4 locations (5-8)
//
//	webgl uynterstuetzt standardmaessig keine VAOS, es gibt eine Erweiterung, die ist aber nicht ueberall vorhanden
//  und der debugger stuerzt ab. deshalb kann mittles des Flags STRICT_WEBGL_1 ein Kompatibilitaetsmodus
//  erzwungen werden. Hierbei werden nur VBOs verwendet und ein AttribArray bei jedem draw neu gebunden
//  -> macht aber im Prinzip keine Unterschied, ist nur langsamer

#include "pch.h"
#include "GLUtils/VAO.h"

namespace tav
{
VAO::VAO(const char* _format, GLenum _storeMode,
		std::vector<coordType>* _instAttribs, int _maxNrInstances,
		bool _createBuffers) :
		storeMode(_storeMode), nrCoordsPerVert(3), nrElements(0), nrVertices(0), instAttribs(
				_instAttribs), maxNrInstances(_maxNrInstances), createBuffers(
				_createBuffers), drawNrVertices(0), elementBuffer(0), buffers(0)
{
	init(_format);
}

//--------------------------------------------------------------------

VAO::VAO(const char* _format, GLenum _storeMode, bool _createBuffers) :
		storeMode(_storeMode), nrCoordsPerVert(3), nrElements(0), nrVertices(0), instAttribs(
				nullptr), maxNrInstances(0), createBuffers(_createBuffers), drawNrVertices(
				0), elementBuffer(0), buffers(0)
{
	init(_format);
}

//--------------------------------------------------------------------

void VAO::init(const char* _format)
{
	for (int i = 0; i < COORDTYPE_COUNT; i++)
	{
		statCoords.push_back(new std::vector<GLfloat>());
		for (int j = 0; j < 4; j++)
			statCoords.back()->push_back(1.0f);
	}

	usesStaticCoords = new bool[COORDTYPE_COUNT];
	for (int i = 0; i < COORDTYPE_COUNT; i++)
		usesStaticCoords[i] = false;

	if (createBuffers)
		buffers.resize(COORDTYPE_COUNT, 0);
	instData = new GLfloat*[COORDTYPE_COUNT]; // data for buffer initialization and maybe later use

	// interpret the format std::string
	std::string strFormat = std::string(_format);
	std::vector<std::string> sepFormat = split(strFormat, ',');

	// separate the declarations by semicolon,
	for (std::vector<std::string>::iterator it = sepFormat.begin();
			it != sepFormat.end(); ++it)
	{
		std::vector<std::string> sep = split((*it), ':');
		if (static_cast<int>(sep.size()) < 2)
			printf("VAO: wrong format declaration!\n");

		attributes.push_back(new VertexAttribute());
		attributes.back()->setName((char*) sep[0].c_str());
		attributes.back()->location = static_cast<GLint>(find(
				stdAttribNames.begin(), stdAttribNames.end(), sep[0])
				- stdAttribNames.begin());
		attributes.back()->size = atoi(sep[1].c_str());
		attributes.back()->setType(sep[1][1]);

		if (sep[0].compare("modMatr") == 0)
			attributes.back()->nrConsecLocs = 4;
		if (sep[0].compare("position") == 0)
			nrCoordsPerVert = attributes.back()->size;
	}

// instanced drawing geht nur ueber extension in webgl
#ifndef __EMSCRIPTEN__
	// check das format, wenn eine attribut instanziert werden soll und
	// nicht vorhanden ist, füge es hinzu
	if (instAttribs)
	{
		for (std::vector<coordType>::iterator it = instAttribs->begin();
				it != instAttribs->end(); it++)
		{
			bool found = false;
			for (std::vector<VertexAttribute*>::iterator pIt =
					attributes.begin(); pIt != attributes.end(); pIt++)
			{
				// wenn das zu instanzierende attribute schon vorhanden ist, setzt es auf instanced
				if ((*pIt)->type == (*it))
				{
					(*pIt)->instDiv = 1; // könnte auch alles andere sein..
					found = true;
				}
			}
			// wenn nicht gefunden, füge es hinzu
			if (!found)
			{
				// spezial fall, modmatr braucht 4 locations (4 x 4) und wird in einen buffer geschrieben
				if ((*it) == MODMATR)
				{
					for (int i = 0; i < 4; i++)
					{
						attributes.push_back(new VertexAttribute());
						attributes.back()->setName(
								(char*) stdAttribNames[MODMATR].c_str());
						attributes.back()->location = MODMATR + i;
						attributes.back()->size = 4;
						attributes.back()->nrConsecLocs = 4;
						attributes.back()->stride = sizeof(glm::mat4);
						attributes.back()->pointer = (void*) (sizeof(glm::vec4)
								* i);
						attributes.back()->instDiv = 1; // könnte auch alles andere sein..
					}
				}
				else
				{
					attributes.push_back(new VertexAttribute());
					attributes.back()->setName(
							(char*) stdAttribNames[(*it)].c_str());
					attributes.back()->location = (*it);
					attributes.back()->size = coTypeStdSize[(*it)];
					attributes.back()->instDiv = 1; // könnte auch alles andere sein..
				}
			}

			// init the instData
			instData[(*it)] =
					new GLfloat[coTypeStdSize[(*it)] * maxNrInstances];

			if ((*it) != MODMATR)
			{
				for (int i = 0; i < maxNrInstances; i++)
					for (int j = 0; j < coTypeStdSize[(*it)]; j++)
						instData[(*it)][i * coTypeStdSize[(*it)] + j] = 0.0f;
			}
			else
			{
				glm::mat4 identMatr = glm::mat4(1.0f);
				for (int i = 0; i < maxNrInstances; i++)
					for (int j = 0; j < 4; j++)
						for (int k = 0; k < 4; k++)
							instData[(*it)][i * 16 + j * 4 + k] =
									identMatr[j][k];

			}
		}
	}
#endif

	//-- init buffers -------------------------------------------------------------------------------------

#ifndef STRICT_WEBGL_1
	glGenVertexArrays(1, &VAOId); // returns 1 unused names for use as VAO in the array VAO
	glBindVertexArray(VAOId); // create VAO, assign the name and bind that array
#endif
	for (int i = 0; i < static_cast<int>(attributes.size()); i++)
	{
		int attrIndex = attributes[i]->location;
		if (attrIndex < MODMATR || attrIndex > MODMATR + 3)
		{
			if (createBuffers)
			{
				glGenBuffers(1, &buffers[attrIndex]);
				glBindBuffer(GL_ARRAY_BUFFER, buffers[attrIndex]);
			}
			attributes[i]->enable();
		}
		else
		{
			// nur ein buffer für alle teile der mod matr
			if (attrIndex == MODMATR)
			{
				if (createBuffers)
				{
					glGenBuffers(1, &buffers[attrIndex]);
					glBindBuffer(GL_ARRAY_BUFFER, buffers[attrIndex]);
				}
				for (int j = 0; j < 4; j++)
					attributes[i + j]->enable();
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);     // create VAO, assign the name and bind that array
#endif
}

//--------------------------------------------------------------------

void VAO::initData(int nrVert, GLfloat* _data)
{
	if (createBuffers)
	{
		for (int i = 0; i < static_cast<int>(attributes.size()); i++)
		{
			coordType uploadType = POSITION;

			for (int j = 0; j < static_cast<int>(stdAttribNames.size()); j++)
				if (std::strcmp(stdAttribNames[j].c_str(), attributes[i]->name)
						== 0)
					uploadType = (coordType) j;

			if (_data)
			{
				upload(uploadType, _data, nrVert);
			}
			else
			{
				GLfloat* vals = new GLfloat[nrVert * attributes[i]->size];
				for (int j = 0; j < nrVert * attributes[i]->size; j++)
					vals[j] = 0.f;
				upload(uploadType, vals, nrVert);
			}
		}
	}
	else
	{
		printf("tav::VAO Error: Vao was build without buffers!!! \n");
	}
}

//-------------------------------------------------------------------------------------------
//--- Nummern angabe in Vertices (unanhaengig von der Groesse des Attibutes) ----------------

void VAO::upload(coordType _type, GLfloat* _entries, int _nrVertices)
{
	// check if coordType exists
	bool exists = false;
	int size = 0;
	GLuint buffer = 0;

	for (int i = 0; i < static_cast<int>(attributes.size()); i++)
	{
		if (std::strcmp(stdAttribNames[(int) _type].c_str(),
				attributes[i]->name) == 0)
		{
			exists = true;
			size = attributes[i]->size;
			buffer = buffers[attributes[i]->location];
		}
	}

	if (exists)
	{
		if (nrVertices == 0)
			nrVertices = _nrVertices;
		else
		{
			if (nrVertices != _nrVertices)
				std::cerr
						<< "VAO Warning: nr of Vertices to upload is not equivalent to the nr of vertices of the existing buffers"
						<< std::endl;
		}

#ifndef STRICT_WEBGL_1
		glBindVertexArray(VAOId);               // bind that array
#endif
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _nrVertices * size,
				_entries, storeMode);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
		glBindVertexArray(0);
#endif
	}
	else
	{
		std::cerr << ("VAO Error: coordType tried to upload doesn´t exist!")
				<< std::endl;
	}
}

//--------------------------------------------------------------------
#ifdef __EMSCRIPTEN__
void VAO::uploadFloat(unsigned int location, unsigned int size, GLfloat* _entries, int _nrVertices)
{
	if(nrVertices == 0)
	nrVertices = _nrVertices;
	else
	{
		if( nrVertices != _nrVertices)
		std::cerr << "VAO Warning: nr of Vertices to upload is not equivalent to the nr of vertices of the existing buffers" << std::endl;
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffers[location]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _nrVertices * size, _entries, storeMode);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#endif

//--------------------------------------------------------------------

void VAO::setElemIndices(unsigned int _COORDTYPE_COUNT, GLuint* _indices)
{
	//vindices = _indices;
	nrElements = _COORDTYPE_COUNT;

	if (elementBuffer == 0)
		glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * nrElements, _indices,
			storeMode);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//--------------------------------------------------------------------

void VAO::resize(unsigned int _newNrVertices)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
	int size = 0;
	for (int i = 0; i < static_cast<int>(attributes.size()); i++)
	{
		size = attributes[i]->size;
		int attrIndex = attributes[i]->location;

		glBindBuffer(GL_ARRAY_BUFFER, buffers[attrIndex]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _newNrVertices * size,
				nullptr, storeMode);

	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);     // create VAO, assign the name and bind that array
#endif
}

//--------------------------------------------------------------------

void VAO::bindBuffer(GLuint _inBuffer, GLenum _type)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);               // bind that array
#endif
	glBindBuffer(GL_ARRAY_BUFFER, _inBuffer);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

GLint VAO::addBuffer(coordType _type)
{
	std::string attribName = stdAttribNames[int(_type)];
	printf("VAO addBUffer name: %s \n", attribName.c_str());

	attributes.push_back(new VertexAttribute());
	attributes.back()->setName((GLchar*) attribName.c_str());
	attributes.back()->location = static_cast<GLint>(find(
			stdAttribNames.begin(), stdAttribNames.end(),
			stdAttribNames[int(_type)]) - stdAttribNames.begin());
	printf("location: %d \n", attributes.back()->location);

	attributes.back()->size = coTypeStdSize[int(_type)];
	printf("size: %d \n", attributes.back()->size);

	attributes.back()->stride = sizeof(float) * coTypeStdSize[int(_type)];
	attributes.back()->setType('f');

	if (attribName.compare("modMatr") == 0)
		attributes.back()->nrConsecLocs = 4;
	if (attribName.compare("position") == 0)
		nrCoordsPerVert = attributes.back()->size;

#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId); // create VAO, assign the name and bind that array
#endif
	glGenBuffers(1, &buffers[attributes.back()->location]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes.back()->location]);
	attributes.back()->enable();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);     // create VAO, assign the name and bind that array
#endif
	return buffers[attributes.back()->location];
}

//--------------------------------------------------------------------

void VAO::addExtBuffer(coordType _type, GLuint _buffer)
{
	std::string attribName = stdAttribNames[int(_type)];
	printf("VAO add external BUffer name: %s \n", attribName.c_str());

	attributes.push_back(new VertexAttribute());
	attributes.back()->setName((GLchar*) attribName.c_str());
	attributes.back()->location = static_cast<GLint>(find(
			stdAttribNames.begin(), stdAttribNames.end(),
			stdAttribNames[int(_type)]) - stdAttribNames.begin());
	printf("location: %d \n", attributes.back()->location);

	attributes.back()->size = coTypeStdSize[int(_type)];
	printf("size: %d \n", attributes.back()->size);

	attributes.back()->stride = sizeof(float) * coTypeStdSize[int(_type)];
	attributes.back()->setType('f');

	if (attribName.compare("modMatr") == 0)
		attributes.back()->nrConsecLocs = 4;
	if (attribName.compare("position") == 0)
		nrCoordsPerVert = attributes.back()->size;

#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId); // create VAO, assign the name and bind that array
#endif
	glBindBuffer(GL_ARRAY_BUFFER, _buffer);
	attributes.back()->enable();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);     // create VAO, assign the name and bind that array
#endif
}

//--------------------------------------------------------------------

#ifdef __EMSCRIPTEN__
GLint VAO::addBufferFloat(unsigned int location, unsigned int size, const char* name)
{
	attributes.push_back(new VertexAttribute());
	attributes.back()->setName((GLchar*) name);
	attributes.back()->location = location;
	attributes.back()->size = size;
	attributes.back()->stride = sizeof(float) * size;
	attributes.back()->setType('f');

	if(location >= COORDTYPE_COUNT)
	buffers.resize(location+1);

	glGenBuffers(1, &buffers[location]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[location]);
	attributes.back()->enable();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return buffers[location];
}

#endif

//--------------------------------------------------------------------

void VAO::bind()
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
}

//--------------------------------------------------------------------

void VAO::bindElementBuffer()
{
	if (elementBuffer != 0)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

		for (int i = 0; i < COORDTYPE_COUNT; i++)
			if (usesStaticCoords[i])
				glVertexAttrib4fv(i, &statCoords[i]->front());
	}
}

//--------------------------------------------------------------------

void VAO::unbindElementBuffer()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//--------------------------------------------------------------------

void VAO::unbind()
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

void VAO::draw(GLenum _mode)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawArrays(_mode, 0, nrVertices);

#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

void VAO::drawElements(GLenum _mode)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawElements(_mode, nrElements, elementBufferType, NULL);

#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
void VAO::draw(GLenum _mode, TFO* _tfo, GLenum _recMode)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recMode);

	glBindVertexArray(VAOId);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawArrays(_mode, 0, nrVertices);

	glBindVertexArray(0);

	// wenn für transform feedback gerendert wird
	if (_tfo)
		_tfo->incCounters(nrVertices);
}

//--------------------------------------------------------------------

void VAO::draw(GLenum _mode, unsigned int offset, unsigned int count, TFO* _tfo)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_mode);

	glBindVertexArray(VAOId);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawArrays(_mode, offset, count);

	glBindVertexArray(0);

	if (_tfo)
		_tfo->incCounters(count);
}

//--------------------------------------------------------------------

void VAO::draw(GLenum _mode, unsigned int offset, unsigned int count, TFO* _tfo,
		GLenum _recMode)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recMode);

	glBindVertexArray(VAOId);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawArrays(_mode, offset, count);

	glBindVertexArray(0);

	// wenn für transform feedback gerendert wird
	if (_tfo)
		_tfo->incCounters(count);
}

//--------------------------------------------------------------------

void VAO::drawInstanced(GLenum _mode, int nrInstances, TFO* _tfo, float nrVert)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_mode);

	glBindVertexArray(VAOId);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	drawNrVertices = static_cast<int>(static_cast<float>(nrVertices) * nrVert);

	//printf("draw vao nr inst %d, nrVert %d \n", nrInstances, drawNrVertices);

	glDrawArraysInstanced(_mode, 0, drawNrVertices, nrInstances);

	glBindVertexArray(0);

	if (_tfo)
		_tfo->incCounters(nrVertices * nrInstances);
}

//--------------------------------------------------------------------

void VAO::drawElements(GLenum _mode, TFO* _tfo, GLenum _recMode)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recMode);

	glBindVertexArray(VAOId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawElements(_mode, nrElements, elementBufferType, NULL);

	glBindVertexArray(0);

	if (_tfo)
		_tfo->incCounters(nrElements);
}

//--------------------------------------------------------------------

void VAO::drawElementsInst(GLenum _mode, int nrInstances, TFO* _tfo,
		GLenum _recMode)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recMode);

	glBindVertexArray(VAOId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawElementsInstanced(_mode, nrElements, elementBufferType, NULL,
			nrInstances);

	glBindVertexArray(0);

	if (_tfo)
		_tfo->incCounters(nrElements * nrInstances);
}

//--------------------------------------------------------------------

void VAO::drawElements(GLenum _mode, TFO* _tfo, GLenum _recMode,
		int _nrElements)
{
	if (_tfo)
		_tfo->pauseAndOffsetBuf(_recMode);

	glBindVertexArray(VAOId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	for (int i = 0; i < COORDTYPE_COUNT; i++)
		if (usesStaticCoords[i])
			glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawElements(_mode, _nrElements, elementBufferType, NULL);

	glBindVertexArray(0);

	if (_tfo)
		_tfo->incCounters(_nrElements);
}
#else

//--------------------------------------------------------------------

void VAO::draw(GLenum _mode, unsigned int offset, unsigned int count)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
	for (int i=0;i<COORDTYPE_COUNT;i++)
	if (usesStaticCoords[i])
	glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawArrays(_mode, offset, count);

#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

void VAO::draw(GLenum _mode, unsigned int offset, unsigned int count, GLenum _recMode)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
	for (int i=0;i<COORDTYPE_COUNT;i++)
	if (usesStaticCoords[i])
	glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawArrays(_mode, offset, count);

#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

void VAO::drawInstanced(GLenum _mode, int nrInstances, float nrVert)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
	for (int i=0;i<COORDTYPE_COUNT;i++)
	if (usesStaticCoords[i])
	glVertexAttrib4fv(i, &statCoords[i]->front());

	drawNrVertices = static_cast<int>(static_cast<float>(nrVertices) * nrVert);

	glDrawArraysInstanced(_mode, 0, drawNrVertices, nrInstances);
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

void VAO::drawElements(GLenum _mode, GLenum _recMode)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	for (int i=0;i<COORDTYPE_COUNT;i++)
	if (usesStaticCoords[i])
	glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawElements(_mode, nrElements, elementBufferType, NULL);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

//--------------------------------------------------------------------

void VAO::drawElements(GLenum _mode, GLenum _recMode, int _nrElements)
{
#ifndef STRICT_WEBGL_1
	glBindVertexArray(VAOId);
#else
	for (int i=0;i<static_cast<int>(attributes.size());i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[attributes[i]->location]);
		attributes[i]->enable();
	}
#endif
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	for (int i=0;i<COORDTYPE_COUNT;i++)
	if (usesStaticCoords[i])
	glVertexAttrib4fv(i, &statCoords[i]->front());

	glDrawElements(_mode, _nrElements, elementBufferType, NULL);
#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
#endif
}

#endif

//--------------------------------------------------------------------

void VAO::uploadMesh(Mesh* mesh)
{
	if (buffers.size() != 0 && mesh != nullptr)
	{
		if (mesh->usesIntrl())
		{
			nrVertices = mesh->getNrVertIntrl() / nrCoordsPerVert;
		}
		else
		{
			nrVertices = mesh->getNrPositions();
		}

#ifndef STRICT_WEBGL_1
		glBindVertexArray(VAOId);
#endif
		// if there are indices, allocate space and upload them
		if (mesh->usesIndices())
		{
			nrElements = mesh->getNrIndices();
			glGenBuffers(1, &elementBuffer); // generate one element buffer for later use

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
			if (mesh->usesUintIndices())
			{
				elementBufferType = GL_UNSIGNED_INT;
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->getByteSizeInd(),
						mesh->getIndiceUintPtr(), storeMode);
			}
			else
			{
				elementBufferType = GL_UNSIGNED_SHORT;
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->getByteSizeInd(),
						mesh->getIndicePtr(), storeMode);
			}
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		// Allocate space for the buffer and upload the data
		// upload data for mod matr later, will usually only be used for instancing
		for (int i = 0; i < static_cast<int>(attributes.size()); i++)
		{
			int attrIndex = attributes[i]->location;

			if ((attrIndex <= MODMATR || attrIndex > (MODMATR + 3))
					&& !attributes[i]->isStatic)
			{
				glBindBuffer(GL_ARRAY_BUFFER, buffers[attrIndex]);

				// if it´s a normal attribute
				if (attributes[i]->instDiv == 0)
				{
					glBufferData(GL_ARRAY_BUFFER, mesh->getByteSize(attrIndex),
							mesh->getPtr(attrIndex), storeMode);
				}
				else
				{
					// if it´s an instanced attribute, make space for the maxNrInstances
					// since for each attrib there´s is an extra buffer pointer is 0
					glBufferData(GL_ARRAY_BUFFER,
							attributes[i]->getByteSize() * maxNrInstances,
							&instData[attrIndex][0], storeMode);
				}

//                glBufferData(GL_ARRAY_BUFFER, mesh->getTotalByteSize(), mesh->getPtrInterleaved(), storeMode);
			}
		}
#ifndef STRICT_WEBGL_1
		glBindVertexArray(0);
#endif

		if (mesh->usesStaticColor())
			setStaticColor(mesh->getStaticColor());
		if (mesh->usesStaticNormal())
			setStaticNormal(mesh->getStaticNormal());

	}
	else
	{
		printf(
				"VAO::uploadMesh Error: VertexBufferObject or mesh was not generated\n");
	}
}

//--------------------------------------------------------------------

void VAO::setStaticNormal(float _x, float _y, float _z)
{
	setStatic(_x, _y, _z, 0, (char*) "normal", (char*) "normal:3f");
}

//--------------------------------------------------------------------

void VAO::setStaticNormal(std::vector<GLfloat>* _norm)
{
	setStatic(_norm->at(0), _norm->at(1), _norm->at(2), 0, (char*) "normal",
			(char*) "normal:3f");
}

//--------------------------------------------------------------------

void VAO::setStaticColor(float _r, float _g, float _b, float _a)
{
	setStatic(_r, _g, _b, _a, (char*) "color", (char*) "color:4f");
}

//--------------------------------------------------------------------

void VAO::setStaticColor(std::vector<GLfloat>* _col)
{
	setStatic(_col->at(0), _col->at(1), _col->at(2), _col->at(3),
			(char*) "color", (char*) "color:4f");
}

//--------------------------------------------------------------------

GLuint VAO::getVAOId()
{
	return VAOId;
}

//--------------------------------------------------------------------

GLuint VAO::getVBO(coordType _attrIndex)
{
	return buffers[_attrIndex];
}

//--------------------------------------------------------------------

void VAO::setStatic(float _r, float _g, float _b, float _a, char* _name,
		char* _format)
{
	GLuint _index = 0;
	int statCoordInd = 0;
	GLint size = 0;

	if (std::strcmp(_name, "color") == 0)
	{
		statCoordInd = COLOR;
	}
	else if (std::strcmp(_name, "normal") == 0)
	{
		statCoordInd = NORMAL;
	}
	size = coTypeStdSize[statCoordInd];
	_index = static_cast<int>(statCoordInd);

	statCoords[statCoordInd]->at(0) = _r;
	statCoords[statCoordInd]->at(1) = _g;
	statCoords[statCoordInd]->at(2) = _b;
	statCoords[statCoordInd]->at(3) = _a;

	// look if there is already an attribute for color
	bool found = false;

	std::vector<VertexAttribute*>::iterator it;
	for (it = attributes.begin(); it != attributes.end(); ++it)
	{
		if (std::strcmp((*it)->name, _name) == 0)
		{
			found = true;
			_index = (*it)->location;
		}
	}

	if (!found)
	{
		attributes.push_back(new VertexAttribute());
		attributes.back()->setName(_name);
		attributes.back()->location = _index;
		attributes.back()->size = _index;
		attributes.back()->setType('f');
		attributes.back()->isStatic = true;

	}
	else if (!usesStaticCoords[statCoordInd])
	{
#ifndef STRICT_WEBGL_1
		glBindVertexArray(VAOId);
		glDisableVertexAttribArray(_index); // disable the attribute
		glBindVertexArray(0);
#endif
	}

	usesStaticCoords[statCoordInd] = true;
}

#ifndef __EMSCRIPTEN__
void* VAO::getMapBuffer(coordType _attrIndex)
{
	void* outPtr = nullptr;

	glBindBuffer(GL_ARRAY_BUFFER, buffers[_attrIndex]);

	if (_attrIndex == MODMATR)
	{
		// get a pointer to the buffer
		glm::mat4* matrices = (glm::mat4*) glMapBuffer(GL_ARRAY_BUFFER,
				GL_WRITE_ONLY);
		outPtr = matrices;

	}
	else if (_attrIndex == POSITION || _attrIndex == NORMAL
			|| _attrIndex == VELOCITY)
	{
		// get a pointer to the buffer
		glm::vec3* ptr = (glm::vec3*) glMapBuffer(GL_ARRAY_BUFFER,
				GL_WRITE_ONLY);
		outPtr = ptr;

	}
	else if (_attrIndex == TEXCOORD)
	{
		// get a pointer to the buffer
		glm::vec2* ptr = (glm::vec2*) glMapBuffer(GL_ARRAY_BUFFER,
				GL_WRITE_ONLY);
		outPtr = ptr;

	}
	else if (_attrIndex == COLOR || _attrIndex == TEXCORMOD
			|| _attrIndex == AUX0 || _attrIndex == AUX1 || _attrIndex == AUX2
			|| _attrIndex == AUX3)
	{
		// get a pointer to the buffer
		glm::vec4* ptr = (glm::vec4*) glMapBuffer(GL_ARRAY_BUFFER,
				GL_WRITE_ONLY);
		outPtr = ptr;
	}

	if (!outPtr)
	{
		printf(
				"VAO::getMapBuffer Error couldn´t get a pointer. Attribute Index:%d \n",
				_attrIndex);
	}
	return outPtr;
}

//--------------------------------------------------------------------

// muss nach jedem getMapBuffer() aufgerufen werden!!!
void VAO::unMapBuffer()
{
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#endif

//--------------------------------------------------------------------

void VAO::remove()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	for (int i = 0; i < static_cast<int>(attributes.size()); i++)
	{
		glDeleteBuffers(1, &buffers[attributes[i]->location]);
		buffers[attributes[i]->location] = 0;
	}

#ifndef STRICT_WEBGL_1
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &VAOId);
#endif
	VAOId = 0;
}

//--------------------------------------------------------------------

int VAO::getNrVertices()
{
	return nrVertices;
}

//--------------------------------------------------------------------

VAO::~VAO()
{
	//statCoords.clear();
	//attributes.clear();
	//indices.clear();
	delete usesStaticCoords;
	delete[] instData;
}
}
