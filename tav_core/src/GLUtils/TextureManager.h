//
// Generic Texture Manager
// adapted and extended from Ben Englishs Singleton Texture Manager class
// For use with OpenGL 3.2+ and the FreeImage library
// assumes the sampler unit to be called "tex"
//

#ifndef _tav_Texture_Manager_
#define _tav_Texture_Manager_

#pragma GCC visibility push(default)
#pragma once

#include <map>
#include <iostream>
#include <string>
#ifndef __EMSCRIPTEN__
#include "FreeImage.h"
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <emscripten/emscripten.h>
//#include <SDL2/SDL.h>	// sdl2 is larger and not integrated into emscripten -> worser for performance
//#include <SDL2/SDL_image.h>
#endif
#include "headers/gl_header.h"

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

namespace tav
{

// Enough mips for 16K x 16K, which is the minimum required for
// OpenGL 4.x and higher
#define MAX_TEXTURE_MIPS 14

// This is the main image data structure. It contains all
// the parameters needed to place texture data into a texture
// object using OpenGL.
class TextureData
{
public:
	TextureData()
	{
		textureID = 0;
		target = GL_TEXTURE_2D;
		internalFormat = GL_RGB;
		format = GL_RGB;
		type = GL_RGB;
		pixelType = GL_UNSIGNED_BYTE;
		mipLevels = 0;
		slices = 0;
		sliceStride = 0;
		totalDataSize = 0;
		nrChan = 3;

		tex_t = 0;
		tex_u = 0;
		tex_w = 0;
		tex_h = 0;
		width = 0;
		height = 0;

		bFlipTexture = false;
		bAllocated = false;
		bUseExternalTextureID = false;
	}

	// Each texture image data structure contains an array of
	// MAX_TEXTURE_MIPS of these mipmap structures. The structure
	// represents the mipmap data for all slices at that level.
	struct ImageMipData
	{
		GLsizei width;          // Width of this mipmap level
		GLsizei height;         // Height of this mipmap level
		GLsizei depth;          // Depth pof mipmap level
		GLsizeiptr mipStride;   // Distance between mip levels in memory
		GLvoid* data;           // Pointer to data
	};

	unsigned int textureID = 0;
	GLenum target;                        // Texture target (2D, cube map, etc.)
	GLenum internalFormat;                  // Recommended internal gpu format.
	GLenum format;                          // Format in cpu memory
	GLenum type;                            // Type in cpu memory (GL_RGB, etc.)
#ifndef __EMSCRIPTEN__
	BYTE** faceData;               // separate space for the slices of a cubemap
#else
	unsigned char** faceData;      // separate space for the slices of a cubemap
#endif
	GLenum swizzle[4];                      // Swizzle for RGBA
	GLenum pixelType;      // type, e.g., GL_UNSIGNED_BYTE. should be named type
	GLsizei mipLevels;                      // Number of present mipmap levels
	GLsizei slices;                         // Number of slices (for arrays)
	GLsizeiptr sliceStride;       // Distance between slices of an array texture
	GLsizeiptr totalDataSize;               // Total data allocated for texture
	ImageMipData mip[MAX_TEXTURE_MIPS];     // Actual mipmap data
	unsigned char* bits;

	float tex_t;
	float tex_u;
	unsigned int tex_w;
	unsigned int tex_h;
	unsigned int width;
	unsigned int height;
	unsigned int nrChan;

	bool bFlipTexture;
	bool bAllocated;
	bool bUseExternalTextureID; //if you need to assign ofTexture's id to an externally texture. 
};

class TextureManager
{
public:
	TextureManager();
	static TextureManager* Inst();
	~TextureManager();

	GLuint loadTexture2D(std::string _filename, int nrMipMaps = 8);
	GLuint loadTexture2D(const char* _filename, int nrMipMaps = 8);	//border size
	GLuint loadTextureRect(std::string _filename);	//border size
	GLuint loadTextureRect(const char* _filename);	//border size
	GLuint loadTextureCube(const char* _filename, int nrMipMaps = 8);//border size
	GLuint loadTextureCube(std::string _filename, int nrMipMaps = 8);//border size
	GLuint loadFromFile(std::string _filename, GLenum _textTarget,
			int nrMipMaps);
#ifndef __EMSCRIPTEN__
	void saveTexToFile2D(const char* _filename, FREE_IMAGE_FORMAT _filetype,
			int w, int h, GLenum _internalFormat, GLint _texNr);
	FIBITMAP* ImageLoader(const char* path, int flag);
	GLuint allocate(int w, GLenum internalGlDataType, GLenum extGlDataType,
			GLenum pixelType = GL_UNSIGNED_BYTE);
#endif
	GLuint allocate(int w, int h, GLenum internalGlDataType,
			GLenum extGlDataType, GLenum textTarget, GLenum pixelType =
					GL_UNSIGNED_BYTE);
	void defineImmutableStore();
	void generateSampler();

	void setFiltering(int a_tfMagnification, int a_tfMinification);
	void setWraping(GLenum _wrap);

	void bind();
	void bind(GLuint _texUnit);
	void bind(GLuint _sampUnit, GLuint _samplerID, GLuint _texUnit);
	void unbind();
	void releaseTexture();

	unsigned int getId();
	unsigned int getHeight();
	unsigned int getWidth();
	float getHeightF();
	float getWidthF();
	unsigned int getNrChans();
	unsigned char* getBits();
	void getGlFormatAndType(GLenum glInternalFormat, GLenum& glFormat,
			GLenum& type);
	int getMinificationFilter();
	int getMagnificationFilter();
	GLfloat* getCoordFromPercent(float xPct, float yPct);
	std::string* getFileName();

	bool isAllocated();

#ifdef __EMSCRIPTEN__
	Uint32 get_pixel32( SDL_Surface *surface, int x, int y );
	void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel );
	SDL_Surface* flip_surface( SDL_Surface *surface, int flags);
#endif

protected:
	enum ETextureFiltering
	{
		TEXTURE_FILTER_MAG_NEAREST = 0, // Nearest criterion for magnification
		TEXTURE_FILTER_MAG_BILINEAR, // Bilinear criterion for magnification
		TEXTURE_FILTER_MIN_NEAREST, // Nearest criterion for minification
		TEXTURE_FILTER_MIN_BILINEAR, // Bilinear criterion for minification
		TEXTURE_FILTER_MIN_NEAREST_MIPMAP, // Nearest criterion for minification, but on closest mipmap
		TEXTURE_FILTER_MIN_BILINEAR_MIPMAP, // Bilinear criterion for minification, but on closest mipmap
		TEXTURE_FILTER_MIN_TRILINEAR, // Bilinear criterion for minification on two closest mipmaps, then averaged
	};

#ifdef __EMSCRIPTEN__
	const int FLIP_VERTICAL = 1;
	const int FLIP_HORIZONTAL = 2;
#endif

	static TextureManager* m_inst;

	GLuint texture;
	std::string texPath;
	TextureData texData;

	GLuint samplerID;
	GLint samplerUnit;

	bool generateMips;
	bool bMipMapsGenerated;
	int tfMinification;
	int tfMagnification;

	std::string filename;
};
}

#pragma GCC visibility pop
#endif
