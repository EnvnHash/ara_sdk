/*
 * ImgSeqPlayer.cpp
 *
 *  Created on: Jul 22, 2018
 *      Author: sven
 */

#include "ImgSeqPlayer.h"


// files muessen mit 0001 beginnen!!!!!!!!

namespace tav
{

ImgSeqPlayer::ImgSeqPlayer(const char* folder, unsigned int _fps):
		fps(_fps), loadFrameQueueSize(10), nrFramesInLoadQueue(0), run(false),
		frameQueueLoadPtr(0), fsFramePtr(0), nrFramesInFile(0), nrColChans(3),
		nrLoadThreads(1), uplPtr(0)
{
	DIR *dir;
	struct dirent *ent;

	printf("ImgSeqPlayer open %s \n", folder);

	// count files in dir
	if ((dir = opendir (folder)) != NULL)
	{
		// print all the files and directories within directory
		while ((ent = readdir (dir)) != NULL)
		{
			if (std::strcmp(ent->d_name, ".") != 0 && std::strcmp(ent->d_name, ".."))
				nrFramesInFile++;
		}
	}

	printf("ImgSeqPlayer % d files in folder \n", nrFramesInFile);

	// make an array to save them
	files = new std::string[nrFramesInFile];

	// assign the fileNames to frameNumbers
	if ((dir = opendir (folder)) != NULL){

		while ((ent = readdir (dir)) != NULL)
		{
			if (std::strcmp(ent->d_name, ".") != 0 && std::strcmp(ent->d_name, "..")){

			//	printf ("%s\n", ent->d_name);

				std::string strFormat = std::string(ent->d_name);
				std::vector<std::string> sepFormat = split(strFormat, '_');

				std::string numb = sepFormat[sepFormat.size()-1];
				std::vector<std::string> sepFormat2 = split(numb, '.');

				unsigned int frameNr = std::atoi(sepFormat2[0].c_str());

				files[frameNr-1] = std::string(folder)+"/"+ent->d_name;
			}
		}

		closedir (dir);

	} else {
		// could not open directory
		perror ("");
	}

	initPars();
}

//----------------------------------------------------

ImgSeqPlayer::~ImgSeqPlayer()
{
	FreeImage_DeInitialise();

	delete [] frameQueue;
	delete [] files;
}

//----------------------------------------------------

void ImgSeqPlayer::initPars()
{
	// Initialize FreeImage
	FreeImage_Initialise();

	//retrieve the fist frame to get the data
	FIBITMAP* pBitmap = ImageLoader(0, 0);

	width = FreeImage_GetWidth(pBitmap);
	height = FreeImage_GetHeight(pBitmap);
	BPP = FreeImage_GetBPP(pBitmap);
	colorType = FreeImage_GetColorType(pBitmap);

	getGlFormat();

	// build framequeue
	frameQueue = new Frame[loadFrameQueueSize];


	/*
	// create textures
	for (unsigned int i=0; i<loadFrameQueueSize; i++)
	{
		frameQueue[i].bitmap = 0;
		frameQueue[i].block = true;
		frameQueue[i].loaded = false;
		allocTexture(&frameQueue[i]);
	}

	loadingThreads = new std::thread[nrLoadThreads];
	*/
}

//----------------------------------------------------

void ImgSeqPlayer::allocTexture(Frame* _frame)
{
//	printf("alloc width %d height %d nrColChans %d \n", width, height, nrColChans);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	//generate an OpenGL texture ID for this texture
	glGenTextures(1, (GLuint *) &_frame->texId); // could be more then one, but for now, just one

	//bind to the new texture ID
	glBindTexture(GL_TEXTURE_2D, _frame->texId);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);
	glTexSubImage2D(GL_TEXTURE_2D,             // target
			0,                          // mipmap level
			0, 0,                       // x and y offset
			width,              // width and height
			height,
			format,
			type,
			NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// set linear filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
}

//----------------------------------------------------

void ImgSeqPlayer::start(double time)
{
	// start loading thread
	run = true;
	startTime = time;
	loadingThreads[0] = std::thread(&ImgSeqPlayer::loadLoop, this);
}

//----------------------------------------------------

void ImgSeqPlayer::loadLoop()
{
	std::unique_lock<std::mutex> lock(loadMtx);

	while (run)
	{
		// if the queue is not filled, load files
		std::cout << "nrFramesInLoadQueue " << nrFramesInLoadQueue << std::endl;

		if (nrFramesInLoadQueue < loadFrameQueueSize)
		{
			std::cout << "Load " << files[fsFramePtr] << std::endl;

			watch.setStart();

			loadAndDecode(fsFramePtr, frameQueueLoadPtr);

			watch.setEnd();
			watch.print((char*)"load t: ");

			frameQueueLoadPtr = (frameQueueLoadPtr +1) % loadFrameQueueSize;
			fsFramePtr++;
			nrFramesInLoadQueue++;

			if (fsFramePtr == nrFramesInFile)
			{
				std::cout << "reached end of file" << std::endl;
			}
		} else
		{
			loadContinueCond.wait(lock);	 // wait until a new frame is needed
		}
	}
}

//----------------------------------------------------

void ImgSeqPlayer::loadAndDecode(unsigned int frameNr, unsigned int queuePtr)
{
	// lock frame mutex
	frameQueue[frameQueueLoadPtr].block = true;

	// free last bitmap data
	if (frameQueue[frameQueueLoadPtr].bitmap != 0)
		FreeImage_Unload(frameQueue[frameQueueLoadPtr].pBitmap);

	frameQueue[frameQueueLoadPtr].pBitmap = ImageLoader(frameNr, 0);
	frameQueue[frameQueueLoadPtr].bitmap = (GLubyte*) FreeImage_GetBits(frameQueue[frameQueueLoadPtr].pBitmap);
	frameQueue[frameQueueLoadPtr].pts = (double)frameNr / (double)fps;
	frameQueue[frameQueueLoadPtr].loaded = true;
	frameQueue[frameQueueLoadPtr].block = false;

//	frameQueue[frameQueueLoadPtr].mtx.unlock();
}

//----------------------------------------------------

void ImgSeqPlayer::update(double time)
{
	if (time > frameQueue[uplPtr].pts && !frameQueue[uplPtr].block)
	{
		std::cout << "upload !!! " << std::endl;

		frameQueue[uplPtr].block = true;
		presentPtr = uplPtr;

		//bind to the new texture ID
		glBindTexture(GL_TEXTURE_2D, frameQueue[uplPtr].texId);
		glTexSubImage2D(GL_TEXTURE_2D,      // target
				0,                          // mipmap level
				0, 0,                       // x and y offset
				width,              		// width and height
				height,
				format,
				type,
				frameQueue[uplPtr].bitmap);

		frameQueue[uplPtr].block = false;
		frameQueue[uplPtr].loaded = false;

		nrFramesInLoadQueue--;

		uplPtr = (uplPtr +1) % loadFrameQueueSize;
	}
}

//----------------------------------------------------

FIBITMAP* ImgSeqPlayer::ImageLoader(unsigned int fileNr, int flag)
{
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	const char* path = files[fileNr].c_str();

	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(path, flag);

	if (fif == FIF_UNKNOWN)
	{
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(path);
	}
	// check that the plugin has reading capabilities ...
	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))
	{
		// ok, let's load the file
		return FreeImage_Load(fif, path, flag);
	}
	else
	{
		std::cout << " unknown format " << std::endl;
		return 0;
	}
}

//----------------------------------------------------

void ImgSeqPlayer::getGlFormat()
{
	switch (colorType)
	{
		case FIC_MINISBLACK:
			nrColChans = 1;
			format = GL_RED;
			internalFormat = BPP == 32 ? GL_R32F : BPP == 16 ? GL_R16F : BPP == 8 ? GL_R8 : 0;
			type = internalFormat == GL_R8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
			break;
		case FIC_MINISWHITE:
			nrColChans = 1;
			format = GL_RED;
			internalFormat = BPP == 32 ? GL_R32F : BPP == 16 ? GL_R16F : BPP == 8 ? GL_R8 : 0;
			type = internalFormat == GL_R8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
			break;
		case FIC_PALETTE:
			nrColChans = 3;
			format = GL_BGR;
			internalFormat = GL_RGB8;
			type = GL_UNSIGNED_BYTE;
			break;
		case FIC_RGB:

			std::cout << "    RGB" << std::endl;

			nrColChans = 3;
			format = GL_BGR;
			internalFormat = BPP == 96 ? GL_RGB32F : BPP == 48 ? GL_RGB16F : BPP == 24 ? GL_RGB8 : 0;
			type = internalFormat == GL_RGB8 ? GL_UNSIGNED_BYTE : GL_FLOAT;

			// strange effect when exporting tiff from gimp... fi says FIC_RGB, but has 32 bit...
			if (BPP == 32)
			{
				internalFormat = GL_RGBA8;
				format = GL_BGRA;
				nrColChans = 4;
				type = GL_UNSIGNED_BYTE;
			}
			break;
		case FIC_RGBALPHA:
			nrColChans = 4;
			format = GL_BGRA;
			internalFormat = BPP == 128 ? GL_RGBA32F : BPP == 64 ? GL_RGBA16F : BPP == 32 ? GL_RGBA8 : 0;
			type = internalFormat == GL_RGBA8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
			break;
		case FIC_CMYK:
			nrColChans = 4;
			format = GL_BGRA;
			internalFormat = BPP == 128 ? GL_RGBA32F : BPP == 64 ? GL_RGBA16F : BPP == 32 ? GL_RGBA8 : 0;
			type = internalFormat == GL_RGBA8 ?
							GL_UNSIGNED_BYTE : GL_FLOAT;
			break;
		default:
			printf("TextureManager::loadFromFile Error: unknown number of channels\n");
	}
}

//----------------------------------------------------

void ImgSeqPlayer::bind(unsigned int texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	std::cout << "texId: " <<   frameQueue[presentPtr].texId << std::endl;
	glBindTexture(GL_TEXTURE_2D, frameQueue[presentPtr].texId);
}

} /* namespace tav */
