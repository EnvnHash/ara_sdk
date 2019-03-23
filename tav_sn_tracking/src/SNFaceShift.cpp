//
// SNFaceShift.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//


#include "SNFaceShift.h"

#define STRINGIFY(A) #A

namespace tav
{
SNFaceShift::SNFaceShift(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs, "NoLight")
//	detectFaceScale(3), actDispMode(SHIFT),kfSmoothFact(1.f)
{
/*
	GWindowManager* winMan = (GWindowManager*)_scd->winMan;
	ShaderCollector* _shCol = (ShaderCollector*)_scd->shaderCollector;
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
			return this->onKey(key, scancode, action, mods); });

	vt = new V4L((ShaderCollector*) _scd->shaderCollector, (char*) "/dev/video0",
			encodingmethod_e::MJPEG, 1280, 720);

	shiftShdr = initShdr(_shCol);
	whiteShdr = initWhiteShdr(_shCol);
	stdTex = _shCol->getStdTex();

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
			NULL, 1, true);

	// sample controls
	addPar("Brightness", &brightness);
	addPar("Contrast", &contrast);
	addPar("Gain", &gain);
	addPar("Saturation", &saturation);
	addPar("Hue", &hue);


	std::cout << vt->getHeight() << ", " << vt->getWidth() << std::endl;
	inFrame = cv::Mat(vt->getHeight(), vt->getWidth(), CV_8UC3);

	// init dlib
	try
	{
		// We need a face detector.  We will use this to get bounding boxes for
		// each face in an image.
		detector = dlib::get_frontal_face_detector();

		// And we also need a shape_predictor.  This is the tool that will predict face
		// landmark positions given an image and face bounding box.  Here we are just
		// loading the model from the shape_predictor_68_face_landmarks.dat file you gave
		// as a command line argument.
		dlib::deserialize((*_scd->dataPath)+"dlib/shape_predictor_68_face_landmarks.dat") >> sp;
	} catch (std::exception& e)
	{
		std::cerr << "\nexception thrown!" << std::endl;
		std::cerr << e.what() << std::endl;
	}


	//Read input images
	img2 = cv::imread((*_scd->dataPath)+"textures/Sebastian_Pinera.JPG");

	img2_tex = new TextureManager();
	img2_tex->loadTexture2D((*_scd->dataPath)+"textures/Sebastian_Pinera.JPG");

	img2Histo = new GLSLHistogram((ShaderCollector*) _scd->shaderCollector,
			img2.size().width, img2.size().height, GL_RGB8, 4, 512, true, false);


	// scale inFrame to fit the img2, height or width, whatever is bigger
	scaleFact = std::fmax( float(img2.size().height) / float(inFrame.size().height),
			float(img2.size().width) / float(inFrame.size().width) );

	roi.x = int( (float(inFrame.size().width) * scaleFact - float(img2.size().width)) * 0.5f ) ;
	roi.y = int( (float(inFrame.size().height) * scaleFact - float(img2.size().height)) * 0.5f ) ;
	roi.width = img2.size().width;
	roi.height = img2.size().height;


	// make a texture to upload the result
	out_tex = new TextureManager();
	out_tex->allocate(vt->getWidth(), vt->getHeight(), GL_RGBA8, GL_BGR, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);

	img1Histo = new GLSLHistogram((ShaderCollector*) _scd->shaderCollector,
			vt->getWidth(), vt->getHeight(), GL_RGB8, 4, 512, true, false);


	mask_tex = new FBO(_shCol, img2.size().width, img2.size().height);

    blur = new FastBlurMem(0.2f, _shCol, img2.size().width, img2.size().height);
    blur->setOffsScale(1.2f);

//	mask_tex = new TextureManager();
//	mask_tex->allocate(img2.size().width, img2.size().height, GL_RGBA8, GL_BGR, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);

	debug_tex = new TextureManager();
	debug_tex->allocate(vt->getWidth(), vt->getHeight(), GL_RGBA8, GL_BGR, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);


	del_tri_vao = new VAO("position:2f,texCoord:2f", GL_DYNAMIC_DRAW);
	*/
}

//----------------------------------------------------------------------------------------------
/*
Shaders* SNFaceShift::initShdr(ShaderCollector* shCol)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 2 ) in vec2 texCoord;\n
		uniform mat4 m_pvm;\n
		out vec2 tex_coord;\n

		void main(){\n
			tex_coord = (texCoord.xy + vec2(1.0)) * 0.5;\n
			gl_Position = m_pvm * position;\n
	});
	vert = "// SNFaceShift, vert\n"+shdr_Header+vert;


	std::string frag = STRINGIFY(
		uniform sampler2D mask;
		uniform sampler2D img2;
		in vec2 tex_coord;\n
		layout (location = 0) out vec4 color;\n

		void main(){\n
			color = texture(img2, tex_coord);\n
			color.a = texture(mask, tex_coord).r;\n
//			color = texture(mask, tex_coord);\n
//			color = texture(video_in, tex_coord) * 0.5 + texture(img2, vec2(tex_coord.x, 1.0 - tex_coord.y)) * 0.5 ;\n
	});

	frag = "// SNFaceShift, frag\n"+shdr_Header+frag;

	return shCol->addCheckShaderText("SNFaceShift", vert.c_str(), frag.c_str());
}

//----------------------------------------------------------------------------------------------

Shaders* SNFaceShift::initWhiteShdr(ShaderCollector* shCol)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		uniform mat4 m_pvm;\n
		void main(){\n
			gl_Position = m_pvm * position;\n
	});
	vert = "// SNFaceShift white, vert\n"+shdr_Header+vert;


	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;\n

		void main(){\n
			color = vec4(1.0);\n
	});

	frag = "// SNFaceShift, white, frag\n"+shdr_Header+frag;

	return shCol->addCheckShaderText("SNFaceShift_white", vert.c_str(), frag.c_str());
}

//----------------------------------------------------------------------------------------------
*/
void SNFaceShift::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
/*
	sendStdShaderInit(_shader);

	if ( actDispMode == RAW )
	{
		useTextureUnitInd(0, vt->getActTexId(), _shader, _tfo);
		quad->draw(_tfo);

	} else if (actDispMode == LANDMARKS)
	{
		useTextureUnitInd(0, debug_tex->getId(), _shader, _tfo);
		quad->draw(_tfo);

	} else if (actDispMode == SHIFT)
	{
		// draw video in
		useTextureUnitInd(0, out_tex->getId(), _shader, _tfo);
		quad->draw(_tfo);

		// draw shifted face
		shiftShdr->begin();
		shiftShdr->setIdentMatrix4fv("m_pvm");
		shiftShdr->setUniform1i("mask", 0);
		shiftShdr->setUniform1i("img2", 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blur->getResult());

		img2_tex->bind(1);

		del_tri_vao->drawElements(GL_TRIANGLES, _tfo, GL_TRIANGLES);


		//glDisable(GL_DEPTH_TEST);

		//stdTex->begin();
		//stdTex->setIdentMatrix4fv("m_pvm");
		//stdTex->setUniform1i("tex", 0);

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, blur->getResult());

		//quad->draw(_tfo);

	}
	*/
}

//----------------------------------------------------------------------------------------------

void SNFaceShift::update(double time, double dt)
{
	/*
	// setCtrl take A LOT time, only change if necessary
	if ( lastBrightness != brightness)
	{
		lastBrightness = brightness;
		vt->setCtrl("Brightness", brightness);
	}
	if ( lastContrast != contrast)
	{
		lastContrast = contrast;
		vt->setCtrl("Contrast", contrast);
	}
	if ( lastGain != gain)
	{
		lastGain = gain;
		vt->setCtrl("Gain", gain);
	}
	if ( lastSaturation != saturation)
	{
		lastSaturation = saturation;
		vt->setCtrl("Saturation", saturation);
	}
	if ( lastHue != hue)
	{
		lastHue = hue;
		vt->setCtrl("Hue", hue);
	}

	//--------------------------------------------------------

	if ( actDispMode == RAW )
	{
		vt->loadFrameToTexture();

	} else
	{
		if (vt->getActFrameNr() != -1 && vt->getActFrameNr() != lastFrame && vt->getActBuf())
		{
			lastFrame = vt->getActFrameNr();
			inFrame.data = vt->getActBuf();

			if (inFrame.data)
			{
				// create a downscaled version of the realtime input to speed up face detection
				cv::resize(inFrame, small,
						cv::Size(inFrame.size().width / detectFaceScale,
								inFrame.size().height / detectFaceScale));

				//----------------------------------------------------
				// initial landmark detection for img2
				if ( points2.size() == 0 )
				{
					// get histogram
					img2Histo->proc(img2_tex->getId());
					for (unsigned short i=0;i<3;i++)
					{
						img2SpecMin[i] = img2Histo->getMinInd(i);
						img2SpecMax[i] = img2Histo->getMaxInd(i);
					}

//					std::cout << glm::to_string(img2SpecMin) << std::endl;
//					std::cout << glm::to_string(img2SpecMax) << std::endl;


					getLandmarks(detector, sp, points2, img2, img2, 1);
					getConvexHull(hullIndex, hull2, points2, hull8U, img2, mask, r, center, delaunay_triangles2,
							delaunay_triangles2f);

					// initial upload
					// normalize landmark positions
					GLfloat* landMarksPos = new GLfloat[points2.size()*2];
					for (std::vector<cv::Point2f>::iterator it = points2.begin(); it != points2.end(); ++it)
					{
						landMarksPos[(it - points2.begin()) *2] = (*it).x / img2.size().width * 2.f - 1.f;
						landMarksPos[(it - points2.begin()) *2 +1] = (1.f - (*it).y / img2.size().height) * 2.f -1.f;
					}

					GLuint* ind = new GLuint[delaunay_triangles2.size()*3];
					for (std::vector<std::vector<int>>::iterator it = delaunay_triangles2.begin(); it != delaunay_triangles2.end(); ++it)
						for (int i=0;i<3;i++)
							ind[(it - delaunay_triangles2.begin()) *3 +i] = (*it)[i];

					del_tri_vao->upload(POSITION, &landMarksPos[0], (unsigned int) points2.size());
					del_tri_vao->upload(TEXCOORD, &landMarksPos[0], (unsigned int) points2.size());
					del_tri_vao->setElemIndices(delaunay_triangles2.size()*3, &ind[0]);

					// upload mask
					mask_tex->bind();
					whiteShdr->begin();
					whiteShdr->setIdentMatrix4fv("m_pvm");

					glDisable(GL_CULL_FACE);
					glDisable(GL_DEPTH_TEST);
					del_tri_vao->drawElements(GL_TRIANGLES, 0, GL_TRIANGLES);
					mask_tex->unbind();

					// apply blur
                    blur->proc(mask_tex->getColorImg());
					for (int j=0;j<8;j++)
	                    blur->proc(blur->getLastResult());
				}

				//----------------------------------------------------
				// get landmarks for realtime input, use the downscaled version
				// result will be saved in points1 (correct to fit the original size of the realtime input
				points1.clear();
				getLandmarks(detector, sp, points1, inFrame, small, detectFaceScale);
				smoothPoints(points1, smoothPoints1, kf);

				if(smoothPoints1.size() > 0)
				{
					// normalize landmark positions
					GLfloat* landMarksPos1 = new GLfloat[smoothPoints1.size()*2];
					for (std::vector<cv::Point2f>::iterator it = smoothPoints1.begin(); it != smoothPoints1.end(); ++it)
					{
						landMarksPos1[(it - smoothPoints1.begin()) *2] = ((*it).x * scaleFact - roi.x) / img2.size().width * 2.f - 1.f;
						landMarksPos1[(it - smoothPoints1.begin()) *2 +1] = (1.f - ((*it).y * scaleFact - roi.y) / img2.size().height) * 2.f -1.f;

						del_tri_vao->upload(POSITION, &landMarksPos1[0], (unsigned int) smoothPoints1.size());
					}
				}

				if (actDispMode == LANDMARKS)
				{
					for (std::vector<cv::Point2f>::iterator it = smoothPoints1.begin(); it!= smoothPoints1.end(); ++it)
					{
						cv::circle(inFrame,
								(*it),
								2,
								cv::Scalar(0,255,0),
								1,
								cv::LINE_8,
								0
						);

						cv::putText(inFrame,
								cv::String( std::to_string(it - smoothPoints1.begin()) ),
								(*it),
								cv::FONT_HERSHEY_PLAIN,
								1,
								cv::Scalar(0,255,0));
					}
				} else if (actDispMode == SHIFT && delaunay_triangles1f.size() > 0)
				{
					del_tri_vao->upload(TEXCOORD, &delaunay_triangles2f[0], delaunay_triangles2f.size()/2);
				}

				// upload
				switch (actDispMode)
				{
				case LANDMARKS:
					debug_tex->bind(0);
					glTexSubImage2D(GL_TEXTURE_2D,
							0,
							0, 0,
							debug_tex->getWidth(), debug_tex->getHeight(),
							GL_BGR,
							GL_UNSIGNED_BYTE,
							inFrame.data);
					break;
				case SHIFT:
					out_tex->bind(0);
					glTexSubImage2D(GL_TEXTURE_2D,
							0,
							0, 0,
							out_tex->getWidth(), out_tex->getHeight(),
							GL_BGR,
							GL_UNSIGNED_BYTE,
							inFrame.data);

					img1Histo->proc(out_tex->getId());
					for (unsigned short i=0;i<3;i++)
					{
						img1SpecMin[i] = img1Histo->getMinInd(i);
						img1SpecMax[i] = img1Histo->getMaxInd(i);
					}
//
//					std::cout << glm::to_string(img1SpecMin) << std::endl;
//					std::cout << glm::to_string(img1SpecMax) << std::endl;

					break;
				default:
					break;
				}
			}
		}
	}
	*/
}

/*
//----------------------------------------------------------------------------------------------

// Apply affine transform calculated using srcTri and dstTri to src
void SNFaceShift::applyAffineTransform(cv::Mat &warpImage, cv::Mat &src,
		std::vector<cv::Point2f> &srcTri, std::vector<cv::Point2f> &dstTri)
{
	// Given a pair of triangles, find the affine transform.
	cv::Mat warpMat = getAffineTransform( srcTri, dstTri );

	// Apply the Affine Transform just found to the src image
	cv::warpAffine( src, warpImage, warpMat, warpImage.size(), cv::INTER_LINEAR, cv::BORDER_REFLECT_101);
}

//----------------------------------------------------------------------------------------------

// Calculate Delaunay triangles for set of points
// Returns the vector of indices of 3 points for each triangle
void SNFaceShift::calculateDelaunayTriangles(cv::Rect rect, std::vector<cv::Point2f> &points,
		std::vector< std::vector<int> > &delaunayTri, std::vector<float>& delaunayTrif, float normWidth, float normHeight)
{
	// Create an instance of Subdiv2D
	cv::Subdiv2D subdiv(rect);

	// Insert points into subdiv
	//for( std::vector<cv::Point2f>::iterator it = points.begin(); it != points.end(); it++)
		subdiv.insert(points);

	// delaunay triangulation happens exactely here
	std::vector<cv::Vec6f> triangleList;
	subdiv.getTriangleList(triangleList);

	std::vector<cv::Point2f> pt(3);
	std::vector<int> ind(3);
	delaunayTri.clear();
	delaunayTrif.clear();

	for( size_t i = 0; i < triangleList.size(); i++ )
	{
		cv::Vec6f t = triangleList[i];

		pt[0] = cv::Point2f(t[0], t[1]);
		pt[1] = cv::Point2f(t[2], t[3]);
		pt[2] = cv::Point2f(t[4], t[5]);

		if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
		{
			for(int j=0; j<3; j++)
				for(size_t k = 0; k < points.size(); k++)
					if(abs(pt[j].x - points[k].x) < 1.0 && abs(pt[j].y - points[k].y) < 1)
						ind[j] = k;

			delaunayTri.push_back(ind);

			for(int j=0; j<3; j++)
			{
				delaunayTrif.push_back( points[ind[j]].x / normWidth  * 2.f - 1.f );
				delaunayTrif.push_back( (1.f - points[ind[j]].y / normHeight) * 2.f - 1.f);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------

// Warps and alpha blends triangular regions from img1 and img2 to img
void SNFaceShift::warpTriangle(cv::Mat &img1, cv::Mat &img2, std::vector<cv::Point2f> &t1,
		std::vector<cv::Point2f> &t2)
{

	cv::Rect r1 = boundingRect(t1);
	cv::Rect r2 = boundingRect(t2);

	// Offset points by left top corner of the respective rectangles
	std::vector<cv::Point2f> t1Rect, t2Rect;
	std::vector<cv::Point> t2RectInt;
	for(int i = 0; i < 3; i++)
	{

		t1Rect.push_back( cv::Point2f( t1[i].x - r1.x, t1[i].y -  r1.y) );
		t2Rect.push_back( cv::Point2f( t2[i].x - r2.x, t2[i].y - r2.y) );
		t2RectInt.push_back( cv::Point(t2[i].x - r2.x, t2[i].y - r2.y) ); // for fillConvexPoly

	}

	// Get mask by filling triangle
	cv::Mat mask = cv::Mat::zeros(r2.height, r2.width, CV_32FC3);
	cv::fillConvexPoly(mask, t2RectInt, cv::Scalar(1.0, 1.0, 1.0), 16, 0);

	// Apply warpImage to small rectangular patches
	cv::Mat img1Rect;
	img1(r1).copyTo(img1Rect);

	cv::Mat img2Rect = cv::Mat::zeros(r2.height, r2.width, img1Rect.type());

	applyAffineTransform(img2Rect, img1Rect, t1Rect, t2Rect);

	cv::multiply(img2Rect,mask, img2Rect);
	cv::multiply(img2(r2), cv::Scalar(1.0,1.0,1.0) - mask, img2(r2));
	img2(r2) = img2(r2) + img2Rect;
}

//----------------------------------------------------------------------------------------------

void SNFaceShift::getConvexHull(std::vector<int>& hullIndex, std::vector<cv::Point2f>& _hull, std::vector<cv::Point2f>& res_points,
		std::vector<cv::Point>& hull8U, cv::Mat& maskImg, cv::Mat& mask_out, cv::Rect& r, cv::Point& center,
		std::vector< std::vector<int> >& dt, std::vector<float>& dtf)
{
	// calculate convex hull
	hullIndex.clear();
	convexHull(res_points, hullIndex, false, false);

	_hull.clear();
	for(unsigned int i=0; i<hullIndex.size(); i++)
		_hull.push_back( res_points[ hullIndex[i] ] );

	// Calculate mask
	hull8U.clear();
	for(unsigned int i=0; i<_hull.size(); i++)
	{
		cv::Point pt(_hull[i].x, _hull[i].y);
		hull8U.push_back(pt);
	}

	mask_out = cv::Mat::zeros(maskImg.rows, maskImg.cols, maskImg.depth());
	fillConvexPoly(mask_out, &hull8U[0], (int)hull8U.size(), cv::Scalar(255,255,255));

	// Clone seamlessly.
	r = boundingRect(_hull);
	center = (r.tl() + r.br()) / 2;

	// Find delaunay triangulation for points on the convex hull
	cv::Rect rect(0, 0, maskImg.cols, maskImg.rows);
	calculateDelaunayTriangles(rect, res_points, dt, dtf, float(maskImg.size().width), float(maskImg.size().height));
}

//----------------------------------------------------------------------------------------------

void SNFaceShift::getLandmarks(dlib::frontal_face_detector& detector, dlib::shape_predictor& sp,
		std::vector<cv::Point2f>& _points, cv::Mat& img, cv::Mat& small, int detectFaceScale)
{
	double e1, e2, diff;

	dlib::cv_image<dlib::bgr_pixel> scimg(small);

	// Detect faces
	std::vector<dlib::rectangle> faces = detector(scimg);

	dlib::cv_image<dlib::bgr_pixel> cimg(img);

	// Find the pose of each face.
	std::vector<dlib::full_object_detection> shapes;
	for (unsigned long i=0; i<faces.size(); ++i)
	{
		// upscale bounding box
		faces[i].set_bottom( faces[i].bottom() * detectFaceScale );
		faces[i].set_top( faces[i].top() * detectFaceScale );
		faces[i].set_left( faces[i].left() * detectFaceScale );
		faces[i].set_right( faces[i].right() * detectFaceScale );

		shapes.push_back( sp(cimg, faces[i]) );
	}

	if (shapes.size() > 0 )
		for(unsigned int i=0; i<shapes[0].num_parts(); i++)
			_points.push_back(
					cv::Point2f(
							float( shapes[0].part(i).x() ),
							float( shapes[0].part(i).y() )
					)
			);
}

//----------------------------------------------------------------------------------------------

void SNFaceShift::smoothPoints(std::vector<cv::Point2f>& inPoints, std::vector<cv::Point2f>& outPoints,
		std::vector<Median<glm::vec2>*>& _kf)
{
	// init filters
	if (kf.size() < inPoints.size() )
	{
		for (std::vector<cv::Point2f>::iterator it = inPoints.begin(); it != inPoints.end(); ++it)
		{
		    _kf.push_back( new Median<glm::vec2>(kfSmoothFact) );
		    _kf.back()->update(glm::vec2((*it).x, (*it).y));

		    lastInPoints.push_back( glm::vec2((*it).x, (*it).y) );
		    outPoints.push_back( cv::Point2f((*it).x, (*it).y) );
		}
	}


	for (std::vector<Median<glm::vec2>*>::iterator it = _kf.begin(); it != _kf.end(); ++it)
	{
		unsigned int pointNr = it - _kf.begin();

		if (inPoints.size() > 0)
		{
			(*it)->update( glm::vec2( inPoints[pointNr].x, inPoints[pointNr].y) );
			lastInPoints[pointNr] = glm::vec2( inPoints[pointNr].x, inPoints[pointNr].y );
		} else
			(*it)->update( lastInPoints[pointNr] );

		if (outPoints.size() > 0)
		{
			outPoints[pointNr].x = (*it)->get().x;
			outPoints[pointNr].y = (*it)->get().y;
		}
	}
}
*/
//----------------------------------------------------------------------------------------------

void SNFaceShift::onKey(int key, int scancode, int action, int mods)
{
	/*
	if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_1 :
                actDispMode = RAW;
                break;
            case GLFW_KEY_2 :
            	actDispMode = LANDMARKS;
            	break;
            case GLFW_KEY_3 :
            	actDispMode = SHIFT;
            	break;
            default:
                actDispMode = RAW;
            	break;
        }
    }
    */
}

//----------------------------------------------------------------------------------------------

SNFaceShift::~SNFaceShift()
{
	//delete quad;
}

}
