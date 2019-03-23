/*
 *  TFO.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 25.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved..
 *
 *  Transformation Feedback Object Wrapper
 */

#ifndef __tav_core_TFO__
#define __tav_core_TFO__

#pragma once

#include <algorithm>
#include <ctime>
#include <string>
#include <utility>
#include <vector>
#include "headers/gl_header.h"
#include "headers/global_vars.h"
#include "headers/tav_types.h"

namespace tav
{
class TFO
{
public:

	TFO(int _bufSize, std::vector<std::string>& _parNames);
	TFO();
	~TFO();
	void init();
	void bind(bool _resetCounters = true);
	void unbind();
	void setVaryingsToRecord(std::vector<std::string>* names, GLuint _prog);
	void setVaryingsToRecordInternNames(GLuint _prog);
	void begin(GLenum _mode);
	void pause();
	void resume();
	void end();
	void clear();
	void offsetBuf();
	void pauseAndOffsetBuf(GLenum _mode);
	void incCounters(int nrVertices);
	void decCounters(int nrVertices);
	void draw(GLenum _mode, TFO* _tfo, GLenum _recToMode, int geoAmpAmt);
	void draw(GLenum _mode, GLuint offset, GLuint count, TFO* _tfo,
			GLenum _recToMode, int geoAmpAmt);
	void setIndices(GLuint _count, GLenum _type, GLvoid* _indices);
	void drawElements(GLenum _mode, GLuint _count, GLenum _type, TFO* _tfo,
			GLenum _recToMode, int geoAmpAmt);
	void drawElementsBaseVertex(GLenum _mode, GLuint _count, GLenum _type,
			GLint basevertex, TFO* _tfo, GLenum _recToMode, int geoAmpAmt);

	void resizeTFOBuf(coordType _nr, unsigned int size);
	GLuint getTFOBuf(coordType _nr);
	unsigned int getTFOBufFragSize(coordType _nr);
	GLuint getTFOBufSize(coordType _nr);
	GLuint getId();
	GLuint getTbo(coordType _coord);
	GLuint getRecVertOffs();
	GLuint getTotalPrimWritten();
	GLuint getBufSize();
	GLuint getVao();
	glm::vec4 getColor(int ind);
	GLenum getLastMode();

	void setBlendMode(GLenum _srcMode, GLenum _dstMode);
	void setSceneNodeColors(glm::vec4* _cols);
	void setSceneProtoName(std::string* _protoName);
	void setVaryingsNames(std::vector<std::string>& _names);
	void setRecNrVert(int _nr);
	void setRecVertOffs(int _nr);
	void setObjOffset();
	std::vector<std::pair<unsigned int, unsigned int> >* getObjOffsets();
	void resetObjOffset();
	void incRecVertOffs(int _nr);
	void addTexture(int _unit, int _texInd, GLenum _target, std::string _name);
	void enableDepthTest();
	void disableDepthTest();
	void recallDepthTestState();
	void recallBlendMode();

	GLuint primitivesWritten = 0;
	GLuint totalPrimsWritten = 0;
	GLuint bufSize;
	std::vector<auxTexPar> textures;

private:
	GLuint tfo;
	GLuint* buffers;
	GLuint* bufferSizes;
	GLuint* tbos;
	GLuint resVAO;
	GLuint resElementBuffer = 0;
	GLuint resElementBufferNrIndices = 0;
	GLuint resElementBufferSize;

	GLfloat* statColor;

	int recNrVert = 0;
	int recVertOffs = 0;
	unsigned int nrPar;
	std::vector<std::string> parNames;
	std::vector<std::string> varyingsNames;
	std::vector<unsigned int> fragSizes;
	std::vector<unsigned int> locations;

	std::vector<std::pair<unsigned int, unsigned int> > objOffset;

	bool depthTest = true;
	GLenum srcMode;
	GLenum dstMode;
	glm::vec4* recColors;
	std::string* recProtoName;
	GLenum lastMode;
};
}
#endif /* defined(__Timeart Visualizer_core__UniformBlock__) */

