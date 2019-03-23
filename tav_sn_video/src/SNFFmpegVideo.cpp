//
// SNFFmpegVideo.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNFFmpegVideo.h"

namespace tav
{
SNFFmpegVideo::SNFFmpegVideo(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	stdTex = shCol->getStdTex();
	quad = static_cast<Quad*>(_scd->stdHFlipQuad);

//	ffmpeg.OpenFile(shCol, "/home/sven/Videos/test_youtube.mp4", 4, 640, 360, true, true);
//	ffmpeg.OpenFile(shCol, "/mnt/data/projekte/pi/projekte/2018/GAM_mustakis/modulo_libro/kaputte_videos/02BiografiaINTRO2.m4v	", 4, 640, 360, true, true);
	ffmpeg.OpenFile(shCol, (*_scd->dataPath)+"/movies/00_Entrada_Loop.mp4", 4, 640, 360, true, true);

    //ffmpeg.OpenFile(shCol, "rtsp://root:ik2017SH0W@190.196.18.5:554/live2.sdp", 4, 1280, 720, true, true);
	//ffmpeg.OpenFile(shCol, "rtsp://192.168.1.153:8080/video/h264", 4, 1280, 720, true, false);
	//ffmpeg.OpenFile(shCol, (*_scd->dataPath)+"/movies/test.mp4", 4, 960, 540, true, false);
	ffmpeg.start();
}

//------------------------------------------

void SNFFmpegVideo::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ffmpeg.shaderBegin(); // draw with conversion yuv -> rgb on gpu
	quad->draw(_tfo);

	_shader->begin();
}

//------------------------------------------

void SNFFmpegVideo::update(double time, double dt)
{
	ffmpeg.loadFrameToTexture(time);
}

//------------------------------------------

SNFFmpegVideo::~SNFFmpegVideo()
{
	delete quad;
}
}
