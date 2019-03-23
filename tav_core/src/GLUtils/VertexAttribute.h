// @file   vertex-attribute.h
// @author Nicolas Rougier (Nicolas.Rougier@inria.fr)
// converted to c++ by sven hahne
//
// stride has to be set after initialisation!!!
//

#pragma once

#include <string>
#include <algorithm>
#include "headers/gl_header.h"

#define MAX_VERTEX_ATTRIBUTE 16     //Maximum number of attributes per vertex

namespace tav
{
class VertexAttribute
{
public:

	// ----------------------------------------------------------------------------
	VertexAttribute() :
			name(0), location(0), size(0), type(GL_FLOAT), normalized(GL_FALSE), stride(
					0), pointer(0), instDiv(0), isStatic(false), nrConsecLocs(
					1), shdrName("")
	{
	}

	//----------------------------------------------------------------------------
	~VertexAttribute()
	{
	}

	//----------------------------------------------------------------------------
	void enable()
	{
		glEnableVertexAttribArray(location);
		glVertexAttribPointer(location, size, type, normalized, stride,
				pointer);
#ifndef STRICT_WEBGL_1
		glVertexAttribDivisor(location, instDiv);
#endif
	}

	//----------------------------------------------------------------------------
	void setName(GLchar* _name)
	{
		name = (GLchar *) strdup(_name);
	}

	//----------------------------------------------------------------------------
	void setType(char _ctype)
	{
		switch (_ctype)
		{
		case 'b':
			type = GL_BYTE;
			break;
		case 'B':
			type = GL_UNSIGNED_BYTE;
			break;
		case 's':
			type = GL_SHORT;
			break;
		case 'S':
			type = GL_UNSIGNED_SHORT;
			break;
		case 'i':
			type = GL_INT;
			break;
		case 'I':
			type = GL_UNSIGNED_INT;
			break;
		case 'f':
			type = GL_FLOAT;
			break;
		default:
			type = 0;
			break;
		}
	}

	GLsizeiptr getByteSize()
	{
		return size * sizeof(type) * nrConsecLocs;
	}

	//----------------------------------------------------------------------------
	void printInfo()
	{
		printf(
				"VertexAttribute with name: %s at location %d, size: %d, type: %d, normalized: %d, stride: %d\n",
				name, location, size, type, normalized, stride);
	}

	//----------------------------------------------------------------------------
	GLchar* name;       // atribute name
	GLuint location; // location of the generic vertex attribute to be modified.
	GLint size;       // Number of components per generic vertex attribute.
					  // Must be 1, 2, 3, or 4. The initial value is 4.
	GLenum type;       // data type of each component in the array.
					   // Symbolic constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
					   // GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_FLOAT, or GL_DOUBLE are accepted.
					   // The initial value is GL_FLOAT.
	GLboolean normalized; // whether fixed-point data values should be normalized (GL_TRUE) or
						  // converted directly as fixed-point values (GL_FALSE) when they are accessed.
	GLsizei stride; // byte offset between consecutive generic vertex attributes.
					//  If stride is 0, the generic vertex attributes are understood to be
					// tightly packed in the array. The initial value is 0.
	GLvoid* pointer; // pointer to the first component of the first attribute element in the array.
	GLint instDiv;   // attribdivisor, 0 means per vertex, >1 means per instance
	GLboolean isStatic; // defines if there is one value for all vertices or if there are values by vertice
	GLuint nrConsecLocs; // if the attribute uses more than one location
	std::string shdrName;
};
}
