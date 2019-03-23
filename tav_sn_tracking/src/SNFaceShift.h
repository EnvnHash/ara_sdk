//
// SNFaceShift.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//


#pragma once

#include <iostream>

#include <GeoPrimitives/Quad.h>
/*#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLHistogram.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/PingPongFbo.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/VAO.h>
#include <headers/opencv_headers.h>
#include <OpenCvUtils/CvKalmanFilter.h>
#include <Shaders/ShaderCollector.h>
#include <V4L/V4L.h>
#include <Median.h>
*/
#include <SceneNode.h>

/*
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv.h>
*/

namespace tav
{
    class SNFaceShift : public SceneNode
    {
    public:
    //	enum dispMode { RAW=0, LANDMARKS=1, SHIFT=3 };

        SNFaceShift(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
        ~SNFaceShift();
/*
        Shaders* initShdr(ShaderCollector* shCol);
        Shaders* initWhiteShdr(ShaderCollector* shCol);
        */
        void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
        void update(double time, double dt);
        /*
        void applyAffineTransform(cv::Mat &warpImage, cv::Mat &src, std::vector<cv::Point2f> &srcTri, std::vector<cv::Point2f> &dstTri);
        void calculateDelaunayTriangles(cv::Rect rect, std::vector<cv::Point2f> &points,
        		std::vector< std::vector<int> > &delaunayTri, std::vector<float>& dtf, float normWidth, float normHeight);
        void warpTriangle(cv::Mat &img1, cv::Mat &img2, std::vector<cv::Point2f> &t1,
        		std::vector<cv::Point2f> &t2);
        void getConvexHull(std::vector<int>& hullIndex, std::vector<cv::Point2f>& hull2, std::vector<cv::Point2f>& res_points,
        		std::vector<cv::Point>& hull8U, cv::Mat& maskImg, cv::Mat& mask_out, cv::Rect& r, cv::Point& center,
        		std::vector< std::vector<int> >& dt, std::vector<float>& dtf);
        void getLandmarks(dlib::frontal_face_detector& detector, dlib::shape_predictor& sp,
        		std::vector<cv::Point2f>& _points, cv::Mat& img, cv::Mat& small, int detectFaceScale);
        void smoothPoints(std::vector<cv::Point2f>& inPoints, std::vector<cv::Point2f>& outPoints,
        		std::vector<Median<glm::vec2>*>& _kf);
        		*/
        void onKey(int key, int scancode, int action, int mods);
    private:
        /*
        Quad*   					quad;
        V4L*						vt;
        TextureManager* 			out_tex;
        TextureManager* 			img2_tex;
        TextureManager* 			debug_tex;

        GLSLHistogram*				img1Histo;
        GLSLHistogram*				img2Histo;

        FBO* 						mask_tex;
        FastBlurMem*				blur;

        VAO*						del_tri_vao=0;

        Shaders*					shiftShdr;
        Shaders*					whiteShdr;
        Shaders*					stdTex;

        dispMode					actDispMode;
        bool    					isInited = false;

    	float						brightness=0;
    	float 						lastBrightness=0;
    	float						contrast =0;
    	float						lastContrast=0;
    	float						gain=0;
    	float 						lastGain=0;
    	float 						saturation=0;
    	float 						lastSaturation=0;
    	float 						hue=0;
    	float 						lastHue=0;

    	float						kfSmoothFact;
    	float 						scaleFact;

    	dlib::shape_predictor 		sp;
    	dlib::frontal_face_detector detector;

    	// Find convex hull
    	std::vector<cv::Point2f> 	hull1;
    	std::vector<cv::Point2f> 	hull2;
    	std::vector<int> 			hullIndex;
    	std::vector<std::vector<int> > delaunay_triangles1;
    	std::vector<std::vector<int> > delaunay_triangles2;
    	std::vector<cv::Point> 		hull8U;
    	cv::Mat 					output;
    	cv::Mat						img2;
    	cv::Mat 					mask;
    	cv::Mat						inFrame;
    	cv::Mat						small;
    	cv::Rect 					r;
    	cv::Mat 					resize;
    	cv::Rect					roi;

    	cv::Point 					center;

    	std::vector<cv::Point2f> 	points1, points2;
    	std::vector<cv::Point2f> 	smoothPoints1;
    	std::vector<glm::vec2> 		lastInPoints;

    	//std::vector<CvKalmanFilter*> kf;
    	std::vector<Median<glm::vec2>*> kf;

    	int 						detectFaceScale;

    	double 						e1, e2, diff;
    	int							lastFrame=-1;

    	std::vector<float>			delaunay_triangles1f;
    	std::vector<float>			delaunay_triangles2f;

    	glm::vec3					img2SpecMin;
    	glm::vec3					img2SpecMax;
    	glm::vec3					img1SpecMin;
    	glm::vec3					img1SpecMax;
    	*/

    };
}

