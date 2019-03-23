/*
 * ObjImporter.cpp
 *
 *  Created on: 14.06.2016
 *      Copyright by Sven Hahne
 */

#include "pch.h"
#include "ObjImporter.h"

namespace tav
{

ObjImporter::ObjImporter(std::string _filename, bool _triangulate) :
		loaded(false), triangulate(_triangulate)
{
	load_obj(_filename);
}

//-------------------------------------------

#ifndef __EMSCRIPTEN__
void ObjImporter::draw(TFO* _tfo)
{
	if (loaded)
	{
		glEnable(GL_DEPTH_TEST);

		if (!triangulate)
			for (size_t i = 0; i < vaos.size(); i++)
				vaos[i]->drawElements(GL_TRIANGLES, _tfo, GL_TRIANGLES);
		else
			for (size_t i = 0; i < vaos.size(); i++)
				vaos[i]->draw(GL_TRIANGLES, _tfo, GL_TRIANGLES);
	}
}
#else
void ObjImporter::draw()
{
	if(loaded)
	{
		glEnable(GL_DEPTH_TEST);

		if (!triangulate)
		for (size_t i=0; i<vaos.size(); i++)
		vaos[i]->drawElements(GL_TRIANGLES);
		else
		for (size_t i=0; i<vaos.size(); i++)
		vaos[i]->draw(GL_TRIANGLES);
	}
}
#endif

//-------------------------------------------

bool ObjImporter::load_obj(std::string filename, unsigned int flags)
{
	std::string err;

	size_t found = filename.find_last_of("/\\");
	if (found != std::string::npos)
	{
		std::string basepath = filename.substr(0, found);
		basepath += "/";

		bool ret = tinyobj::LoadObj(shapes, materials, err, filename.c_str(),
				basepath.c_str(), flags);

		if (!err.empty())
			std::cerr << err << std::endl;

		if (!ret)
		{
			printf("Failed to load/parse .obj.\n");
			return false;
		}

		createMeshes(shapes, materials, triangulate);

		// PrintInfo(shapes, materials, triangulate);

		return true;
	}
	else
		return false;
}

//-------------------------------------------

void ObjImporter::createMeshes(const std::vector<tinyobj::shape_t>& shapes,
		const std::vector<tinyobj::material_t>& materials, bool triangulate)
{
	for (size_t i = 0; i < shapes.size(); i++)
	{
		std::string format = "";

		if (shapes[i].mesh.positions.size() != 0)
			format += "position:3f";
		if (shapes[i].mesh.normals.size() != 0)
		{
			if (shapes[i].mesh.positions.size() != 0)
				format += ",";
			format += "normal:3f";
		}
		if (shapes[i].mesh.texcoords.size() != 0)
		{
			if (std::strlen(format.c_str()) > 0)
				format += ",";
			format += "texCoord:2f";
		}

		meshes.push_back(new Mesh(format.c_str()));
		vaos.push_back(new VAO(format.c_str(), GL_STATIC_DRAW, nullptr, 1));

		if (!triangulate)
		{
			meshes.back()->push_back_indices(
					(GLuint*) (&shapes[i].mesh.indices[0]),
					static_cast<int>(shapes[i].mesh.indices.size()));
			meshes.back()->push_back_positions(
					(GLfloat*) (&shapes[i].mesh.positions[0]),
					static_cast<int>(shapes[i].mesh.positions.size()));
			meshes.back()->push_back_normals(
					(GLfloat*) (&shapes[i].mesh.normals[0]),
					static_cast<int>(shapes[i].mesh.normals.size()));
			meshes.back()->push_back_texCoords(
					(GLfloat*) (&shapes[i].mesh.texcoords[0]),
					static_cast<int>(shapes[i].mesh.texcoords.size()));
		}
		else
		{
			positions.push_back(
					new GLfloat[static_cast<unsigned int>(shapes[i].mesh.indices.size())
							* 3]);
			normals.push_back(
					new GLfloat[static_cast<unsigned int>(shapes[i].mesh.indices.size())
							* 3]);
			texCoords.push_back(
					new GLfloat[static_cast<unsigned int>(shapes[i].mesh.indices.size())
							* 2]);

			if (static_cast<unsigned int>(shapes[i].mesh.positions.size()) > 0)
				for (unsigned int j = 0;
						j
								< static_cast<unsigned int>(shapes[i].mesh.indices.size());
						j++)
					for (unsigned int k = 0; k < 3; k++)
						positions.back()[j * 3 + k] =
								shapes[i].mesh.positions[shapes[i].mesh.indices[j]
										* 3 + k];

			if (static_cast<unsigned int>(shapes[i].mesh.normals.size()) > 0)
				for (unsigned int j = 0;
						j
								< static_cast<unsigned int>(shapes[i].mesh.indices.size());
						j++)
					for (unsigned int k = 0; k < 3; k++)
						normals.back()[j * 3 + k] =
								shapes[i].mesh.normals[shapes[i].mesh.indices[j]
										* 3 + k];

			if (static_cast<unsigned int>(shapes[i].mesh.texcoords.size()) > 0)
				for (unsigned int j = 0;
						j
								< static_cast<unsigned int>(shapes[i].mesh.indices.size());
						j++)
					for (unsigned int k = 0; k < 2; k++)
						texCoords.back()[j * 2 + k] =
								shapes[i].mesh.texcoords[shapes[i].mesh.indices[j]
										* 2 + k];

			meshes.back()->push_back_positions(
					(GLfloat*) (&positions.back()[0]),
					static_cast<unsigned int>(shapes[i].mesh.indices.size())
							* 3);
			meshes.back()->push_back_normals((GLfloat*) (&normals.back()[0]),
					static_cast<unsigned int>(shapes[i].mesh.indices.size())
							* 3);
			meshes.back()->push_back_texCoords(
					(GLfloat*) (&texCoords.back()[0]),
					static_cast<unsigned int>(shapes[i].mesh.indices.size())
							* 2);
		}

		vaos.back()->uploadMesh(meshes.back());
	}
	loaded = true;
}

//-------------------------------------------

void ObjImporter::PrintInfo(const std::vector<tinyobj::shape_t>& shapes,
		const std::vector<tinyobj::material_t>& materials, bool triangulate)
{
	std::cout << "# of shapes    : " << shapes.size() << std::endl;
	std::cout << "# of materials : " << materials.size() << std::endl;

	for (int i = 0; i < (int) shapes.size(); i++)
	{
		printf("shape[%d].name = %s\n", i, shapes[i].name.c_str());
		printf("Size of shape[%d].indices: %d\n", i,
				(int) shapes[i].mesh.indices.size());

		if (triangulate)
		{
			printf("Size of shape[%d].material_ids: %d\n", i,
					(int) shapes[i].mesh.material_ids.size());
			assert((shapes[i].mesh.indices.size() % 3) == 0);
		}
		else
		{
			for (int f = 0; f < (int) shapes[i].mesh.indices.size(); f++)
			{
				printf("  idx[%d] = %d\n", f, shapes[i].mesh.indices[f]);
			}

			printf("Size of shape[%d].material_ids: %d\n", i,
					(int) shapes[i].mesh.material_ids.size());
			assert(
					shapes[i].mesh.material_ids.size()
							== shapes[i].mesh.num_vertices.size());
		}

		printf("shape[%d].num_faces: %d\n", i,
				(int) shapes[i].mesh.num_vertices.size());
		printf("shape[%d].vertices: %d\n", i,
				(int) shapes[i].mesh.positions.size());
		assert((shapes[i].mesh.positions.size() % 3) == 0);

		printf("shape[%d].num_tags: %d\n", i, (int) shapes[i].mesh.tags.size());
		for (int t = 0; t < (int) shapes[i].mesh.tags.size(); t++)
		{
			printf("  tag[%d] = %s ", t, shapes[i].mesh.tags[t].name.c_str());
			;
			printf(" floats: [");
			for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size();
					++j)
			{
				printf("%f", shapes[i].mesh.tags[t].floatValues[j]);
				if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
				{
					printf(", ");
				}
			}
			printf("]");

			printf(" strings: [");
			for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size();
					++j)
			{
				printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
				if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
				{
					printf(", ");
				}
			}
			printf("]");
			printf("\n");
		}
	}

	for (int i = 0; i < (int) materials.size(); i++)
	{
		printf("material[%d].name = %s\n", i, materials[i].name.c_str());
		printf("  material.Ka = (%f, %f ,%f)\n", materials[i].ambient[0],
				materials[i].ambient[1], materials[i].ambient[2]);
		printf("  material.Kd = (%f, %f ,%f)\n", materials[i].diffuse[0],
				materials[i].diffuse[1], materials[i].diffuse[2]);
		printf("  material.Ks = (%f, %f ,%f)\n", materials[i].specular[0],
				materials[i].specular[1], materials[i].specular[2]);
		printf("  material.Tr = (%f, %f ,%f)\n", materials[i].transmittance[0],
				materials[i].transmittance[1], materials[i].transmittance[2]);
		printf("  material.Ke = (%f, %f ,%f)\n", materials[i].emission[0],
				materials[i].emission[1], materials[i].emission[2]);
		printf("  material.Ns = %f\n", materials[i].shininess);
		printf("  material.Ni = %f\n", materials[i].ior);
		printf("  material.dissolve = %f\n", materials[i].dissolve);
		printf("  material.illum = %d\n", materials[i].illum);
		printf("  material.map_Ka = %s\n",
				materials[i].ambient_texname.c_str());
		printf("  material.map_Kd = %s\n",
				materials[i].diffuse_texname.c_str());
		printf("  material.map_Ks = %s\n",
				materials[i].specular_texname.c_str());
		printf("  material.map_Ns = %s\n",
				materials[i].specular_highlight_texname.c_str());
		printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
		printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
		printf("  material.disp = %s\n",
				materials[i].displacement_texname.c_str());
		std::map<std::string, std::string>::const_iterator it(
				materials[i].unknown_parameter.begin());
		std::map<std::string, std::string>::const_iterator itEnd(
				materials[i].unknown_parameter.end());

		for (; it != itEnd; it++)
		{
			printf("  material.%s = %s\n", it->first.c_str(),
					it->second.c_str());
		}
		printf("\n");
	}
}

//-------------------------------------------

std::vector<VAO*>* ObjImporter::getVaos()
{
	return &vaos;
}

//-------------------------------------------

GLfloat* ObjImporter::getPositions(unsigned int i)
{
	return meshes[i]->getPositionPtr();
}

//-------------------------------------------

GLfloat* ObjImporter::getNormals(unsigned int i)
{
	return meshes[i]->getNormalPtr();
}

//-------------------------------------------

GLfloat* ObjImporter::getTexCoords(unsigned int i)
{
	return meshes[i]->getTexCoordPtr();
}

//-------------------------------------------

ObjImporter::~ObjImporter()
{
	meshes.clear();
	vaos.clear();
}

} /* namespace tav */
