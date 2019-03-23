//
// SNLogoBlobAnimation.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//	der osc befehl zum "klicken" eines icons ist "click", iconNr
//	in der update-methode wird gecheckt ob ein update gefordert wurde
//
//	im vao der partikel gibt es POSITION, AUX0 und AUX1
//  AUX0: x: clickTime + TimeDur (<0 when inactive) y: clickDuration z: icon Nr a: alpha


#define STRINGIFY(A) #A

#include "SNLogoBlobAnimation.h"

using namespace cv;
using namespace std;

namespace tav
{
SNLogoBlobAnimation::SNLogoBlobAnimation(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
{

	tex = new TextureManager();
	tex->loadTexture2D((*_scd->dataPath+"/textures/camchal/logo_jornadas2.tga"));

	img = cv::imread((*_scd->dataPath)+"/textures/camchal/logo_jornadas_w.png", cv::IMREAD_COLOR);
	if (img.rows*img.cols <= 0){
		std::cout << "Image " << (*_scd->dataPath)+"/textures/camchal/logo_jornadas_w.png" << " is empty or cannot be found\n" << std::endl;
	}
	imgDim = cv::Point2f( img.cols, img.rows );

	initContours();
	initShader();

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	stdTex = shCol->getStdTex();

	imgProp = imgDim.x / imgDim.y;

	addPar("blendPos", &blendPos);
	addPar("posRandAmt", &posRandAmt);
	addPar("posRandSpd", &posRandSpd);
	addPar("alpha", &alpha);

	randPhase = new float[2];
	randPhase[0] = 0.f;
	randPhase[1] = 0.f;
}



void SNLogoBlobAnimation::initShader()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(
			layout(location = 0) in vec4 position;
	layout(location = 1) in vec4 normal;
	layout(location = 2) in vec2 texCoord;
	layout(location = 3) in vec4 color;

	uniform mat4 m_pvm;
	uniform vec2 texTrans;
	uniform vec2 texScale;
	uniform vec3 noiseOffs;
	uniform sampler3D noise;
	uniform float time;
	uniform float blendPos;

	out tex_inf
	{
		vec2 rawTexCoord;
		vec2 texCoord;
	} vertex_out;

	void main() {

		vertex_out.rawTexCoord = texCoord;
		vertex_out.texCoord = (texCoord * texScale) + texTrans;
		vec4 offsPos = texture(noise, vec3(texTrans.x + time, texTrans.y + time * 0.23, 0.3) + noiseOffs);
		gl_Position = m_pvm * mix(position, position + vec4(offsPos.xy * 16.0, 0.0, 0.0), blendPos);
	});

	vert = "// SNLogoBlobAnimation base vertex Shader\n" + shdr_Header + vert;

	

	std::string frag = STRINGIFY(
			layout (location = 0) out vec4 color;
	uniform sampler2D tex;
	uniform sampler2D mask;
	uniform float alpha;

	in tex_inf {
		vec2 rawTexCoord;
		vec2 texCoord;
	} vertex_in;

	void main() {
		vec4 mCol = texture(mask, vec2(vertex_in.rawTexCoord.x, 1.0 - vertex_in.rawTexCoord.y));
		color = texture(tex, vertex_in.texCoord);
		color.a *= mCol.r * alpha;
	});

	frag = "// SNLogoBlobAnimation base frag Shader\n" + shdr_Header + frag;


	shdr = shCol->addCheckShaderText("SNLogoBlobAnimation", vert.c_str(), frag.c_str());
}



void SNLogoBlobAnimation::initContours()
{
	Mat src_gray;
	int thresh = 231;
	int max_thresh = 255;
	RNG rng(12345);

	/// Convert image to gray and blur it
	cvtColor( img, src_gray, COLOR_BGR2GRAY );
	blur( src_gray, src_gray, Size(3,3) );

	cv::Mat threshold_output;
	std::vector<cv::Vec4i> hierarchy;

	/// Detect edges using Threshold
	cv::threshold( src_gray, threshold_output, thresh, 255, THRESH_BINARY );

	/// Find contours
	cv::findContours( threshold_output, contours, hierarchy,
			RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );

	/// Approximate contours to polygons + get bounding rects and circles
	vector<float>radius( contours.size() );
	boundRect.resize( contours.size() );
	std::vector<cv::Point2f> center( contours.size() );
	contours_poly.resize( contours.size() );


	for( std::size_t i = 0; i < contours.size(); i++ )
	{
		cv::approxPolyDP( Mat(contours[i]), contours_poly[i], 0, true );
		boundRect[i] = boundingRect( Mat(contours_poly[i]) );
		cv::minEnclosingCircle( contours_poly[i], center[i], radius[i] );

		cv::Point2f center = cv::Point2f(boundRect[i].x + boundRect[i].width / 2.f,
				boundRect[i].y + boundRect[i].height / 2.f);
		logoFrags.push_back( logoFrag() );

		// position entspricht der verschiebung von 0|0 auf die center position, skaliert auf -1|1
		logoFrags.back().center = glm::vec3((center.x / imgDim.x) * 2.f -1.f,
				-((center.y / imgDim.y) * 2.f -1.f),
				0.f);

		logoFrags.back().texTrans = glm::vec2((center.x - boundRect[i].width /2) / imgDim.x,
				1.0 - ((center.y + boundRect[i].height /2) / imgDim.y));

		// groesse in koordinaten system -1 | 1, wie muss man das quad (w:2|h:2) skalieren,
		// damit es diesselbe Groesse wie der Teil des Logos hat?
		logoFrags.back().scale = glm::vec3((boundRect[i].width / imgDim.x),
				(boundRect[i].height / imgDim.y),
				1.f);

		logoFrags.back().texScale = glm::vec2(boundRect[i].width / imgDim.x,
				boundRect[i].height / imgDim.y);

		// define offset pos
		logoFrags.back().transCenter = glm::vec3(frand(), frand(), 0.f);
		logoFrags.back().transCenter = glm::vec3((sfrand() > 0.f ? -1.f : 1.f) * (logoFrags.back().transCenter.x + 1.f),
												  (sfrand() > 0.f ? -1.f : 1.f) * (logoFrags.back().transCenter.y + 1.f),
												  0.f);


		// make mask, first draw into a Mat same size as the original
		cv::Mat drawing = Mat::zeros( img.size(), CV_8UC3 );
		cv::Scalar color(255, 255, 255);
		drawContours( drawing, contours_poly, (int)i, color, CV_FILLED, 8, vector<Vec4i>(), 0, Point() );

		// then copy the specific subpart
		cv::Mat sub = drawing( boundRect[i] );
		sub.copyTo( logoFrags.back().shape );


		// convert cv::Mat to opengl Texture
		glGenTextures(1, &logoFrags.back().texNr);   // could be more then one, but for now, just one
		glBindTexture(GL_TEXTURE_2D, logoFrags.back().texNr);

		glTexStorage2D(GL_TEXTURE_2D,
				1,                 // nr of mipmap levels
				GL_RGB8,
				boundRect[i].width,
				boundRect[i].height);

		// das hier super wichtig!!!
		glPixelStorei(GL_UNPACK_ALIGNMENT, (logoFrags.back().shape.step & 3) ? 1 : 4);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, logoFrags.back().shape.step / logoFrags.back().shape.elemSize());

		glTexSubImage2D(GL_TEXTURE_2D,             // target
				0,                          // mipmap level
				0,0,
				boundRect[i].width,
				boundRect[i].height,
				GL_BGR,
				GL_UNSIGNED_BYTE,
				logoFrags.back().shape.ptr());
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}



void SNLogoBlobAnimation::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glm::mat4 m_pvm;
	glm::mat4 model;

	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if(!inited)
	{
		noiseTex = new Noise3DTexGen(shCol,
				true, 4,
				256, 256, 64,
				4.f, 4.f, 16.f);

		inited = true;
	}

	float blendPos = blendPos;

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);

	shdr->begin();
	shdr->setUniform1i("tex", 0);
	shdr->setUniform1i("mask", 1);
	shdr->setUniform1i("noise", 2);
	shdr->setUniform1f("blendPos", blendPos);
	shdr->setUniform1f("alpha", alpha);

	tex->bind(0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());


	// render 2x
	for( int j=0; j<2; j++ )
	{
		shdr->setUniform1f("time", time * 0.02 + float(j * 0.2f));
		shdr->setUniform3f("noiseOffs", float(j) * 0.43f, float(j) * 0.14f, float(j) * 0.23f);
		randPhase[j] += dt * posRandSpd * (float(j) * 0.3f + 1.f);


		// draw elements, not the first one, since it seems to be always the whole image
		for( std::size_t i=1; i<contours.size(); i++ )
		{
			glm::mat4 propScale = glm::scale(glm::mat4(1.f), glm::vec3(0.1f, 0.1f, 1.f));

			glm::vec3 randPos = glm::vec3(glm::perlin( glm::vec4(randPhase[j] + logoFrags[i].center.x, logoFrags[i].center.y, 0.21f, 0.f) ),
										  glm::perlin( glm::vec4(randPhase[j] * 1.8f + logoFrags[i].center.x, logoFrags[i].center.y + 0.4f, 0.f, 0.f) ),
										  0.f);

			model = glm::translate(glm::mat4(1.f), glm::mix(logoFrags[i].center, logoFrags[i].transCenter, blendPos)
													+ randPos * (posRandAmt * (float(j) * 0.23f + 1.f)));
			model = glm::scale(model, glm::mix(logoFrags[i].scale, glm::vec3(0.f, 0.f, 1.f), blendPos));

			m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat
					* glm::scale(glm::mat4(1.f), glm::vec3(imgProp, 1.f, 1.f)) * model;

			shdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);

			shdr->setUniform2fv("texTrans", &logoFrags[i].texTrans[0]);
			shdr->setUniform2fv("texScale", &logoFrags[i].texScale[0]);

			//logoFrags[i].tex->bind(1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, logoFrags[i].texNr );

			// berechne die textur koordinaten
			quad->draw();
		}
	}
}



void SNLogoBlobAnimation::update(double time, double dt)
{}



SNLogoBlobAnimation::~SNLogoBlobAnimation()
{ }

}
