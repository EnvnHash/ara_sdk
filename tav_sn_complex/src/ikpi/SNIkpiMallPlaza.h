//
// SNIkpiMallPlaza.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <array>
#include <cstdio>
#include <iostream>
#include <limits>
#include <mutex>
#include <memory>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include <signal.h>

#include <opencv2/dnn.hpp>

#include <SceneNode.h>
#include "GeoPrimitives/Quad.h"
//#include <FFMpegDecode.h>
#include <headers/opencv_headers.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/Typo/FreetypeTex.h>

#include "../../tav_tracking/src/TrackObj.h"
#include <GLFW/glfw3.h>

#include <mysql/mysql.h>


namespace tav
{

class FFError
{
public:
	std::string    Label;
	FFError( ) { Label = (char *)"Generic Error"; }

	FFError( char *message ) { Label = message; }
	~FFError() { }
	inline const char*   GetMessage  ( void )   { return Label.c_str(); }

};

class SNIkpiMallPlaza: public SceneNode
{
public:

	struct connection_details
	{
	    const char *server;
	    const char *user;
	    const char *password;
	    const char *database;
	};
	struct connection_details mysqlD;

	typedef struct {
		cv::VideoCapture* cap=NULL;
		cv::Mat* frames=NULL;
		int frame_ptr=0;
		int frame_nr=-1;
		int id = 0;
		std::chrono::time_point<std::chrono::system_clock> last_update_time;
		std::chrono::time_point<std::chrono::system_clock> requestReopen_tp;
		int lastFrame = -1;
		int emptyCount = 0;
		bool requestReopen = false;
		bool didStart = false;
	} cvThreadData;

	SNIkpiMallPlaza(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNIkpiMallPlaza();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods) {}

	bool removeDoubleAssignments(int* mapNewToOld, std::map<float, int>* diffNewOld);
	void correctAssignment(int* mapNewToOld, std::map<float, int>* diffNewOld, float* finalDiffNewToOld);
	void addFace(int ind, double time);
	void updateFace(int old, int newInd, double time, float* finalDiffNewToOld);
	void trackFaces(double time);

	void openStream(cv::VideoCapture* cap, std::string file);
	std::string exec(const char* cmd);
	void getCvFrame(cvThreadData* _data);
	cv::VideoCapture* getVideoCap(int id);
	void detectFace();

	void clearDB();

	void writeCSV(std::string filename, TrackObj* obj);
	inline bool file_exists (const std::string& name);

	MYSQL* mysql_connection_setup(struct connection_details mysql_details);
	MYSQL_RES* mysql_perform_query(MYSQL *connection, std::string sql_query);
	void setOnset(TrackObj* obj);

	void loadConfig();

private:
	Shaders*				stdTex;
	Shaders*				stdCol;
	ShaderCollector*		shCol;
	//FFMpegDecode*		ffmpeg;
	Quad*					quad;
    glm::mat4*          	pos_mat;
    TextureManager**		texs;
    FreetypeTex*			ft;
    cv::Size 				mSize;
    std::vector<glm::vec4> 	faces;

    bool					run=false;
	bool					isInited = false;
	bool					processing = false;
	bool					gotNewFrame = false;
	bool					dataBaseAccessInProcess = false;

	int						capRetryInt;
	int 					maxNrFaces = 20;
	int						streamTimeOut;
    int                 	nrStreams;
    int						procWidth, procHeight;
    int 					lastNrFaces = -1;
    int						nrBufferFrames;

    int*					last_upload_frame;


    unsigned int 			totNrTrackedVisitors = 0;

    float					alpha = 0.f;

    double					timeToDelTrackObjs;
    double					maxFaceDiff;

    cv::Mat					faceFrame;
    cvThreadData*			cvThreadDatas;
    std::string				csvFileName;
    std::thread				detectFaceThread;

    //boost::mutex*			cap_thread_mtx;
	std::thread**			cap_thread;

    std::string 			m_mysql_url;
    std::string 			m_mysql_user;
    std::string 			m_mysql_pw;
    std::string 			m_mysql_db;
    std::string 			m_mysql_table;

    std::string 			m_stream1_url;
    std::string 			m_stream2_url;
    std::string 			m_stream3_url;
    std::string 			m_stream4_url;
    std::string* 			stream_urls;

    std::string				configFileName;

    cv::dnn::Net 			net;

    std::vector<TrackObj>	trackedFaces;
};
}
