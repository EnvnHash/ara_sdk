//
// SNIkpiMallPlaza.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNIkpiMallPlaza.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

namespace tav
{
SNIkpiMallPlaza::SNIkpiMallPlaza(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), nrStreams(3), procWidth(300), procHeight(300),
		texs(NULL), timeToDelTrackObjs(1.0), maxFaceDiff(0.22), nrBufferFrames(4),
		streamTimeOut(5000), capRetryInt(1000)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	quad = static_cast<Quad*>(_scd->stdHFlipQuad);

	stdTex = shCol->getStdTex();
	stdCol = shCol->getStdParCol();

	system("/home/sven/start_mallplaza.sh");

	// ------- config i/o --------------------------------------

	configFileName = (*scd->dataPath)+"config/ikpi_mallplaza.yml";

	// check if calibration file exists, if this is the case load it
	if (access(configFileName.c_str(), F_OK) != -1)
		loadConfig();

	// DEBUGGING !!!! clear db
	clearDB();

	// ---------------------------------------------

	csvFileName = (*_scd->dataPath)+"/test_ikpi_mall_plaza_db.csv";

	// use opencv to capture the streams
    texs = new TextureManager*[nrStreams+1];
    //ft = new FreetypeTex(((*_scd->dataPath)+"/fonts/Arial.ttf").c_str(), 20);

    mSize = cv::Size(procWidth, procHeight);

    // generate 4 matrices to fill the screen
    pos_mat = new glm::mat4[nrStreams+1];
    float x_pos[] = {  0.5f, -0.5f,  0.5f, -0.5f };
    float y_pos[] = { -0.5f,  0.5f,  0.5f, -0.5f };

    for (int i=0;i<nrStreams+1;i++)
    {
    	texs[i] = NULL;
        pos_mat[i] = glm::mat4(1.f);
        pos_mat[i] = glm::translate(pos_mat[i], glm::vec3(x_pos[i], y_pos[i], 0.f));
        pos_mat[i] = glm::scale(pos_mat[i], glm::vec3(0.5, 0.5, 1.0));
    }


    // start capture streams
    cvThreadDatas = new cvThreadData[nrStreams+1];
	cap_thread = new std::thread*[nrStreams+1];
    last_upload_frame = new int[nrStreams+1];

    for (int i=0; i<nrStreams+1; i++)
	{
    	last_upload_frame[i] = -1;

		cvThreadDatas[i].frames = new cv::Mat[nrBufferFrames];
		cvThreadDatas[i].id = i;
		cvThreadDatas[i].requestReopen_tp = std::chrono::system_clock::now();
		cvThreadDatas[i].last_update_time = std::chrono::system_clock::now();

		cap_thread[i] = new std::thread(&SNIkpiMallPlaza::getCvFrame, this,
				&cvThreadDatas[i]);
		cap_thread[i]->detach();
		std::cout << "cap_thread[i]->native_handle() " << cap_thread[i]->native_handle() << endl;

		usleep(500);
	}


	// face detection
    String modelConfiguration = (*_scd->dataPath)+"/dnn/deploy.prototxt";
    String modelBinary = (*_scd->dataPath)+"/dnn/res10_300x300_ssd_iter_140000_fp16.caffemodel";

    //! [Initialize network]
    net = readNetFromCaffe(modelConfiguration, modelBinary);

    detectFaceThread = std::thread(&SNIkpiMallPlaza::detectFace, this);
    detectFaceThread.detach();

    faces = std::vector<glm::vec4>(maxNrFaces);


    cout << "setup done start" << endl;
    run = true;
}

//------------------------------------------

void SNIkpiMallPlaza::openStream(cv::VideoCapture* cap, std::string file)
{

}

//------------------------------------------

void SNIkpiMallPlaza::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);


	stdTex->begin();
	stdTex->setUniform1i("tex", 0);

    for (int i=0;i<nrStreams+1;i++)
    {
    	if (texs[i] && texs[i]->getId() >= 0)
    	{
        	stdTex->setUniformMatrix4fv("m_pvm", &pos_mat[i][0][0]);
        	glBindTexture(GL_TEXTURE_2D, texs[i]->getId());
            quad->draw();
    	}
    }


    stdCol->begin();
    stdCol->setUniform4f("color", 1.f, 0.f, 0.f, 0.4f);

	//mtx.lock();
	for (int i=0;i<trackedFaces.size();i++)
	{
		glm::mat4 quad_mat = glm::mat4(1.f);

		glm::vec3 transF = glm::vec3(trackedFaces[i].getPos()->x, trackedFaces[i].getPos()->y, 0.f);
		quad_mat = glm::translate(quad_mat, transF);

		glm::vec3 scaleF = glm::vec3(trackedFaces[i].getSize().x, trackedFaces[i].getSize().y, 1.f);
		quad_mat = glm::scale(quad_mat, scaleF);

		// multiply with screen seg matrix
		quad_mat = pos_mat[nrStreams] * quad_mat;

		stdCol->setUniformMatrix4fv("m_pvm", &quad_mat[0][0]);

		quad->draw(_tfo);
	}
//	mtx.unlock();


	//_shader->begin();
}

//------------------------------------------

void SNIkpiMallPlaza::getCvFrame(cvThreadData* _data)
{
	_data->cap = getVideoCap(_data->id);

	while (run)
	{
		// if the stream is not open, open it
		if (_data->cap && _data->cap->isOpened())
		{
			//if (_data->id != nrStreams)
			//	std::cout << "capture frame " << endl;

			(_data->frame_nr)++;
			_data->frame_ptr = (_data->frame_ptr +1) % nrBufferFrames;
			*_data->cap >> _data->frames[_data->frame_ptr];

			if(_data->frames[_data->frame_ptr].empty())
			{
				cout << "cam returned empty frame !!!!" << endl;
				_data->emptyCount++;

				if (_data->emptyCount > 3)
				{
					cout << "cam returned more than 3 empty frames, aborting !!!!" << endl;
					_data->cap->release();
					delete _data->cap;
					_data->cap = NULL;
					_data->didStart = false;
				}
			} else {
				_data->last_update_time = std::chrono::system_clock::now();
				_data->didStart = true;
			}

		} else if (_data->cap && !_data->cap->isOpened())
		{
			auto now = std::chrono::system_clock::now();
			auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _data->requestReopen_tp);
			int diff_msecs = (int)diff_ms.count();

			if (diff_msecs > capRetryInt)
			{
				cout << "capture is closed, trying to reopen" << endl;

				_data->didStart = false;
				_data->requestReopen_tp = std::chrono::system_clock::now();
				_data->cap = getVideoCap(_data->id);
				_data->emptyCount = 0;

				cout << "trying to reopen done" << endl;
			}

		} else if (!_data->cap)
		{
			cout << "reopening cam..." << endl;

			_data->didStart = false;
			_data->requestReopen_tp = std::chrono::system_clock::now();
			_data->cap = getVideoCap(_data->id);
			_data->emptyCount = 0;
		}
	}

	std::cout << "thread exited" << std::endl;
}

//---------------------------------------------------------------

std::string SNIkpiMallPlaza::exec(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
        result += buffer.data();
    }
    return result;
}

//------------------------------------------

cv::VideoCapture* SNIkpiMallPlaza::getVideoCap(int id)
{
	if (id < nrStreams){
		return new cv::VideoCapture(stream_urls[id]);
	} else {

		if( strcmp(stream_urls[id].c_str(), "/dev/video") == 0 )
		{
			// get the first available device from /dev/video*
			std::string act_video = exec("ls /dev/video*");

			if (act_video.size() > 0){
				act_video.pop_back();
				return new cv::VideoCapture(act_video);
			} else {
				return NULL;
			}
		} else {
			return new cv::VideoCapture(stream_urls[id]);
		}
	}
}

//------------------------------------------

void SNIkpiMallPlaza::detectFace()
{
	Mat inputBlob;
	const double inScaleFactor = 1.0;
	const Scalar meanVal(104.0, 177.0, 123.0);
	float confidenceThreshold = 0.5;

	while (true)
	{
		if (gotNewFrame)
		{
			faces.clear();
			processing = true;

			int thisFramePtr = (cvThreadDatas[nrStreams].frame_ptr -1 +nrBufferFrames) % nrBufferFrames;

			if (!cvThreadDatas[nrStreams].frames[thisFramePtr].empty())
			{
		//		cv::resize (frame[nrStreams], faceFrame, mSize);

		        inputBlob = blobFromImage(faceFrame, inScaleFactor, mSize, meanVal, false, false); //Convert Mat to batch of images
		        net.setInput(inputBlob, "data"); //set the network input
		        Mat detection = net.forward("detection_out"); //compute output

		        Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

		        for(int i=0; i<detectionMat.rows; i++)
		        {
		            float confidence = detectionMat.at<float>(i, 2);

		            if(confidence > confidenceThreshold)
		            {
		                float xLeftBottom = detectionMat.at<float>(i, 3) * 2.f - 1.f;
		                float yLeftBottom = (1.f - detectionMat.at<float>(i, 4)) * 2.f - 1.f;
		                float xRightTop = detectionMat.at<float>(i, 5) * 2.f - 1.f;
		                float yRightTop = (1.f - detectionMat.at<float>(i, 6)) * 2.f - 1.f;

		                //faces.push_back(glm::vec4(xLeftBottom, yLeftBottom, xRightTop, yRightTop));

		                faces.push_back(glm::vec4(
		                		(xRightTop - xLeftBottom) * 0.5f + xLeftBottom,
		                		(yRightTop - yLeftBottom) * 0.5f + yLeftBottom,
		                		std::fabs(xRightTop - xLeftBottom) * 0.5f,
		                		std::fabs(yRightTop - yLeftBottom) * 0.5f));

		               // printf("[%d]: %f %f %f %f \n\n", faces.size(), xLeftBottom, yLeftBottom, xRightTop, yRightTop);
		            }
		        }
			}

			trackFaces(glfwGetTime());

			processing = false;
			gotNewFrame = false;

		} else {
			usleep(1000);
		}
	}
}

//------------------------------------------

bool SNIkpiMallPlaza::removeDoubleAssignments(int* mapNewToOld, std::map<float, int>* diffNewOld)
{
	bool gotDoubleAssignment = false;

	// if we got more new faces than old faces, there must be multiple assignments
	// check for the lowest diff and remove all other assignments
	for (size_t i=0; i<faces.size(); i++)
	{
		if (mapNewToOld[i] != -1)
		{
			int thisMapInd = mapNewToOld[i];
			int newMapInd = thisMapInd;
			int searchInd = i+1;
			float lowestDiff = std::numeric_limits<float>::max();
			float actDiff = (*diffNewOld[i].begin()).first;
		//	cout << "assignment would be: new[" << i << "] to old [" << mapNewToOld[i] << "], diff: " << actDiff << endl;

			// check in the rest of the array for same index
			while(searchInd < faces.size())
			{
				if (mapNewToOld[searchInd] == thisMapInd && actDiff > (*diffNewOld[searchInd].begin()).first)
				{
					actDiff = (*diffNewOld[searchInd].begin()).first;
				//	cout << " found lower diff: " << actDiff << endl;
					newMapInd = searchInd;
				}
				searchInd++;
			}

		//	cout << "after double check : new[" << i << "] to old [" << newMapInd << "], removing other assignments" << endl;

			// remove other assigments
			thisMapInd = mapNewToOld[i];
			searchInd = i;
			while(searchInd < faces.size())
			{
				if (mapNewToOld[searchInd] == thisMapInd && actDiff < (*diffNewOld[searchInd].begin()).first)
				{
				//	cout << "removing assignment at index " << searchInd << endl;
					gotDoubleAssignment = true;
					mapNewToOld[searchInd] = -1;
				}
				searchInd++;
			}
		}
	}

	return gotDoubleAssignment;
}

//------------------------------------------

void SNIkpiMallPlaza::correctAssignment(int* mapNewToOld, std::map<float, int>* diffNewOld,
		float* finalDiffNewToOld )
{
//	cout << "correcting Assignment" << endl;

	for (size_t i=0; i<faces.size(); i++)
	{
		// take the wrong assignments
		if (mapNewToOld[i] == -1)
		{
			bool couldFix = false;
			std::map<float, int>::iterator searchIt = diffNewOld[i].begin()++;
			while(searchIt != diffNewOld[i].end())
			{
				// check if the next lowest diff for this actual face is already used
				bool found = false;
				int mapSearchInd = 0;
				while (mapSearchInd < faces.size() && !found)
				{
					if(mapSearchInd != i && (*searchIt).second == mapNewToOld[mapSearchInd])
						found = true;

					mapSearchInd++;
				}

				// if no double assignment was found, stop the search and assign the new index
				if (!found)
				{
					mapNewToOld[i] = (*searchIt).second;
					finalDiffNewToOld[i] = (*searchIt).first;
					couldFix = true;
					break;
				}
				++searchIt;
			}

			if (!couldFix){
				printf(" Error couldn't assign the face \n");
			}
		}
	}
}

//------------------------------------------

void SNIkpiMallPlaza::addFace(int ind, double time)
{
	trackedFaces.push_back(TrackObj());
	trackedFaces.back().setOn();
	trackedFaces.back().setState(TrackObj::trackState::ON);
	trackedFaces.back().setPos(faces[ind].x, faces[ind].y);
	trackedFaces.back().setSize(faces[ind].z, faces[ind].w);
	trackedFaces.back().setUpdtTime(time);
}

//------------------------------------------

void SNIkpiMallPlaza::updateFace(int old, int newInd, double time, float* finalDiffNewToOld )
{
	//cout << "update, diff: " << finalDiffNewToOld[newInd] << endl;

	if (finalDiffNewToOld[newInd] < maxFaceDiff)
	{
		trackedFaces[ old ].setPos(faces[newInd].x, faces[newInd].y);
		trackedFaces[ old ].setSize(faces[newInd].z, faces[newInd].w);
		trackedFaces[ old ].setUpdtTime(time);
	} else
	{
	//	cout << "didn't update because of too high diff, creating a new face instead" << endl;
		addFace(newInd, time);
	}
}

//------------------------------------------

void SNIkpiMallPlaza::trackFaces(double time)
{
	std::map<float, int> diffNewOld[faces.size()];
	int mapNewToOld[faces.size()];
	float finalDiffNewToOld[faces.size()];

	// for every actual face calculate the differences to the old tracked Faces
	// the difference is between vec4s with (x, y, width, height)
	for (size_t i=0; i<faces.size(); i++)
	{
		for (size_t j=0; j<trackedFaces.size(); j++)
			diffNewOld[i][glm::length(faces[i] - glm::vec4(trackedFaces[j].getPos()->x, trackedFaces[j].getPos()->y,
															trackedFaces[j].getSize().x, trackedFaces[j].getSize().y))] = j;

		// find the lowest difference and assign this index as the corresponding old tracked face to this face
		mapNewToOld[i] = (*diffNewOld[i].begin()).second;
		finalDiffNewToOld[i] = (*diffNewOld[i].begin()).first;
	}


	// now do the mapping to the old values, we distinguish between, no old faces, more new than old,
	// exactly same amount of new as old, and less new than old

	// if we dont have any old faces, assign the new ones straight away
	if (trackedFaces.size() == 0)
		for (size_t i=0; i<faces.size(); i++)
			addFace(i, time);


	// let's be sure that we don't have double assignments
	bool gotDoubleAssigments = removeDoubleAssignments(mapNewToOld, diffNewOld);

	if(faces.size() > trackedFaces.size())
	{
		// now check again for actual faces without assignment and add just straight away
		// if we dont have any old faces, assign the new ones straight away
		for (size_t i=0; i<faces.size(); i++)
		{
			// if we dont have any old faces, assign the new ones straight away
			// if the assignment is valid update the values
			if (mapNewToOld[i] == -1) 	addFace(i, time);
			else						updateFace(mapNewToOld[i], i, time, finalDiffNewToOld);
		}

	} else if(faces.size() == trackedFaces.size())
	{
		if (gotDoubleAssigments)
			correctAssignment(mapNewToOld, diffNewOld,finalDiffNewToOld);

		// update all valid assignments
		for (size_t i=0; i<faces.size(); i++)
			if (mapNewToOld[i] != -1) updateFace(mapNewToOld[i], i, time, finalDiffNewToOld);

	} else if(faces.size() < trackedFaces.size())
	{
		if (gotDoubleAssigments)
			correctAssignment(mapNewToOld, diffNewOld, finalDiffNewToOld);

		// update all valid assignments
		for (size_t i=0; i<faces.size(); i++)
			if (mapNewToOld[i] != -1) updateFace(mapNewToOld[i], i, time, finalDiffNewToOld);

/*
		// kill dead faces if necessary
		vector< vector<TrackObj>::iterator > toKill;

		// check all unmapped indices
		for (int i=0; i<trackedFaces.size(); i++)
		{
			bool found = false;
			int searchInd = 0;

			// check if the index is in mapNewToOld
			while (!found && searchInd < faces.size())
			{
				if(mapNewToOld[searchInd] == i) found = true;
				searchInd++;
			}

			// if we don't have an assignment, check if we need to delete this TrackObj
			if (!found && (time - trackedFaces[i].getLastUpdtTime()) > timeToDelTrackObjs){
				toKill.push_back(trackedFaces.begin() + i);
			}
		}

		for (vector< vector<TrackObj>::iterator >::iterator it = toKill.begin(); it != toKill.end(); ++it)
			trackedFaces.erase(*it);
			*/
	}



	// check if we need to delete faces that haven't been update since more than threshold
	vector< vector<TrackObj>::iterator > toKill;
	for (int i=0; i<trackedFaces.size(); i++)
		if ((time - trackedFaces[i].getLastUpdtTime()) > timeToDelTrackObjs)
			toKill.push_back(trackedFaces.begin() + i);

	for (vector< vector<TrackObj>::iterator >::iterator it = toKill.begin(); it != toKill.end(); ++it)
	{
		//cout << "deleting, writing csv" << endl;

		(*(*it)).setOff();

		// write to csv
	//	writeCSV(csvFileName, &(*(*it)));
		setOnset(&(*(*it)));

		trackedFaces.erase(*it);
	}


//	cout << "trackedFaces.size(): " << trackedFaces.size() << endl;
//	cout << endl;
}

//------------------------------------------

void SNIkpiMallPlaza::update(double time, double dt)
{
	int nrThreadsReady=0;
	auto now = std::chrono::system_clock::now();

	for (int i=0; i<nrStreams+1; i++)
	{
		int thisFramePtr = (cvThreadDatas[i].frame_ptr -1 +nrBufferFrames) % nrBufferFrames;

		// check if stream is alive
		auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - cvThreadDatas[i].last_update_time);
		if(cvThreadDatas[i].didStart && (int)diff_ms.count() > streamTimeOut )
		{
			std::cout << "stream[" << i << "] is dead" << endl;
			std::cout << "releasing the cap object " << endl;
			cvThreadDatas[i].emptyCount = 100;
			cvThreadDatas[i].cap->release();
			//delete cvThreadDatas[i].cap;
			//cvThreadDatas[i].cap = NULL;

		}


		if(cvThreadDatas[i].frames[thisFramePtr].rows != 0
				&& cvThreadDatas[i].frames[thisFramePtr].cols != 0
				&& cvThreadDatas[i].frame_nr != last_upload_frame[i])
		{
			// upload to gl
			if(!texs[i])
			{
				texs[i] = new TextureManager();
				texs[i]->allocate(cvThreadDatas[i].frames[thisFramePtr].cols,
						cvThreadDatas[i].frames[thisFramePtr].rows, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D);
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texs[i]->getId());
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cvThreadDatas[i].frames[thisFramePtr].cols,
					cvThreadDatas[i].frames[thisFramePtr].rows, GL_BGR, GL_UNSIGNED_BYTE,
					cvThreadDatas[i].frames[thisFramePtr].data);

			if (i == nrStreams && !processing)
			{
				cv::resize(cvThreadDatas[i].frames[thisFramePtr], faceFrame, mSize);
				gotNewFrame = true;
			}

			last_upload_frame[i] = cvThreadDatas[i].frame_nr;
		}
	}


	/*
	// update fonts
	if (lastNrFaces != (int)trackedFaces.size())
	{
		if (ft) {
			delete ft;
		}

	    ft = new FreetypeTex(((*scd->dataPath)+"/fonts/Arial.ttf").c_str(), 20);
		ft->setText("Active Faces: ");
		//lastNrFaces = (int)trackedFaces.size();
	}
	*/
}

//------------------------------------------------------------------------------

bool SNIkpiMallPlaza::file_exists(const std::string& name)
{
	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

//------------------------------------------------------------------------------

void SNIkpiMallPlaza::writeCSV(string filename, TrackObj* obj)
{
	if (!file_exists(filename))
	{
		ofstream log(filename.c_str(), std::ios_base::app | std::ios_base::out);
		log << "ID" << ";" << "InTime" << ";" << "OutTime" << ";" << "AreaMinX" << ";" << "AreaMaxX" << ";" << "AreaMinY" << ";" << "AreaMaxY" << std::endl;
		log.close();
	}

	// date strings come with newline at the end -> remove it!!
	std::string onDate = obj->getOnDateStr();
	onDate.pop_back();

	std::string offDate = obj->getOffDateStr();
	offDate.pop_back();

	ofstream log(filename.c_str(), std::ios_base::app | std::ios_base::out);
	log << std::to_string(obj->getUniqueID()) << ";";
	log << onDate << ";" << offDate << ";";
	log << obj->getMoveArea()->x << ";" << obj->getMoveArea()->y << ";" << obj->getMoveArea()->z << ";" << obj->getMoveArea()->w << std::endl;
	log.close();
}

//-------------------------------------------------------

MYSQL* SNIkpiMallPlaza::mysql_connection_setup(struct connection_details mysql_details)
{
     // first of all create a mysql instance and initialize the variables within
    MYSQL *connection = mysql_init(NULL);

    // connect to the database with the details attached.
    if (!mysql_real_connect(connection,mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database, 0, NULL, 0)) {
      printf("Conection error : %s\n", mysql_error(connection));
      exit(1);
    }
    return connection;
}

//-------------------------------------------------------

MYSQL_RES* SNIkpiMallPlaza::mysql_perform_query(MYSQL *connection, string sql_query)
{
   // send the query to the database
   if (mysql_query(connection, sql_query.c_str()))
   {
      printf("MySQL query error : %s\n", mysql_error(connection));
      exit(1);
   }

   return mysql_use_result(connection);
}

//-------------------------------------------------------

void SNIkpiMallPlaza::clearDB()
{
	cout << " ---> try to clear db" << endl;

	MYSQL *conn;      // the connection
	MYSQL_RES *res;   // the results
	MYSQL_ROW row;    // the results row (line by line)

	// connect to the mysql database
	conn = mysql_connection_setup(mysqlD);

	// assign the results return to the MYSQL_RES pointer
	res = mysql_perform_query(conn, "USE "+m_mysql_db);
	if (res != 0){
		cout << "ERROR: USE " << m_mysql_db << " failed" << endl;
	}

	string q = "TRUNCATE TABLE `"+m_mysql_table+"`;";

	res = mysql_perform_query(conn, q);
	if (res != 0){
		cout << "ERROR: USE " << m_mysql_db << " failed" << endl;
	}

	// clean up the database result set
	mysql_free_result(res);

	// clean up the database link
	mysql_close(conn);

	dataBaseAccessInProcess = false;

	cout << " ---> db cleared" << endl;
}

//-------------------------------------------------------

void SNIkpiMallPlaza::setOnset(TrackObj* obj)
{
//	cout << " ---> set onset" << endl;

	try
	{
		MYSQL *conn;      // the connection
		MYSQL_RES *res;   // the results
		MYSQL_ROW row;    // the results row (line by line)

		// connect to the mysql database
		conn = mysql_connection_setup(mysqlD);

		// assign the results return to the MYSQL_RES pointer
		res = mysql_perform_query(conn, "USE "+m_mysql_db);
		if (res != 0){
			cout << "ERROR: USE " << m_mysql_db << " failed" << endl;
		}

		// get onset time string
		std::time_t t = std::chrono::system_clock::to_time_t(obj->getOnDate());
		std::tm now_tm = *std::localtime(&t);
		char buf[80];
		strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &now_tm);
		string onsetDateStr = string(buf);
		//onsetDateStr.pop_back();

		// get onset time string
		std::time_t ot = std::chrono::system_clock::to_time_t(obj->getOffDate());
		std::tm onow_tm = *std::localtime(&ot);
		char obuf[80];
		strftime(obuf, sizeof obuf, "%Y-%m-%d %H:%M:%S", &onow_tm);
		string offsetDateStr = string(obuf);
		//offsetDateStr.pop_back();

		int onTime = (int)(obj->getTotalOntime() );

		if (onTime > 3)
		{
			string q = "INSERT INTO `"+m_mysql_table+"` (`id`, `in_time`, `out_time`, `duration`) ";
			q += "VALUES ('"+std::to_string(obj->getUniqueID())+"','"+onsetDateStr+"','"+offsetDateStr+"','"+std::to_string(onTime)+"');";

			cout << "q: " << q << endl;
			res = mysql_perform_query(conn, q);
			if (res != 0){
				cout << "ERROR: USE " << m_mysql_db << " failed" << endl;
			}

			// clean up the database result set
			mysql_free_result(res);
		} else {
		//	std::cout << "result too short" << std::endl;
		}

		// clean up the database link
		mysql_close(conn);

	} catch ( FFError e )
	{
		printf("%s\n", e.Label.c_str());
	}

	dataBaseAccessInProcess = false;

//	cout << " ---> set onset done " << endl;
}

//--------------------------------------------------------------------------------

void SNIkpiMallPlaza::loadConfig()
{
	printf("loading config \n");
	cv::FileStorage fs(configFileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
	    fs["mysql_url"] >> m_mysql_url;
	    if (std::strlen(m_mysql_url.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong mysql_url definition!\n");
	    	exit(1);
	    }

	    fs["mysql_user"] >> m_mysql_user;
	    if (std::strlen(m_mysql_user.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong mysql_user definition!\n");
	    	exit(1);
	    }

	    fs["mysql_pw"] >> m_mysql_pw;
	    if (std::strlen(m_mysql_pw.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong mysql_pw definition!\n");
	    	exit(1);
	    }

	    fs["mysql_db"] >> m_mysql_db;
	    if (std::strlen(m_mysql_db.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong mysql_db definition!\n");
	    	exit(1);
	    }

	    fs["mysql_table"] >> m_mysql_table;
	    if (std::strlen(m_mysql_pw.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong mysql_table definition!\n");
	    	exit(1);
	    }

	    fs["stream1_url"] >> m_stream1_url;
	    if (std::strlen(m_stream1_url.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong m_stream1_url definition!\n");
	    	exit(1);
	    }

	    fs["stream2_url"] >> m_stream2_url;
	    if (std::strlen(m_stream2_url.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong m_stream2_url definition!\n");
	    	exit(1);
	    }

	    fs["stream3_url"] >> m_stream3_url;
	    if (std::strlen(m_stream3_url.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong m_stream3_url definition!\n");
	    	exit(1);
	    }

	    fs["stream4_url"] >> m_stream4_url;
	    if (std::strlen(m_stream4_url.c_str()) == 0){
	    	fprintf(stderr, " ERROR reading config file, no or wrong m_stream4_url definition!\n");
	    	exit(1);
	    }

	    stream_urls = new std::string[4];
	    stream_urls[0] = m_stream1_url;
	    stream_urls[1] = m_stream2_url;
	    stream_urls[2] = m_stream3_url;
	    stream_urls[3] = m_stream4_url;
	}

	cout << "mysql_url: " << m_mysql_url << endl;
	cout << "mysql_user: " << m_mysql_user << endl;
	cout << "mysql_pw: " << m_mysql_pw << endl;
	cout << "mysql_db: " << m_mysql_db << endl;
	cout << "mysql_table: " << m_mysql_table << endl;
	cout << "stream1_url: " << m_stream1_url << endl;
	cout << "stream2_url: " << m_stream2_url << endl;
	cout << "stream3_url: " << m_stream3_url << endl;
	cout << "stream4_url: " << m_stream4_url << endl;

	mysqlD.server = m_mysql_url.c_str();  // where the mysql database is
	mysqlD.user =  m_mysql_user.c_str();     // the root user of mysql
	mysqlD.password = m_mysql_pw.c_str(); // the password of the root user in mysql
	mysqlD.database = m_mysql_db.c_str();    // the databse to pick
}

//----------------------------------------------------

SNIkpiMallPlaza::~SNIkpiMallPlaza()
{
	delete quad;
	delete ft;
	delete cap_thread;
	delete stream_urls;
    delete [] last_upload_frame;
    delete [] cvThreadDatas;
}
}
