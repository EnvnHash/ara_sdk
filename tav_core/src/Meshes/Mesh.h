//
//  Mesh.h
//  tav_gl4
//
//  Created by Sven Hahne on 06.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#ifndef __tav_gl4__Mesh__
#define __tav_gl4__Mesh__

#include <iostream>
#include <vector>
#include <string>

#include "headers/gl_header.h"
#include "headers/tav_types.h"
#include "GLUtils/glm_utils.h"

namespace tav
{

class Mesh
{

public:
	Mesh();
	Mesh(const char* _format);
	void init(const char* _format);
	~Mesh();

	void scale(float _scaleX, float _scaleY, float _scaleZ);
	void rotate(float _angle, float _rotX, float _rotY, float _rotZ);
	void translate(float _x, float _y, float _z);
	void doTransform(bool transfNormals);
	void invertNormals();

	void calcNormals();
	void calcSmoothNormals();
	void genTexCoord(tav::texCordGenType _type);

	void push_back(GLfloat* _coords, int count);
	void push_back_any(tav::coordType t, GLfloat* _vertices, int count);
	void push_back_indices(GLuint* _indices, int count);
	void push_back_indices(GLushort* _indices, int count);
	void push_back_positions(GLfloat* _vertices, int count);
	void push_back_normals(GLfloat* _colors, int count);
	void push_back_texCoords(GLfloat* _colors, int count);
	void push_back_colors(GLfloat* _colors, int count);

	void clear_indices();
	void clear_positions();
	void clear_normals();
	void clear_texCoords();
	void clear_colors();

	void remove();

	std::vector<GLfloat>* getPositions();
	std::vector<GLfloat>* getNormals();
	std::vector<GLfloat>* getTexCoords();
	std::vector<GLfloat>* getColors();

	int getNrPositions();
	int getNrIndices();
	int getNrVertIntrl();
	int getNrAttributes();

	int getByteSize(coordType t);
	int getByteSize(int t);
	int getTotalByteSize();
	int getByteSizeVert();
	int getByteSizeInd();

	int getSize(coordType t);
	GLenum getType(coordType t);

	void* getPtr(int t);
	void* getPtr(coordType t);
	void* getPtrInterleaved();

	GLfloat* getPositionPtr();
	GLfloat* getNormalPtr();
	GLfloat* getTexCoordPtr();
	GLushort* getIndicePtr();
	GLuint* getIndiceUintPtr();

	GLuint getIndexUint(int ind);
	GLushort getIndex(int ind);
	glm::vec3 getVec3(coordType t, int ind);

	void resize(coordType t, unsigned int size);

	std::vector<GLfloat>* getStaticColor();
	std::vector<GLfloat>* getStaticNormal();

	void setStaticColor(float _r, float _g, float _b, float _a);
	void setStaticNormal(float _x, float _y, float _z);

	bool usesStaticColor();
	bool usesStaticNormal();

	void dumpInterleaved();
	bool usesIntrl();
	bool usesIndices();
	bool usesUintIndices();
	bool hasTexCoords();

	void setVec3(coordType t, unsigned int ind, glm::vec3 _vec);
	void setName(std::string _name);
	void setMaterialId(int _id);

private:
	std::string name;
	std::string* entr_types;           // Format of the vertex buffer.
	bool* bUsing;
	bool useIntrl = false;
	bool useIndices = false;
	bool useIndicesUint = false;
	std::vector<coordType> usedCoordTypes;
	int* coordTypeSize;

	std::vector<GLfloat> statColor;
	std::vector<GLfloat> statNormal;

	std::vector<GLushort> indices;
	std::vector<GLuint> indicesUint;

	std::vector<GLfloat> positions;
	std::vector<GLfloat> normals;
	std::vector<GLfloat> texCoords;
	std::vector<GLfloat> colors;
	std::vector<std::vector<GLfloat>*> allCoords;

	std::vector<GLfloat> interleaved;

	glm::mat4 transfMatr;
	glm::mat4 normTransfMatr;

	int materialId;
};

}

#endif /* defined(__tav_gl4__Mesh__) */
