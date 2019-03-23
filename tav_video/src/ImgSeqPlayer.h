/*
 * ImgSeqPlayer.h
 *
 *  Created on: Jul 22, 2018
 *      Author: sven
 */

#ifndef SRC_IMGSEQPLAYER_H_
#define SRC_IMGSEQPLAYER_H_

#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <mutex>
#include <vector>
#include <stdio.h>
#include <string>
#include <thread>

#include <FreeImage.h>

#include <GLUtils/TextureManager.h>
#include <string_utils.h>
#include <headers/sceneData.h>
#include "StopWatch.h"

namespace tav
{

class ImgSeqPlayer
{
public:

	typedef struct {
		bool			block;
		bool			loaded;
		double			pts;
		unsigned int	nr;
		std::mutex 		mtx;
		GLuint			texId;
		GLubyte*		bitmap;
		FIBITMAP* 		pBitmap;
	} Frame;

	ImgSeqPlayer(const char* folder, unsigned int _fps);
	virtual ~ImgSeqPlayer();

	void initPars();
	void allocTexture(Frame* _frame);
	void start(double time);
	void loadLoop();
	void loadAndDecode(unsigned int frameNr, unsigned int queuePtr);

	void update(double time);

	FIBITMAP* ImageLoader(unsigned int fileNr, int flag);
	void getGlFormat();

	void bind(unsigned int texUnit);


private:
	bool						run;

	unsigned int 				fps;
	unsigned int 				nrFramesInLoadQueue;
	unsigned int 				loadFrameQueueSize;
	unsigned int 				frameQueueLoadPtr;
	unsigned int 				fsFramePtr;
	unsigned int 				nrFramesInFile;

	unsigned int 				nrColChans;
	unsigned int				width;
	unsigned int				height;
	unsigned int				BPP;

	unsigned int				nrLoadThreads;
	unsigned int				uplPtr;
	unsigned int				presentPtr;

	double						startTime;

	std::string*				files;
	std::thread* 				loadingThreads;
	std::mutex					loadMtx;
	std::condition_variable 	loadContinueCond;

	FREE_IMAGE_COLOR_TYPE 		colorType;
	GLenum						format;
	GLenum						internalFormat;
	GLenum						type;

	Frame*						frameQueue;
	StopWatch					watch;

};

} /* namespace tav */

#endif /* SRC_IMGSEQPLAYER_H_ */
