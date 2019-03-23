/*
 * ObjImporter.h
 *
 *  Created on: 14.06.2016
 *      Copyright by Sven Hahne
 */

#ifndef OBJIMPORTER_H_
#define OBJIMPORTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <clocale>
#include <ctime>
#include <cwchar>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_obj_loader.h"

#include "Meshes/Mesh.h"
#include "GLUtils/VAO.h"

#ifdef __EMSCRIPTEN__
#include "Shaders/Shaders.h"
#endif

namespace tav
{

class ObjImporter
{
public:
	ObjImporter(std::string _filename, bool _triangulate = false);
	~ObjImporter();
	bool load_obj(std::string filename, unsigned int flags = 1);
	void createMeshes(const std::vector<tinyobj::shape_t>& shapes,
			const std::vector<tinyobj::material_t>& materials,
			bool triangulate = false);
	void PrintInfo(const std::vector<tinyobj::shape_t>& shapes,
			const std::vector<tinyobj::material_t>& materials,
			bool triangulate = true);
#ifndef __EMSCRIPTEN__
	void draw(TFO* _tfo = 0);
#else
	void draw();
#endif
	std::vector<VAO*>* getVaos();
	GLfloat* getPositions(unsigned int i);
	GLfloat* getNormals(unsigned int i);
	GLfloat* getTexCoords(unsigned int i);

private:
	bool loaded;
	bool triangulate;
	std::vector<VAO*> vaos;
	std::vector<Mesh*> meshes;

	std::vector<GLfloat*> positions;
	std::vector<GLfloat*> normals;
	std::vector<GLfloat*> texCoords;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
};

} /* namespace tav */

#endif /* OBJIMPORTER_H_ */
