//
// Generic Texture Manager
// adapted and extended from Ben Englishs Singleton Texture Manager class
// For use with OpenGL 3.2+ and the FreeImage library
//
//

#include "pch.h"
#include "GLUtils/TextureManager.h"

namespace tav
{

TextureManager* TextureManager::m_inst(0);

TextureManager* TextureManager::Inst()
{
	if (!m_inst)
		m_inst = new TextureManager();

	return m_inst;
}

TextureManager::TextureManager()
{
// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif
}

//------------------------------------------------------------------------------------------------

TextureManager::~TextureManager()
{
	releaseTexture();
// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_DeInitialise();
#endif
}

//-----------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
FIBITMAP* TextureManager::ImageLoader(const char* _path, int flag)
{
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// set path to resources, needed for osx compilation
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) printf("error setting path to resources \n");
	CFRelease(resourcesURL);
	chdir(_path);
#endif
	texPath = std::string(_path);

	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(_path, flag);

	if (fif == FIF_UNKNOWN)
	{
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(_path);
	}
	// check that the plugin has reading capabilities ...
	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))
	{
		// ok, let's load the file
		return FreeImage_Load(fif, _path, flag);
	}
	else
	{
		std::cout << " unknown format " << std::endl;
		return 0;
	}
}
#endif

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadTexture2D(std::string _filename, int nrMipMaps)
{
	filename = _filename;
	return loadFromFile(filename, GL_TEXTURE_2D, nrMipMaps);
}

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadTexture2D(const char* _filename, int nrMipMaps)
{
	filename = std::string(_filename); // copy
	return loadFromFile(filename, GL_TEXTURE_2D, nrMipMaps);
}

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadTextureRect(std::string _filename)
{
	filename = _filename.c_str();
	return loadFromFile(filename, GL_TEXTURE_RECTANGLE, 1);
}

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadTextureRect(const char* _filename)
{
	filename = _filename;
	return loadFromFile(filename, GL_TEXTURE_RECTANGLE, 1);
}

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadTextureCube(const char* _filename, int nrMipMaps)
{
	filename = _filename;
	return loadFromFile(filename, GL_TEXTURE_CUBE_MAP, nrMipMaps);
}

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadTextureCube(std::string _filename, int nrMipMaps)
{
	filename = _filename.c_str();
	return loadFromFile(filename, GL_TEXTURE_CUBE_MAP, nrMipMaps);
}

//------------------------------------------------------------------------------------------------

GLuint TextureManager::loadFromFile(std::string _filename, GLenum _textTarget,
		int nrMipMaps)
{
	//std::cout << "loadFromFile" << std::endl;

#ifndef __EMSCRIPTEN__
	//BYTE*       bits(0);
	//texData.bits(0);
	GLboolean generateMips = true; // maybe makes no sense to turn it off -> slower
	unsigned int width(0), height(0), BPP(0);
	unsigned int mimapLevels;
	unsigned int nrChannels = 1;
	filename = _filename;

	if (_textTarget == GL_TEXTURE_RECTANGLE
			|| _textTarget == GL_TEXTURE_CUBE_MAP)
		generateMips = false;

	if (generateMips)
		mimapLevels = nrMipMaps;
	else
		mimapLevels = 1;

	//retrieve the image data
	FIBITMAP* pBitmap = ImageLoader(filename.c_str(), 0);

	//bits = FreeImage_GetBits(pBitmap);
	texData.bits = (GLubyte*) FreeImage_GetBits(pBitmap);
	width = FreeImage_GetWidth(pBitmap);
	height = FreeImage_GetHeight(pBitmap);
	BPP = FreeImage_GetBPP(pBitmap);
	FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(pBitmap);
	FIBITMAP** faceDataBM = new FIBITMAP*[6];

	getGlError();

	switch (colorType)
	{
	case FIC_MINISBLACK:
		nrChannels = 1;
		texData.nrChan = 1;
		texData.format = GL_RED;
		texData.internalFormat = BPP == 32 ? GL_R32F : BPP == 16 ? GL_R16F :
									BPP == 8 ? GL_R8 : 0;
		texData.type =
				texData.internalFormat == GL_R8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
		break;
	case FIC_MINISWHITE:
		nrChannels = 1;
		texData.nrChan = 1;
		texData.format = GL_RED;
		texData.internalFormat = BPP == 32 ? GL_R32F : BPP == 16 ? GL_R16F :
									BPP == 8 ? GL_R8 : 0;
		texData.type =
				texData.internalFormat == GL_R8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
		break;
	case FIC_PALETTE:
		nrChannels = 3;
		texData.nrChan = 3;
		texData.format = GL_BGR;
		texData.internalFormat = GL_RGB8;
		texData.type = GL_UNSIGNED_BYTE;
		break;
	case FIC_RGB:
		nrChannels = 3;
		texData.nrChan = 3;
		texData.format = GL_BGR;
		texData.internalFormat = BPP == 96 ? GL_RGB32F : BPP == 48 ? GL_RGB16F :
									BPP == 24 ? GL_RGB8 : 0;
		texData.type =
				texData.internalFormat == GL_RGB8 ? GL_UNSIGNED_BYTE : GL_FLOAT;

		// strange effect when exporting tiff from gimp... fi says FIC_RGB, but has 32 bit...
		if (BPP == 32)
		{
			texData.internalFormat = GL_RGBA8;
			texData.format = GL_BGRA;
			nrChannels = 4;
			texData.nrChan = 4;
			texData.type = GL_UNSIGNED_BYTE;
		}
		break;
	case FIC_RGBALPHA:
		nrChannels = 4;
		texData.nrChan = 4;
		texData.format = GL_BGRA;
		texData.internalFormat = BPP == 128 ? GL_RGBA32F :
									BPP == 64 ? GL_RGBA16F :
									BPP == 32 ? GL_RGBA8 : 0;
		texData.type =
				texData.internalFormat == GL_RGBA8 ?
						GL_UNSIGNED_BYTE : GL_FLOAT;
		break;
	case FIC_CMYK:
		nrChannels = 4;
		texData.nrChan = 4;
		texData.format = GL_BGRA;
		texData.internalFormat = BPP == 128 ? GL_RGBA32F :
									BPP == 64 ? GL_RGBA16F :
									BPP == 32 ? GL_RGBA8 : 0;
		texData.type =
				texData.internalFormat == GL_RGBA8 ?
						GL_UNSIGNED_BYTE : GL_FLOAT;
		break;
	default:
		printf(
				"TextureManager::loadFromFile Error: unknown number of channels\n");
	}

	texData.tex_w = width;
	texData.tex_h = height;
	texData.tex_t = width / texData.tex_w;
	texData.tex_u = height / texData.tex_h;
	texData.target = _textTarget;         // asuming 2d pictures
	//texData.internalFormat = BPP == 32 ? GL_RGBA8 : BPP == 24 ? GL_RGB8 : BPP == 8 ? GL_DEPTH_COMPONENT : 0;
	texData.textureID = 0;  // init id

#else
	//unsigned char* bits(0);
	GLenum colorFormat;
	GLenum inColorFormat;
	GLboolean generateMips = false;// maybe makes no sense to turn it off -> slower
	unsigned int width(0), height(0), BPP(0);
	unsigned int mimapLevels;
	filename = _filename;
	SDL_Surface** faceDataBM;

	generateMips = false;
	mimapLevels = 0;

	SDL_Surface *surface;// Gives us the information to make the texture

	//if ( SDL_Init(SDL_INIT_VIDEO) != 0 ) {
	//   printf("Unable to initialize SDL: %s\n", SDL_GetError());
	//}

	int flags = IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF;
	int initted = IMG_Init(flags);

	if ( !(surface = IMG_Load(filename.c_str())) )
	printf("SDL could not load %s: %s\n", filename.c_str(), SDL_GetError());
#ifndef NO_PRELOADED
	int w, h;
	char *data = emscripten_get_preloaded_image_data(filename.c_str(), &w, &h);
	width = w; height = h;
#endif
	SDL_PixelFormat* sdlFormat = surface->format;
	BPP = sdlFormat->BitsPerPixel;
	width = surface->w;
	height = surface->h;

	// flip the image
	SDL_Surface* flipped = flip_surface(surface, FLIP_VERTICAL);
	texData.bits = static_cast<unsigned char*>(flipped->pixels);

	if(sdlFormat->Amask )
	{
		colorFormat = GL_RGB;
		inColorFormat = GL_BGR;
		texData.nrChan = 3;
	}
	else
	{
		colorFormat = GL_RGBA;
		inColorFormat = GL_BGRA;
		texData.nrChan = 4;
	}

	texData.tex_w = width;
	texData.tex_h = height;
	texData.tex_t = width / texData.tex_w;
	texData.tex_u = height / texData.tex_h;
	texData.target = _textTarget;         // asuming 2d pictures
	texData.format = BPP == 32 ? GL_RGBA : BPP == 24 ? GL_BGR : BPP == 8 ? GL_R8 : 0;
	texData.type = GL_UNSIGNED_BYTE;
	// die desktop formate wie GL_RGBA8 funktionieren hier nicht...
	texData.internalFormat = BPP == 32 ? GL_RGBA : BPP == 24 ? GL_RGB : BPP == 8 ? GL_DEPTH_COMPONENT : 0;
	texData.textureID = 0;// init id
#endif
	//if this somehow one of these failed (they shouldn't), return failure
	if ((texData.bits == 0) || (width == 0) || (height == 0))
	{
		std::cerr << "TextureManager Error: could not read image " << std::endl;
		return false;
	}

	// if the texture is a cube map, cut the input file according to a standard
	// cubemap separation
	if (texData.target == GL_TEXTURE_CUBE_MAP)
	{
		int stepX = texData.tex_w / 4;
		int stepY = texData.tex_h / 3;
		int pos[6][4] =
		{
		{ stepX * 2, stepY, stepX * 3, stepY * 2 },   // 0: positive-x
				{ 0, stepY, stepX, stepY * 2 },   // 1: negative-x
				{ stepX, 0, stepX * 2, stepY },   // 3: negative-y
				{ stepX, stepY * 2, stepX * 2, stepY * 3 },   // 2: positive-y
				{ stepX, stepY, stepX * 2, stepY * 2 },   // 4: positive-z
				{ stepX * 3, stepY, stepX * 4, stepY * 2 }    // 5: negative-z
		};

#ifndef __EMSCRIPTEN__
		texData.faceData = new BYTE*[6];

		for (auto face = 0; face < 6; face++)
		{
			faceDataBM[face] = FreeImage_Copy(pBitmap, pos[face][0],
					pos[face][1], pos[face][2], pos[face][3]);

			FreeImage_FlipVertical(faceDataBM[face]);

			if (faceDataBM[face] == NULL)
				printf(
						"TextureManager: cube texture separation failed at nr: %d !\n",
						face);
			texData.faceData[face] = FreeImage_GetBits(faceDataBM[face]);
		}
#else
		texData.faceData = new unsigned char*[6];
		faceDataBM = new SDL_Surface*[6];
		Uint32 rmask, gmask, bmask, amask;

		// SDL interprets each pixel as a 32-bit number, so our masks must depend
		// on the endianness (byte order) of the machine
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
#endif
		SDL_Rect dstRect;
		SDL_Rect srcRect;
		dstRect.x = 0; dstRect.y = 0; dstRect.w = stepX; dstRect.h = stepY;

		//printf("stepX: %d stepY: %d \n", stepX, stepY);

		for (auto face=0;face<6;face++)
		{
			srcRect.x = pos[face][0];
			srcRect.y = pos[face][1]; // SDL takes the upper left corner and not the lower left
			srcRect.w = stepX;
			srcRect.h = stepY;

			faceDataBM[face] = SDL_CreateRGBSurface(0, stepX, stepY, BPP, rmask, gmask, bmask, amask);

			SDL_BlitSurface(surface, &srcRect, faceDataBM[face], &dstRect);

			if(faceDataBM[face] == NULL)
			printf("TextureManager: cube texture separation failed at nr: %d !\n", face);

			SDL_Surface* flipped = flip_surface(faceDataBM[face], FLIP_VERTICAL);
			faceDataBM[face] = flipped;

			texData.faceData[face] = static_cast<unsigned char*>(faceDataBM[face]->pixels);
		}
#endif
		texData.tex_w = texData.tex_w / 4;
		texData.tex_h = texData.tex_h / 3;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	//generate an OpenGL texture ID for this texture
	glGenTextures(1, (GLuint *) &texData.textureID); // could be more then one, but for now, just one

	//bind to the new texture ID
	glBindTexture(texData.target, texData.textureID);

	// Specify the data for the texture
	switch (texData.target)
	{
#ifndef __EMSCRIPTEN__
	case GL_TEXTURE_1D:
		glTexStorage1D(texData.target, mimapLevels,       // nr of mipmap levels
				texData.internalFormat, texData.tex_w);
		glTexSubImage1D(GL_TEXTURE_1D,              // target
				0,                          // mipmap level
				0,                          // xoffset
				texData.tex_w,              // width
				texData.format, texData.type, texData.bits);
		break;
#endif
	case GL_TEXTURE_CUBE_MAP:
		// Now that storage is allocated for the texture object,
		// we can place the texture data into its texel array.
		for (GLuint face = 0; face < 6; face++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
					0,                      // mimap Level
					texData.internalFormat,
					texData.tex_w,          // Size of face
					texData.tex_h, 0, texData.format, texData.type,
					texData.faceData[face]);
		}

#ifndef __EMSCRIPTEN__
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
		break;
	default:
#ifndef __EMSCRIPTEN__
		glTexStorage2D(texData.target, mimapLevels,       // nr of mipmap levels
				texData.internalFormat, texData.tex_w, texData.tex_h);
		glTexSubImage2D(texData.target,             // target
				0,                          // mipmap level
				0, 0,                       // x and y offset
				texData.tex_w,              // width and height
				texData.tex_h, texData.format, texData.type, texData.bits);
#else
		glTexImage2D(texData.target,
				mimapLevels,                 // nr of mipmap levels
				texData.internalFormat,
				texData.tex_w,
				texData.tex_h,
				0,
				texData.format,
				texData.type,
				texData.bits);
#endif
		break;
	}

	if (_textTarget == GL_TEXTURE_RECTANGLE
			|| _textTarget == GL_TEXTURE_CUBE_MAP)
	{
		// GL_TEXTURE_RECTANGLE can´t repeat
		glTexParameterf(texData.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(texData.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameterf(texData.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(texData.target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// mipmaps
	if (generateMips)
	{
		glGenerateMipmap(texData.target);
		glTexParameterf(texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(texData.target, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		// set linear filtering
		glTexParameterf(texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	bMipMapsGenerated = generateMips;

	glBindTexture(texData.target, 0);

#ifndef __EMSCRIPTEN__
	//Free FreeImage's copy of the data
	// FreeImage_Unload(pBitmap);

	//        if (texData.target == GL_TEXTURE_CUBE_MAP)
	//            for (auto face=0; face<6; face++)
	//                FreeImage_Unload(faceData[face]);
#else
	SDL_FreeSurface(surface);
	if (texData.target == GL_TEXTURE_CUBE_MAP)
	for (auto face=0; face<6; face++)
	SDL_FreeSurface(faceDataBM[face]);

	//IMG_Quit(); // wird ignoriert...
	//SDL_Quit(); // wird auch ignoriert
#endif
	return texData.textureID;
}

//------------------------------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
GLuint TextureManager::allocate(int w, GLenum internalGlDataType,
		GLenum extGlDataType, GLenum pixelType)
{
	//our graphics card might not support arb so we have to see if it is supported.
	//otherwise we need to calculate the next power of 2 for the requested dimensions
	//ie (320x240) becomes (512x256)
	texData.tex_w = w;
	texData.tex_t = w / texData.tex_w;
	texData.target = GL_TEXTURE_1D;
	texData.internalFormat = internalGlDataType;
	texData.format = extGlDataType;
	texData.pixelType = pixelType;

	// get the type (format) and pixelType (type) corresponding to the typeInteral (internalFormat)
	// getGlFormatAndType(texData.internalFormat, texData.format, texData.pixelType);

	glGenTextures(1, (GLuint *) &texData.textureID); // could be more then one, but for now, just one
	glBindTexture(texData.target, (GLuint) texData.textureID);

	GLsizei n;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &n);
	if (texData.tex_w >= (unsigned int) n)
		printf(
				"tav::TextureManager Error: allocating GL_TEXTURE_1D, requested size exceeds memory limit. Limit: %d Requested Size: %d \n",
				n, texData.tex_w);

	// define immutable storage space. best practise since opengl hereby stops tracking certain features
	// use levels = 1
	glTexStorage1D(texData.target, 1, texData.internalFormat, texData.tex_w);

	int nrChans = texData.internalFormat == GL_RGBA8 ? 4 :
					texData.internalFormat == GL_RGB8 ? 3 :
					texData.internalFormat == GL_RG8 ? 2 :
					texData.internalFormat == GL_R8 ? 1 : 1;

	float* nullImg = new float[texData.tex_w * nrChans];

	// Specify the data for the texture
	glTexSubImage1D(texData.target,             // target
			0,                          // mipmap level
			0,                          // x and y offset
			texData.tex_w,              // width and height
			texData.format, texData.pixelType, nullImg);

	glTexParameterf(texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(texData.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(texData.target, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	texData.width = w;
	texData.bFlipTexture = false;
	texData.bAllocated = true;

	return texData.textureID;
}

//------------------------------------------------------------------------------------------------

void TextureManager::saveTexToFile2D(const char* _filename,
		FREE_IMAGE_FORMAT _filetype, int w, int h, GLenum _internalFormat,
		GLint _texNr)
{
	GLenum format;
	GLenum type;
	FIBITMAP* bitmap = nullptr;

	glBindTexture(GL_TEXTURE_2D, _texNr);
	getGlFormatAndType(_internalFormat, format, type);

	switch (type)
	{
	case GL_UNSIGNED_SHORT:
	{
		switch (format)
		{
		case GL_RED:
			bitmap = FreeImage_AllocateT(FIT_UINT16, w, h);
			break;
		default:
			printf("TextureManager::saveTexToFile2D Error: unknown format \n");
			break;
		}

		if (bitmap)
		{
			WORD* bits = (WORD*) FreeImage_GetBits(bitmap);
			glGetTexImage(GL_TEXTURE_2D, 0, format, type, bits);
		}
		else
		{
			printf(
					"TextureManager::saveTexToFile2D Error: could not allocate bitmap \n");
		}
		break;
	}
	case GL_UNSIGNED_BYTE:
	{
		int nrChan = 3;

		switch (format)
		{
		case GL_RED:
			nrChan = 1;
			break;
		case GL_RG:
			nrChan = 2;
			break;
		case GL_RGB:
			nrChan = 3;
			break;
		case GL_RGBA:
			nrChan = 4;
			break;
		default:
			printf("TextureManager::saveTexToFile2D Error: unknown format \n");
			break;
		}

		bitmap = FreeImage_Allocate(w, h, nrChan * 8);
		if (bitmap)
		{
			BYTE* bits = (BYTE*) FreeImage_GetBits(bitmap);
			glGetTexImage(GL_TEXTURE_2D, 0, format, type, bits);
		}
		else
		{
			printf(
					"TextureManager::saveTexToFile2D Error: could not allocate bitmap \n");
		}
		break;
	}
	case GL_FLOAT:
	{
		switch (_internalFormat)
		{
		case GL_R32F:
			bitmap = FreeImage_AllocateT(FIT_FLOAT, w, h);
			break;
		case GL_RGB16F:
			bitmap = FreeImage_AllocateT(FIT_RGB16, w, h);
			break;
		case GL_RGBA16F:
			bitmap = FreeImage_AllocateT(FIT_RGBA16, w, h);
			break;
		case GL_RGB32F:
			bitmap = FreeImage_AllocateT(FIT_RGBF, w, h);
			break;
		case GL_RGBA32F:
			bitmap = FreeImage_AllocateT(FIT_RGBAF, w, h);
			break;
		default:
			printf("TextureManager::saveTexToFile2D Error: unknown format \n");
			break;
		}

		if (bitmap)
		{
			BYTE* bits = (BYTE*) FreeImage_GetBits(bitmap);
			glGetTexImage(GL_TEXTURE_2D, 0, format, type, bits);
		}
		else
		{
			printf(
					"TextureManager::saveTexToFile2D Error: could not allocate bitmap \n");
		}
		break;
	}
	default:
		printf("TextureManager::saveTexToFile2D Error: Unknown pixel format\n");
		break;
	}

	if (!FreeImage_Save(_filetype, bitmap, _filename))
	{
		printf(
				"TextureManager::saveTexToFile2D Error: FreeImage_Save failed\n");
	}
	else
	{
		FreeImage_Unload(bitmap);
	}
}
#endif

//------------------------------------------------------------------------------------------------

GLuint TextureManager::allocate(int w, int h, GLenum internalGlDataType,
		GLenum extGlDataType, GLenum textTarget, GLenum pixelType)
{
	//our graphics card might not support arb so we have to see if it is supported.
	//otherwise we need to calculate the next power of 2 for the requested dimensions
	//ie (320x240) becomes (512x256)
	texData.tex_w = w;
	texData.tex_h = h;
	texData.tex_t = w / texData.tex_w;
	texData.tex_u = h / texData.tex_h;
	texData.target = textTarget;
	texData.internalFormat = internalGlDataType;
	texData.format = extGlDataType;
	texData.pixelType = pixelType;

	// get the type (format) and pixelType (type) corresponding to the typeInteral (internalFormat)
	// getGlFormatAndType(texData.internalFormat, texData.format, texData.pixelType);

	glGenTextures(1, (GLuint *) &texData.textureID); // could be more then one, but for now, just one
	glBindTexture(texData.target, (GLuint) texData.textureID);

	// define immutable storage space. best practise since opengl hereby stops tracking certain features
	// use levels = 1
#ifndef __EMSCRIPTEN__
	glTexStorage2D(texData.target, 1, texData.internalFormat, texData.tex_w,
			texData.tex_h);
#endif

	int nrChans = texData.internalFormat == GL_RGBA8 ? 4 :
					texData.internalFormat == GL_RGB8 ? 3 :
					texData.internalFormat == GL_RG8 ? 2 :
					texData.internalFormat == GL_R8 ? 1 : 1;

	float* nullImg = new float[texData.tex_w * texData.tex_h * nrChans];

	// Specify the data for the texture
	switch (texData.target)
	{
#ifndef __EMSCRIPTEN__
	case GL_TEXTURE_1D:
		glTexSubImage1D(texData.target,             // target
				0,                          // mipmap level
				0,                          // x and y offset
				texData.tex_w,              // width and height
				texData.format, texData.pixelType, nullImg);
		break;
	case GL_TEXTURE_2D:
		glTexSubImage2D(texData.target,             // target
				0,                          // mipmap level
				0, 0,                       // x and y offset
				texData.tex_w,              // width and height
				texData.tex_h, texData.format, texData.pixelType, nullImg);
		break;
	case GL_TEXTURE_RECTANGLE:
		glTexSubImage2D(texData.target,             // target
				0,                          // mipmap level
				0, 0,                       // x and y offset
				texData.tex_w,              // width and height
				texData.tex_h, texData.format, texData.pixelType, nullImg);
		break;
#else
		case GL_TEXTURE_2D :
		glTexImage2D(texData.target,             // target
				0,// mipmap level
				texData.internalFormat,// x and y offset
				texData.tex_w,// width and height
				texData.tex_h,
				0,
				texData.format,
				texData.pixelType,
				nullImg);
		break;
		case GL_TEXTURE_RECTANGLE :
		glTexImage2D(texData.target,// target
				0,// mipmap level
				texData.internalFormat,// x and y offset
				texData.tex_w,// width and height
				texData.tex_h,
				0,
				texData.format,
				texData.pixelType,
				nullImg);
		break;
#endif
	default:
		break;
	}

	delete [] nullImg;

	glTexParameterf(texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(texData.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(texData.target, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	texData.width = w;
	texData.height = h;
	texData.bFlipTexture = false;
	texData.bAllocated = true;

	return texData.textureID;
}

//------------------------------------------------------------------------------------------------

void TextureManager::generateSampler()
{
	glGenSamplers(1, &samplerID);
	glBindSampler(samplerUnit, samplerID);

}

//------------------------------------------------------------------------------------------------

// dead method not used...
void TextureManager::defineImmutableStore()
{
	int level;
	GLubyte * ptr = (GLubyte *) texData.mip[0].data;

	// define immutable storage space. best practise since opengl hereby stops tracking certain features
	switch (texData.target)
	{
#ifndef __EMSCRIPTEN__
	case GL_TEXTURE_1D:
		glTexStorage1D(texData.target, texData.mipLevels,
				texData.internalFormat, texData.mip[0].width);

		for (level = 0; level < texData.mipLevels; ++level)
		{
			glTexSubImage1D(GL_TEXTURE_1D, level, 0, texData.mip[level].width,
					texData.format, texData.type, texData.mip[level].data);
		}
		break;
#endif
	case GL_TEXTURE_1D_ARRAY:
#ifndef __EMSCRIPTEN__
		glTexStorage2D(texData.target, texData.mipLevels,
				texData.internalFormat, texData.mip[0].width, texData.slices);
#endif
		for (level = 0; level < texData.mipLevels; ++level)
		{
			glTexSubImage2D(GL_TEXTURE_1D, level, 0, 0,
					texData.mip[level].width, texData.slices, texData.format,
					texData.type, texData.mip[level].data);
		}
		break;
	case GL_TEXTURE_2D:
#ifndef __EMSCRIPTEN__
		glTexStorage2D(texData.target, texData.mipLevels,
				texData.internalFormat, texData.mip[0].width,
				texData.mip[0].height);
#endif
		for (level = 0; level < texData.mipLevels; ++level)
		{
			glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0,
					texData.mip[level].width, texData.mip[level].height,
					texData.format, texData.type, texData.mip[level].data);
		}
		break;
	case GL_TEXTURE_CUBE_MAP:
		for (level = 0; level < texData.mipLevels; ++level)
		{
			ptr = (GLubyte *) texData.mip[level].data;
			for (int face = 0; face < 6; face++)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level,
						texData.internalFormat, texData.mip[level].width,
						texData.mip[level].height, 0, texData.format,
						texData.type, ptr + texData.sliceStride * face);
			}
		}
		break;
	case GL_TEXTURE_2D_ARRAY:
#ifndef __EMSCRIPTEN__
		glTexStorage3D(texData.target, texData.mipLevels,
				texData.internalFormat, texData.mip[0].width,
				texData.mip[0].height, texData.slices);
#endif
		for (level = 0; level < texData.mipLevels; ++level)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, 0,
					texData.mip[level].width, texData.mip[level].height,
					texData.slices, texData.format, texData.type,
					texData.mip[level].data);
		}
		break;
	case GL_TEXTURE_CUBE_MAP_ARRAY:
#ifndef __EMSCRIPTEN__
		glTexStorage3D(texData.target, texData.mipLevels,
				texData.internalFormat, texData.mip[0].width,
				texData.mip[0].height, texData.slices);
#endif
		break;
	case GL_TEXTURE_3D:
#ifndef __EMSCRIPTEN__
		glTexStorage3D(texData.target, texData.mipLevels,
				texData.internalFormat, texData.mip[0].width,
				texData.mip[0].height, texData.mip[0].depth);
#endif
		for (level = 0; level < texData.mipLevels; ++level)
		{
			glTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0,
					texData.mip[level].width, texData.mip[level].height,
					texData.mip[level].depth, texData.format, texData.type,
					texData.mip[level].data);
		}
		break;
	default:
		break;
	}
}

//------------------------------------------------------------------------------------------------

void TextureManager::setFiltering(int a_tfMagnification, int a_tfMinification)
{
	// Set magnification filter
	if (a_tfMagnification == TEXTURE_FILTER_MAG_NEAREST)
		glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	else if (a_tfMagnification == TEXTURE_FILTER_MAG_BILINEAR)
		glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Set minification filter
	if (a_tfMinification == TEXTURE_FILTER_MIN_NEAREST)
		glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	else if (a_tfMinification == TEXTURE_FILTER_MIN_BILINEAR)
		glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	else if (a_tfMinification == TEXTURE_FILTER_MIN_NEAREST_MIPMAP)
		glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER,
				GL_NEAREST_MIPMAP_NEAREST);
	else if (a_tfMinification == TEXTURE_FILTER_MIN_BILINEAR_MIPMAP)
		glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_NEAREST);
	else if (a_tfMinification == TEXTURE_FILTER_MIN_TRILINEAR)
		glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);

	tfMinification = a_tfMinification;
	tfMagnification = a_tfMagnification;
}

//------------------------------------------------------------------------------------------------

void TextureManager::setWraping(GLenum _wrap)
{
	// Set magnification filter
	glBindTexture(texData.target, texData.textureID);

	glTexParameterf(texData.target, GL_TEXTURE_WRAP_S, _wrap);
	glTexParameterf(texData.target, GL_TEXTURE_WRAP_T, _wrap);
}

//------------------------------------------------------------------------------------------------

void TextureManager::bind()
{
	glBindTexture(texData.target, texData.textureID);
}

//------------------------------------------------------------------------------------------------

void TextureManager::bind(GLuint _texUnit)
{
	glActiveTexture(GL_TEXTURE0 + _texUnit);
	glBindTexture(texData.target, texData.textureID);
//        samplerUnit = _samplerUnit;
//        glBindSampler(samplerUnit, samplerID);
}

//------------------------------------------------------------------------------------------------

void TextureManager::bind(GLuint _samplerUnit, GLuint _samplerID,
		GLuint _texUnit)
{
	samplerUnit = _samplerUnit;
	glActiveTexture(GL_TEXTURE0 + _texUnit);
	glBindTexture(texData.target, texData.textureID);
	glBindSampler(samplerUnit, _samplerID);
}

//------------------------------------------------------------------------------------------------

void TextureManager::unbind()
{
	glBindTexture(texData.target, 0);
//        glBindSampler(samplerUnit, 0);
}

//------------------------------------------------------------------------------------------------

void TextureManager::releaseTexture()
{

	//  glDeleteSamplers(1, &samplerID);
	if (texData.textureID != 0){
		glDeleteTextures(1, &texData.textureID);
	}
	texData.textureID = 0;
}

//------------------------------------------------------------------------------------------------

void TextureManager::getGlFormatAndType(GLenum glInternalFormat,
		GLenum& glFormat, GLenum& type)
{
	switch (glInternalFormat)
	{
	case GL_R8:
		glFormat = GL_RED;
		type = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA:
#ifndef TARGET_OPENGLES
	case GL_RGBA8:
#endif
		glFormat = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB:
#ifndef TARGET_OPENGLES
	case GL_RGB8:
#endif
		glFormat = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
//            case GL_LUMINANCE:
//#ifndef TARGET_OPENGLES
//            case GL_LUMINANCE8:
//#endif
//                glFormat = GL_LUMINANCE;
//                type = GL_UNSIGNED_BYTE;
//                break;

#ifndef TARGET_OPENGLES
		// 16-bit unsigned short formats
	case GL_RGBA16:
		glFormat = GL_RGBA;
		type = GL_UNSIGNED_SHORT;
		break;
	case GL_RGB16:
		glFormat = GL_RGB;
		type = GL_UNSIGNED_SHORT;
		break;
//            case GL_LUMINANCE16:
//                glFormat = GL_LUMINANCE;
//                type = GL_UNSIGNED_SHORT;
//                break;

		// 32-bit float formats
	case GL_RGBA32F:
		glFormat = GL_RGBA;
		type = GL_FLOAT;
		break;
	case GL_RGB32F:
		glFormat = GL_RGB;
		type = GL_FLOAT;
		break;
	case GL_RG32F:
		glFormat = GL_RG;
		type = GL_FLOAT;
		break;
	case GL_R32F:
		glFormat = GL_RED;
		type = GL_FLOAT;
		break;
//            case GL_LUMINANCE32F_ARB:
//                glFormat = GL_LUMINANCE;
//                type = GL_FLOAT;
//                break;

		// 16-bit float formats
	case GL_RGBA16F:
		glFormat = GL_RGBA;
		type = GL_FLOAT;
		break;
	case GL_RGB16F:
		glFormat = GL_RGB;
		type = GL_FLOAT;
		break;
	case GL_RG16F:
		glFormat = GL_RG;
		type = GL_FLOAT;
		break;
	case GL_R16F:
		glFormat = GL_RED;
		type = GL_FLOAT;
		break;
//            case GL_LUMINANCE16F_ARB:
//                glFormat = GL_LUMINANCE;
//                type = GL_FLOAT;
//                break;
#endif

		// used by prepareBitmapTexture(), not supported by ofPixels
//            case GL_LUMINANCE_ALPHA:
//#ifndef TARGET_OPENGLES
//            case GL_LUMINANCE8_ALPHA8:
//#endif
//                glFormat = GL_LUMINANCE_ALPHA;
//                type = GL_UNSIGNED_BYTE;
//                break;

	default:
		glFormat = glInternalFormat;
		type = GL_UNSIGNED_BYTE;
		break;
	}
}

unsigned int TextureManager::getId()
{
	return texData.textureID;
}

unsigned int TextureManager::getHeight()
{
	return static_cast<unsigned int>(texData.tex_h);
}

unsigned int TextureManager::getWidth()
{
	return static_cast<unsigned int>(texData.tex_w);
}

unsigned int TextureManager::getNrChans()
{
	return texData.nrChan;
}

unsigned char* TextureManager::getBits()
{
	return texData.bits;
}

float TextureManager::getHeightF()
{
	return static_cast<float>(texData.tex_h);
}

float TextureManager::getWidthF()
{
	return static_cast<float>(texData.tex_w);
}

int TextureManager::getMinificationFilter()
{
	return tfMinification;
}

int TextureManager::getMagnificationFilter()
{
	return tfMagnification;
}

GLfloat* TextureManager::getCoordFromPercent(float xPct, float yPct)
{
	GLfloat* temp = new GLfloat[2];

	if (!texData.bAllocated)
		return temp;

#ifndef TARGET_OPENGLES
	if (texData.target == GL_TEXTURE_RECTANGLE)
	{
		temp[0] = xPct * texData.width;
		temp[1] = yPct * texData.height;
	}
	else
	{
#endif
		xPct *= texData.tex_t;
		yPct *= texData.tex_u;
		temp[0] = xPct;
		temp[1] = yPct;
#ifndef TARGET_OPENGLES	
	}
#endif	
	return temp;
}

std::string* TextureManager::getFileName()
{
	return &filename;
}

bool TextureManager::isAllocated()
{
	return texData.bAllocated;
}

#ifdef __EMSCRIPTEN__
Uint32 TextureManager::get_pixel32( SDL_Surface *surface, int x, int y )
{
	//Convert the pixels to 32 bit
	Uint32 *pixels = (Uint32 *)surface->pixels;

	//Get the requested pixel
	return pixels[ ( y * surface->w ) + x ];
}

void TextureManager::put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel )
{
	//Convert the pixels to 32 bit
	Uint32 *pixels = (Uint32 *)surface->pixels;

	//Set the pixel
	pixels[ ( y * surface->w ) + x ] = pixel;
}

SDL_Surface* TextureManager::flip_surface( SDL_Surface *surface, int flags )
{
	//Pointer to the soon to be flipped surface
	SDL_Surface *flipped = NULL;

	//If the image is color keyed
	if( surface->flags & SDL_SRCCOLORKEY )
	{
		flipped = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel,
				surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, 0 );
	} //Otherwise
	else
	{
		flipped = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel,
				surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );
	}

	//If the surface must be locked
	if( SDL_MUSTLOCK( surface ) )
	{
		//Lock the surface
		SDL_LockSurface( surface );
	}

	//Go through columns
	for( int x = 0, rx = flipped->w - 1; x < flipped->w; x++, rx-- )
	{
		//Go through rows
		for( int y = 0, ry = flipped->h - 1; y < flipped->h; y++, ry-- )
		{
			//Get pixel
			Uint32 pixel = get_pixel32( surface, x, y );
			//Copy pixel
			if( ( flags & FLIP_VERTICAL ) && ( flags & FLIP_HORIZONTAL ) )
			{
				put_pixel32( flipped, rx, ry, pixel );
			}
			else if( flags & FLIP_HORIZONTAL )
			{
				put_pixel32( flipped, rx, y, pixel );
			}
			else if( flags & FLIP_VERTICAL )
			{
				put_pixel32( flipped, x, ry, pixel );
			}
		}
	}

	//Unlock surface
	if( SDL_MUSTLOCK( surface ) )
	{
		SDL_UnlockSurface( surface );
	}

	//Copy color key
	if( surface->flags & SDL_SRCCOLORKEY )
	{
		SDL_SetColorKey( flipped, SDL_RLEACCEL | SDL_SRCCOLORKEY, surface->format->Amask );
	}

	//Return flipped surface
	return flipped;
}
#endif

}
