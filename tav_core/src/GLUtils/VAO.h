//
//  VAO.h
//  Test_Basic_GL4
//
//  Created by Sven Hahne on 04.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//  the data is supposed to be organized e.g. (e.g. vncvncvncvnc for 4 vertices, 4 normals, 4 colors)
//  shader locations are supposed to be
//  layout(location = 0) in glm::vec4 vertex;
//  layout(location = 1) in glm::vec4 normal;
//  layout(location = 2) in glm::vec4 color;
//  layout(location = 3) in glm::vec4 texCoords;
//  layout(location = 4) in glm::vec4 model_matrix;
//  where the later consumes 4 locations (5-8)
//
//	webgl uynterstuetzt standardmaessig keine VAOS, es gibt eine Erweiterung, die ist aber nicht ueberall vorhanden
//  und der debugger stuerzt ab. deshalb kann mittles des Flags STRICT_WEBGL_1 ein Kompatibilitaetsmodus
//  erzwungen werden. Hierbei werden nur VBOs verwendet und ein AttribArray bei jedem draw neu gebunden
//  -> macht aber im Prinzip keine Unterschied, ist nur langsamer

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include "headers/gl_header.h"
#include "headers/tav_types.h"
#include "Meshes/Mesh.h"
#include "VertexAttribute.h"
#include "../string_utils.h"
#ifndef __EMSCRIPTEN__
#include "GLUtils/TFO.h"
#endif

namespace tav
{

class VAO
{
public:
	// takes a std::string for defining the VAO
	// can be vertex, normal, color, texCoord
	// datatypes are b(GL_BYTE) B(GL_UNSIGNED_BYTE), s(GL_SHORT) ,
	// S(GL_UNSIGNED_SHORT), i(GL_INT), I(GL_UNSIGNED_INT),
	// f(GL_FLOAT), d(GL_DOUBLE)

	VAO(const char* _format, GLenum _storeMode, bool _createBuffers = true);
	VAO(const char* _format, GLenum _storeMode,
			std::vector<coordType>* _instAttribs, int _maxNrInstances,
			bool _createBuffers = true);
	void init(const char* _format);
	~VAO();

	void initData(int nrVert, GLfloat* _data = nullptr);
	void upload(coordType _type, GLfloat* _entries, int _nrVertices);
	void setElemIndices(unsigned int _count, GLuint* _indices);

	void resize(unsigned int _newNrVertices);
	void bindBuffer(GLuint buffer, GLenum _type = GL_ARRAY_BUFFER);
	GLint addBuffer(coordType _type);
	void addExtBuffer(coordType _type, GLuint _buffer);

	void bind();
	void unbind();

	void bindElementBuffer();
	void unbindElementBuffer();

	void draw(GLenum _mode = GL_TRIANGLES);
	void drawElements(GLenum _mode);
#ifndef __EMSCRIPTEN__
	void draw(GLenum _mode, TFO* _tfo, GLenum _recMode);
	void draw(GLenum _mode, unsigned int offset, unsigned int count, TFO* _tfo);
	void draw(GLenum _mode, unsigned int offset, unsigned int count, TFO* _tfo,
			GLenum _recMode);
	void drawInstanced(GLenum _mode, int nrInstances, TFO* _tfo = nullptr,
			float nrVert = 1.f);
	void drawElements(GLenum _mode, TFO* _tfo, GLenum _recMode);
	void drawElementsInst(GLenum _mode, int nrInstances, TFO* _tfo,
			GLenum _recMode);
	void drawElements(GLenum _mode, TFO* _tfo, GLenum _recMode,
			int _nrElements);
	void* getMapBuffer(coordType _attrIndex);
	void unMapBuffer();
#else
	GLint addBufferFloat(unsigned int location, unsigned int size, const char* name);
	void uploadFloat(unsigned int location, unsigned int size, GLfloat* _entries, int _nrVertices);

	void draw(GLenum _mode, unsigned int offset, unsigned int count);
	void draw(GLenum _mode, unsigned int offset, unsigned int count,GLenum _recMode);
	void drawInstanced(GLenum _mode, int nrInstances, float nrVert = 1.f);
	void drawElements(GLenum _mode, GLenum _recMode);
	void drawElements(GLenum _mode, GLenum _recMode, int _nrElements);

#endif
	void uploadMesh(Mesh* mesh);

	void setStaticNormal(float _x, float _y, float _z);
	void setStaticNormal(std::vector<GLfloat>* _norm);
	void setStaticColor(float _r, float _g, float _b, float _a);
	void setStaticColor(std::vector<GLfloat>* _col);
	void setStatic(float _r, float _g, float _b, float _a, char* _name,
			char* _format);

	GLuint getVAOId();
	GLuint getVBO(coordType _attrIndex);

	void remove();

	int getNrVertices();

private:
	std::vector<GLuint> buffers;
	//GLuint                        tfoBuf;
	GLfloat** instData;
	GLuint elementBuffer;
	GLuint VAOId;
	GLenum storeMode;
	GLenum elementBufferType = GL_UNSIGNED_INT;

	bool createBuffers;

	unsigned int nrElements;
	int nrVertices;
	int drawNrVertices;
	int nrCoordsPerVert;
	int maxNrInstances;

	std::vector<VertexAttribute*> attributes;   //  Array of attributes.
	std::vector<coordType>* instAttribs;
	bool* usesStaticCoords;
	std::vector<GLushort> indices;
	std::vector<std::vector<GLfloat>*> statCoords;
};
}
