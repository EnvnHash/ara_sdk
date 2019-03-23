/*
*  FboDedistGridN.hpp
*  tav_core
*
*  Created by Sven Hahne on 22/12/15.
*  Copyright © 2015 Sven Hahne. All rights reserved..
*
*
*  A FBO with a n * n point grid for non-linearily (de)distortion
*
*				   xtN
*  		o-----o-----o-----o-----o
*  		|	  |	    |	  |	    |
*  		|	  |	    |	  |	    |
* 		|	  |	    |	  |	    |
*  		o-----*-----*-----*-----o
*  ylN	|	  |	    |	  |	    | yrN
*  		|	  |	    |	  |	    |
*  		|	  |	    |	  |	    |
*  		o-----*-----*-----*-----o
*  		|	  |	    |	  |	    |
*  		|	  |	    |	  |	    |
*  		|	  |	    |	  |	    |
*  		o-----o-----o-----o-----o
*				   xbN
*
*	Only the position of the of the border points get defined, the points in between are
*	calculated by intersecting the specific line between the x-axis points and y-axis points.
*
*	All points can be controlled via OSC.
*	Optionally a yml file can be used for saving and loading.
*	Changes are saved automatically.
*
*	An instance has to be bound by the bind() method, then drawing has to occur and afterwards unbind() has to be called
*
*
*/

#ifndef FboDedistGridN_hpp
#define FboDedistGridN_hpp

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "GLUtils/FBO.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/QuadArray.h"
#include "GLUtils/UniformBlock.h"

namespace tav
{
class FboDedistGridN
{
public:
	FboDedistGridN(ShaderCollector* _shCol, unsigned _nrGridPointsX, unsigned _nrGridPointsY,
			unsigned int _width, unsigned int _height, GLenum _type, GLenum _target,
			bool _depthBuf, int _nrSample, bool _saveToDisk);
	~FboDedistGridN();
	void init();
	void onKey(int key, int scancode, int action, int mods);
	void bind() { fbo->bind(); }
	void clear() { fbo->clear(); }
	void unbind() { fbo->unbind(); }
	void draw();
	void drawExtFbo(int scrWidth, int scrHeight, int texId);

	void setupRenderFboShader();
	glm::mat4 cvMat33ToGlm(cv::Mat& _mat);
	void loadCalib();
	void saveCalib();

	inline void setCalibFileName(std::string _name)
	{
		fileName = _name;
	}

private:
	FBO*						fbo;
	ShaderCollector* 			shCol;
	Shaders* 					texShader;
	Shaders* 					colShader;

	QuadArray* 					fboQuad;
	Quad* 						cornerQuad;
	UniformBlock* 				uBlock;

	std::vector<glm::vec2>		corners;
	std::vector<std::string>	cornersNames;
	std::vector<glm::vec2> 		blCorners;
	std::vector<bool> 			cornersState;
	std::string 				fileName;

	std::vector<cv::Point2f> 	quadPoint;

	glm::mat4 					mat;

	GLfloat 					quadSkel[8];

	unsigned int 				width;
	unsigned int 				height;
	unsigned int 				nrGridPointsX;
	unsigned int 				nrGridPointsY;

	unsigned int 				nrSep;
	unsigned int 				nrCtx;
	unsigned int 				cornersSize;

	bool 						showCorners;
	bool 						newCornerVals;
	bool 						saveToDisk;
};
}

#endif /* FboDedistGridN_hpp */
