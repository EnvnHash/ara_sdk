//
//  opencv_headers.h
//  Tav_App
//
//  Created by Sven Hahne on 29/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//
//  opencv2/header.hpp is opencv 3.0
//  opencv2/header/header.hpp is opencv 2.4 may lead to problems...

#pragma once

#ifdef ARA_USE_OPENCV

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/opengl.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/video.hpp>

#ifdef HAVE_CUDA
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <opencv2/cudaoptflow.hpp>
#endif

#endif