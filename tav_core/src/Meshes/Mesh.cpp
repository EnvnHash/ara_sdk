//
//  Mesh.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 06.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  assumes vertex to be in the format x, y, z
//  color: r g b a
//  normal: x y z
//  texCoord: x, y

#include "pch.h"
#include "Meshes/Mesh.h"

namespace tav
{

Mesh::Mesh()
{
	const char* _format = "position:3f,normal:3f,texCoord:2f,color:4f";
	init(_format);
}

//---------------------------------------------------------------

Mesh::Mesh(const char* _format)
{
	init(_format);
}

//---------------------------------------------------------------

void Mesh::init(const char* _format)
{
	entr_types = new std::string[COORDTYPE_COUNT];
	entr_types[POSITION] = "position";
	entr_types[NORMAL] = "normal";
	entr_types[TEXCOORD] = "texCoord";
	entr_types[COLOR] = "color";

	bUsing = new bool[COORDTYPE_COUNT];
	for (int i = 0; i < COORDTYPE_COUNT; i++)
		bUsing[i] = false;

	allCoords.push_back(&positions);
	allCoords.push_back(&normals);
	allCoords.push_back(&texCoords);
	allCoords.push_back(&colors);

	transfMatr = glm::mat4(1.0f);
	normTransfMatr = glm::mat4(1.0f);

	// init coord sizes
	coordTypeSize = new int[COORDTYPE_COUNT];
	coordTypeSize[POSITION] = 3;
	coordTypeSize[NORMAL] = 3;
	coordTypeSize[TEXCOORD] = 2;
	coordTypeSize[COLOR] = 4;

	// parse format desc
	int _size = 0;
	const char *start = 0, *end = 0;
	char* format = strdup(_format);

	start = format;
	do
	{
		char *desc = 0;
		// separate std::string by the char ','
		end = (char *) (strchr(start + 1, ','));

		// get complete type description
		if (end == NULL)
		{
			desc = strdup(start);
		}
		else
		{
			desc = strndup(start, end - start);
		}

		// get type name
		char* p = strchr(desc, ':');
		const char* _name = strndup(desc, p - desc);

		if (*(++p) == '\0')
		{
			fprintf( stderr, "No size specified for '%s' attribute\n", _name);
		}
		_size = *p - '0';

		if (std::strcmp(_name, "position") == 0)
		{
			coordTypeSize[POSITION] = _size;
			// printf("vert size: %d \n", _size);
		}
		else if (std::strcmp(_name, "normal") == 0)
		{
			coordTypeSize[NORMAL] = _size;
		}
		else if (std::strcmp(_name, "texCoord") == 0)
		{
			coordTypeSize[TEXCOORD] = _size;
		}
		else if (std::strcmp(_name, "color") == 0)
		{
			coordTypeSize[COLOR] = _size;
		}

		start = end + 1;
		free(desc);
	} while (end);
}

//---------------------------------------------------------------

Mesh::~Mesh()
{
	delete[] bUsing;
	delete[] entr_types;
	delete[] coordTypeSize;
}

//---------------------------------------------------------------

void Mesh::scale(float _scaleX, float _scaleY, float _scaleZ)
{
	transfMatr = glm::scale(glm::vec3(_scaleX, _scaleY, _scaleZ));
	doTransform(false);
}

//---------------------------------------------------------------

void Mesh::rotate(float _angle, float _rotX, float _rotY, float _rotZ)
{
	transfMatr = glm::rotate(_angle, glm::vec3(_rotX, _rotY, _rotZ));
	// normTransfMatr = glm::rotate(_angle, glm::vec3(_rotX, _rotY, _rotZ));
	doTransform(true);
}

//---------------------------------------------------------------

void Mesh::translate(float _x, float _y, float _z)
{
	transfMatr = glm::translate(glm::vec3(_x, _y, _z));
	doTransform(false);
}

//---------------------------------------------------------------

void Mesh::doTransform(bool transfNormals)
{
	if (!useIntrl)
	{
		for (int i = 0;
				i < static_cast<int>(positions.size()) / coordTypeSize[POSITION];
				i++)
		{
			glm::vec4 res = transfMatr
					* glm::vec4(positions[i * 3], positions[i * 3 + 1],
							positions[i * 3 + 2], 1.0f);
			positions[i * 3] = res.r;
			positions[i * 3 + 1] = res.g;
			positions[i * 3 + 2] = res.b;

			if (static_cast<int>(normals.size()) != 0 && transfNormals)
			{
				glm::vec3 nor = glm::mat3(transfMatr)
						* glm::vec3(normals[i * 3], normals[i * 3 + 1],
								normals[i * 3 + 2]);
				normals[i * 3] = nor.r;
				normals[i * 3 + 1] = nor.g;
				normals[i * 3 + 2] = nor.b;
			}
		}
	}
	else
	{
		std::cout << "tav::Mesh Error: can´t transform interleaved mesh!"
				<< std::endl;
	}
}

//---------------------------------------------------------------

void Mesh::invertNormals()
{
	if (static_cast<int>(normals.size()) != 0)
	{
		for (int i = 0;
				i < static_cast<int>(positions.size()) / coordTypeSize[POSITION];
				i++)
		{
			normals[i * 3] *= -1.0f;
			normals[i * 3 + 1] *= -1.0f;
			normals[i * 3 + 2] = -1.0f;
		}
	}
}

//---------------------------------------------------------------------------------------------------

// calculate normals assuming triangles
// should happen before transforming
void Mesh::calcNormals()
{
	int m = coordTypeSize[POSITION] * 3;
	int offs = coordTypeSize[POSITION];

	if (!useIntrl)
	{
		// takes two following positions, calulates the cross product
		// and set the result for all positions of the specific triangle
		for (unsigned int i = 0; i < positions.size() / m; i++)
		{
			int ind1 = i * m + offs;
			int ind2 = i * m + 2 * offs;
			glm::vec3 side1 = glm::vec3(positions[ind1] - positions[i * m],
					positions[ind1 + 1] - positions[i * m + 1],
					positions[ind1 + 2] - positions[i * m + 2]);

			glm::vec3 side2 = glm::vec3(positions[ind2] - positions[i * m],
					positions[ind2 + 1] - positions[i * m + 1],
					positions[ind2 + 2] - positions[i * m + 2]);

			glm::vec3 normal = glm::cross(side2, side1);
			normal = glm::normalize(normal);

			// run through triangle
			for (int j = 0; j < 3; j++)
			{
				if (static_cast<unsigned int>(normals.size())
						< (((i * 3) + j + 1) * coordTypeSize[NORMAL]))
				{
					normals.push_back(normal.x);
					normals.push_back(normal.y);
					normals.push_back(normal.z);
				}
				else
				{
					normals[(i * coordTypeSize[NORMAL] * 3)
							+ (coordTypeSize[NORMAL] * j)] = normal.x;
					normals[(i * coordTypeSize[NORMAL] * 3)
							+ (coordTypeSize[NORMAL] * j) + 1] = normal.y;
					normals[(i * coordTypeSize[NORMAL] * 3)
							+ (coordTypeSize[NORMAL] * j) + 2] = normal.z;
				}
			}
		}
	}
	if (!bUsing[NORMAL])
	{
		bUsing[NORMAL] = true;
		usedCoordTypes.push_back(NORMAL);
	}
}

// assuming triangles
// take two positions, checks where they connect,
// calculates two normals, corresponding to the two triangles
// takes the medium of the two
// NOT TESTED!!!
void Mesh::calcSmoothNormals()
{
	/*
	 if ( !useIntrl )
	 {
	 // run through all triangles
	 for (int i=1;i<(positions.size()/9)+1;i++)
	 {
	 // take each point
	 for (int j=0;j<3;j++)
	 {
	 // look for a connecting point in the rest of the triangles
	 bool found = false; int triCOORDTYPE_COUNT = 0; int vertCOORDTYPE_COUNT = 0;
	 int actInd = ((i+1)*9 +j) % positions.size();
	 int actInd2;
	 
	 while (!found && triCOORDTYPE_COUNT < (int)(positions.size()/9 - 1))
	 {
	 // check each one of the three positions in the triangle
	 for (int k=0;k<3;k++)
	 {
	 int ind = ( ((i+1+triCOORDTYPE_COUNT)*9) +(k*3) ) % positions.size();
	 
	 if (positions[actInd] == positions[ind]
	 && positions[actInd+1] == positions[ind+1]
	 && positions[actInd+2] == positions[ind+2]
	 )
	 {
	 vertCOORDTYPE_COUNT = k;
	 found = true;
	 }
	 }
	 }
	 
	 glm::vec3 normal;
	 
	 // if there was a connecting point
	 if (found)
	 {
	 actInd2 = ( (i+1) *9 + ((j+1) %3) ) % positions.size();
	 // calc two normals corresponding to the two triangles
	 normal = glm::cross(glm::vec3(positions[actInd*9], positions[actInd*9+1], positions[actInd*9+2]),
	 glm::vec3(positions[actInd2*9], positions[actInd2*9+1], positions[actInd2*9+2]));
	 actInd = ((triCOORDTYPE_COUNT *9) + vertCOORDTYPE_COUNT) % positions.size();
	 actInd2 = ( (triCOORDTYPE_COUNT *9) + (vertCOORDTYPE_COUNT+1 % 3) ) % positions.size();
	 glm::vec3 normal2 = glm::cross(glm::vec3(positions[actInd*9], positions[actInd*9+1], positions[actInd*9+2]),
	 glm::vec3(positions[actInd2*9], positions[actInd2*9+1], positions[actInd2*9+2]));
	 normal = glm::vec3((normal.x + normal2.x) * 0.5, (normal.y + normal2.y) * 0.5,
	 (normal.z + normal2.z) * 0.5);
	 normal = glm::normalize(normal);
	 
	 } else
	 {
	 actInd = ((i+1)*9 +j) % positions.size();
	 actInd2 = ((i+1)*9 + ((j+1) %3)) % positions.size();

	 // if there was no connecting point, take the normal relative to the actual triangle
	 normal = glm::cross(glm::vec3(positions[actInd*9], positions[actInd*9+1], positions[actInd*9+2]),
	 glm::vec3(positions[actInd2*9], positions[actInd2*9+1], positions[actInd2*9+2]));
	 normal = glm::normalize(normal);
	 }
	 
	 // add or replace them into the normals array
	 actInd = ((i+1)*9 +j) % positions.size();
	 for (int k=0;k<3;k++)
	 {
	 if ( normals.size() < actInd )
	 {
	 normals.push_back(normal.x);
	 normals.push_back(normal.y);
	 normals.push_back(normal.z);
	 } else {
	 normals[actInd] = normal.x;
	 normals[actInd+1] = normal.y;
	 normals[actInd+2] = normal.z;
	 }
	 }
	 }
	 }
	 }
	 if (!bUsing[NORMAL])
	 {
	 bUsing[NORMAL] = true;
	 usedCoordTypes.push_back(NORMAL);
	 }
	 */
}

// should happen before transforming
void Mesh::genTexCoord(texCordGenType _type)
{
	// loop through the positions
	switch (_type)
	{
	case PLANE_PROJECTION:

		// get the normal of the first triangle
		glm::vec3 normal = glm::cross(
				glm::vec3(positions[0], positions[1], positions[2]),
				glm::vec3(positions[3], positions[4], positions[5]));
//                std::cout << "bfore:  " <<glm::to_string( glm::vec3(positions[0], positions[1], positions[2]) ) << std::endl;
//                std::cout << "bfore:  " <<glm::to_string( glm::vec3(positions[3], positions[4], positions[5]) ) << std::endl;
//                std::cout << "bfore:  " << glm::to_string(normal) << std::endl;
		normal = glm::normalize(normal);
//                std::cout << glm::to_string(normal) << std::endl;

		// normal of x,y plane
		glm::vec3 dstNormal = glm::vec3(0.0f, 0.0f, 1.0f);

		// get a rotation matrix to rotate the plane into the x,y plane
		glm::quat rotQuat = RotationBetweenVectors(normal, dstNormal);

		// convert it to a rotation matrix
		glm::mat4 rotMat = glm::mat4_cast(rotQuat);

		std::vector<glm::vec4> rotVert;

		// get the borders of the transformed plane
		// and save the transformed coordinates
		GLfloat minX = 10000.0f, maxX = -10000.0f, minY = 10000.0f, maxY =
				-10000.0f;
		for (unsigned int i = 0;
				i
						< static_cast<unsigned int>(positions.size())
								/ coordTypeSize[POSITION]; i++)
		{
			glm::vec4 inV = glm::vec4(positions[i * coordTypeSize[POSITION]],
					positions[i * coordTypeSize[POSITION] + 1],
					positions[i * coordTypeSize[POSITION] + 2], 1.0);
			glm::vec4 transV = rotMat * inV;

			rotVert.push_back(transV);
			if (transV.x > maxX)
				maxX = transV.x;
			if (transV.y > maxY)
				maxY = transV.y;
			if (transV.x < minX)
				minX = transV.x;
			if (transV.y < minY)
				minY = transV.y;
		}

		// set texCoords
		for (unsigned int i = 0;
				i
						< static_cast<unsigned int>(positions.size())
								/ coordTypeSize[POSITION]; i++)
		{
			GLfloat tex[] =
			{ (rotVert[i].x - minX) / (maxX - minX), (rotVert[i].y - minY)
					/ (maxY - minY) };

//                    printf("set texture coordinates \n");

			if (static_cast<unsigned int>(texCoords.size())
					< ((i + 1) * coordTypeSize[TEXCOORD]))
			{
				push_back_any(TEXCOORD, tex, coordTypeSize[TEXCOORD]);
				// printf("push back: %f %f \n", tex[0], tex[1]);
			}
			else
			{
				for (int j = 0; j < coordTypeSize[TEXCOORD]; j++)
					texCoords[(i * coordTypeSize[TEXCOORD]) + j] = tex[j];
			}
		}

		// get the convex hull
		//            std::vector<vector<cv::Point> > hull( std::vectors.size() );

		break;
	}
}

//===============================================================================================

void Mesh::push_back(GLfloat* _coords, int COORDTYPE_COUNT)
{
	if ((int) positions.size() == 0 && (int) normals.size() == 0
			&& (int) colors.size() == 0 && (int) texCoords.size() == 0
			&& ((int) indices.size() == 0 || (int) indicesUint.size() == 0))
	{
		for (int i = 0; i < COORDTYPE_COUNT; i++)
			interleaved.push_back(_coords[i]);
		if (!useIntrl)
			useIntrl = true;
	}
	else
	{
		printf(
				"tav::Mesh::push_back_positions Error: mixing with interleave mode, please reset the mesh before changing modes \n");
	}
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_any(coordType t, GLfloat* _positions, int COORDTYPE_COUNT)
{
	if (!useIntrl)
	{
		for (int i = 0; i < COORDTYPE_COUNT; i++)
			allCoords[t]->push_back(_positions[i]);

		if (!bUsing[t])
		{
			bUsing[t] = true;
			usedCoordTypes.push_back(t);
		}
	}
	else
	{
		printf(
				"tav::Mesh::push_back_positions Error: mixing with interleave mode, please reset the mesh before changing modes \n");
	}
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_indices(GLuint* _indices, int COORDTYPE_COUNT)
{
	for (int i = 0; i < COORDTYPE_COUNT; i++)
		indicesUint.push_back(_indices[i]);
	useIndicesUint = true;
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_indices(GLushort* _indices, int COORDTYPE_COUNT)
{
	for (int i = 0; i < COORDTYPE_COUNT; i++)
		indices.push_back(_indices[i]);
	useIndices = true;
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_positions(GLfloat* _positions, int COORDTYPE_COUNT)
{
	push_back_any(POSITION, _positions, COORDTYPE_COUNT);
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_normals(GLfloat* _normals, int COORDTYPE_COUNT)
{
	push_back_any(NORMAL, _normals, COORDTYPE_COUNT);
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_texCoords(GLfloat* _texCoords, int COORDTYPE_COUNT)
{
	push_back_any(TEXCOORD, _texCoords, COORDTYPE_COUNT);
}

//-----------------------------------------------------------------------------------------------

void Mesh::push_back_colors(GLfloat* _colors, int COORDTYPE_COUNT)
{
	push_back_any(COLOR, _colors, COORDTYPE_COUNT);
}

//-----------------------------------------------------------------------------------------------

void Mesh::clear_indices()
{
	indices.clear();
	indicesUint.clear();
}

//-----------------------------------------------------------------------------------------------

void Mesh::clear_positions()
{
	positions.clear();
}

//-----------------------------------------------------------------------------------------------

void Mesh::clear_normals()
{
	normals.clear();
}

//-----------------------------------------------------------------------------------------------

void Mesh::clear_texCoords()
{
	texCoords.clear();
}

//-----------------------------------------------------------------------------------------------

void Mesh::clear_colors()
{
	colors.clear();
}

//===============================================================================================

void Mesh::remove()
{
	statColor.clear();
	statNormal.clear();
	positions.clear();
	normals.clear();
	texCoords.clear();
	colors.clear();
	allCoords.clear();
	interleaved.clear();
}

//---------------------------------------------------------------

std::vector<GLfloat>* Mesh::getPositions()
{
	return &positions;
}

std::vector<GLfloat>* Mesh::getNormals()
{
	return &normals;
}

std::vector<GLfloat>* Mesh::getTexCoords()
{
	return &texCoords;
}

std::vector<GLfloat>* Mesh::getColors()
{
	return &texCoords;
}

//---------------------------------------------------------------------------------------------------

int Mesh::getNrPositions()
{
	// printf("Mesh::getNrPositions: %d coordType: %d results: %d\n", positions.size(), coordTypeSize[POSITION],  static_cast<int>(positions.size()) / coordTypeSize[POSITION]);
	return static_cast<int>(positions.size()) / coordTypeSize[POSITION];
}

int Mesh::getNrIndices()
{
	int nrInd = 0;
	if (useIndices)
	{
		nrInd = static_cast<int>(indices.size());
	}
	else if (useIndicesUint)
	{
		nrInd = static_cast<int>(indicesUint.size());
	}
	return nrInd;
}

int Mesh::getNrVertIntrl()
{
	return static_cast<int>(interleaved.size());
}

//---------------------------------------------------------------------------------------------------

int Mesh::getByteSizeVert()
{
	return static_cast<int>(positions.size()) * sizeof(GLfloat);
}

int Mesh::getByteSizeInd()
{
	if (useIndices)
	{
		return static_cast<int>(indices.size()) * sizeof(GLushort);
	}
	else if (useIndicesUint)
	{
		return static_cast<int>(indicesUint.size()) * sizeof(GLuint);
	}
	else
	{
		return 0;
	}
}

int Mesh::getByteSize(int t)
{
	return static_cast<int>(allCoords[t]->size() * sizeof(GL_FLOAT));
}

int Mesh::getByteSize(coordType t)
{
	return static_cast<int>(allCoords[t]->size() * sizeof(GL_FLOAT));
}

//---------------------------------------------------------------------------------------------------

int Mesh::getNrAttributes()
{
	return static_cast<int>(usedCoordTypes.size());
}

int Mesh::getSize(coordType t)
{
	return static_cast<int>(allCoords[t]->size());
}

GLenum Mesh::getType(coordType t)
{
	return GL_FLOAT;
}

//---------------------------------------------------------------------------------------------------

void* Mesh::getPtr(int t)
{
	return &allCoords[t]->front();
}

void* Mesh::getPtr(coordType t)
{
	return &allCoords[t]->front();
}

GLfloat* Mesh::getPositionPtr()
{
	return &positions.front();
}

GLfloat* Mesh::getNormalPtr()
{
	return &normals.front();
}

GLfloat* Mesh::getTexCoordPtr()
{
	return &texCoords.front();
}

GLushort* Mesh::getIndicePtr()
{
	return &indices.front();
}

GLuint* Mesh::getIndiceUintPtr()
{
	return &indicesUint.front();
}

//-----------------------------------------------------------------------------------------------

GLuint Mesh::getIndexUint(int ind)
{
	GLuint out = 0;
	if (int(indices.size()) > ind)
		out = indices[ind];
	return out;
}

GLushort Mesh::getIndex(int ind)
{
	if (int(indices.size()) > ind)
		return indices[ind];
	return 0;
}

glm::vec3 Mesh::getVec3(coordType t, int ind)
{
	if (bUsing[t])
	{
		if (!useIntrl
				&& static_cast<unsigned int>(allCoords[t]->size())
						> static_cast<unsigned int>(ind * coordTypeSize[t]))
		{
			return glm::vec3(allCoords[t]->at(coordTypeSize[t] * ind),
					allCoords[t]->at(coordTypeSize[t] * ind + 1),
					allCoords[t]->at(coordTypeSize[t] * ind + 2));

		}
		else
		{
			std::cerr << "tav::Mesh::getVec3 Error: using interleave mode"
					<< std::endl;
		}
	}
	else
	{
		std::cerr << "tav::Mesh::getVec3 Error: coordType not found"
				<< std::endl;
	}

	return glm::vec3(0.f);
}

//---------------------------------------------------------------------------------------------------

void Mesh::resize(coordType t, unsigned int size)
{
	if (!useIntrl)
	{
		if (!bUsing[t])
		{
			bUsing[t] = true;
			usedCoordTypes.push_back(t);
		}

		//unsigned int actSize = allCoords[t]->size() / coordTypeSize[t];
		allCoords[t]->resize(size * coordTypeSize[t], 0.f);

		// if new size bigger than old size init with zeros
//        	if(size > actSize)
//        		std::fill(allCoords[t]->begin() + actSize * coordTypeSize[t], allCoords[t]->end(), 0.f);

	}
	else
		std::cerr << "tav::Mesh::resize Error: using interleave mode"
				<< std::endl;
}

//---------------------------------------------------------------------------------------------------

void* Mesh::getPtrInterleaved()
{
	if (!useIntrl)
	{
		// printf("build interleaved \n");
		interleaved.clear();
		for (int i = 0; i < getNrPositions(); i++)
			for (int j = 0; j < COORDTYPE_COUNT; j++)
				if (allCoords[j]->size() > 0)
					for (int k = 0; k < coordTypeSize[j]; k++)
						interleaved.push_back(
								allCoords[j]->at(i * coordTypeSize[j] + k));
	}
	return &interleaved.front();
}

int Mesh::getTotalByteSize()
{
	int size = 0;

	if (!useIntrl)
	{
		std::vector<coordType>::iterator it;
		for (it = usedCoordTypes.begin(); it != usedCoordTypes.end(); ++it)
			size += allCoords[(*it)]->size() * sizeof(GL_FLOAT);

	}
	else
	{
		size = static_cast<int>(interleaved.size()) * sizeof(GL_FLOAT);
	}

	return size;
}

std::vector<GLfloat>* Mesh::getStaticColor()
{
	return &statColor;
}

std::vector<GLfloat>* Mesh::getStaticNormal()
{
	return &statNormal;
}

//---------------------------------------------------------------

void Mesh::setVec3(coordType t, unsigned int ind, glm::vec3 _vec)
{
	if (bUsing[t])
	{
		if (!useIntrl
				&& static_cast<unsigned int>(allCoords[t]->size()) >= ind * 3)
		{
			allCoords[t]->at(coordTypeSize[t] * ind) = _vec.x;
			allCoords[t]->at(coordTypeSize[t] * ind + 1) = _vec.x;
			allCoords[t]->at(coordTypeSize[t] * ind + 2) = _vec.x;

		}
		else
		{
			std::cerr << "tav::Mesh::getVec3 Error: using interleave mode"
					<< std::endl;
		}
	}
	else
	{
		std::cerr << "tav::Mesh::getVec3 Error: coordType not found"
				<< std::endl;
	}
}

void Mesh::setStaticColor(float _r, float _g, float _b, float _a)
{
	if (static_cast<int>(statColor.size()) == 0)
	{
		statColor.push_back(_r);
		statColor.push_back(_g);
		statColor.push_back(_b);
		statColor.push_back(_a);
	}
	else
	{
		statColor[0] = _r;
		statColor[1] = _g;
		statColor[2] = _b;
		statColor[3] = _a;
	}
}

void Mesh::setStaticNormal(float _x, float _y, float _z)
{
	if (static_cast<int>(statNormal.size()) == 0)
	{
		statNormal.push_back(_x);
		statNormal.push_back(_y);
		statNormal.push_back(_z);
	}
	else
	{
		statNormal[0] = _x;
		statNormal[1] = _y;
		statNormal[2] = _z;
	}
}

//---------------------------------------------------------------

bool Mesh::usesStaticColor()
{
	bool use = false;
	if (static_cast<int>(statColor.size()) != 0)
		use = true;
	return use;
}

bool Mesh::usesStaticNormal()
{
	bool use = false;
	if (static_cast<int>(statNormal.size()) != 0)
		use = true;
	return use;
}

//---------------------------------------------------------------------------------------------------

void Mesh::dumpInterleaved()
{
	int COORDTYPE_COUNT = 0;
	std::vector<GLfloat>::iterator it;
	for (it = interleaved.begin(); it != interleaved.end(); ++it)
	{
		printf("[%d] %f \n", COORDTYPE_COUNT, (*it));
		COORDTYPE_COUNT++;
	}
}

//---------------------------------------------------------------------------------------------------

bool Mesh::usesIntrl()
{
	return useIntrl;
}

//---------------------------------------------------------------------------------------------------

bool Mesh::usesIndices()
{
	bool doesUse = false;
	if (useIndices)
	{
		doesUse = true;
	}
	else if (useIndicesUint)
	{
		doesUse = true;
	}
	return doesUse;
}

//---------------------------------------------------------------------------------------------------

bool Mesh::usesUintIndices()
{
	return useIndicesUint;
}

//---------------------------------------------------------------------------------------------------

bool Mesh::hasTexCoords()
{
	return static_cast<int>(texCoords.size() > 0);
}

//---------------------------------------------------------------------------------------------------

void Mesh::setName(std::string _name)
{
	name = _name;
}

//---------------------------------------------------------------------------------------------------

void Mesh::setMaterialId(int _id)
{
	materialId = _id;
}

}
