/*
 *  CvFxChain.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 07.07.12.
 *  Copyright 2012 Sven Hahne. All rights reserved.
 *
 */

#ifdef ARA_USE_OPENCV

#pragma once

#include "glm/include/glm/glm.hpp"
#include "opencv_headers.h"
#include "util_common.h"

namespace ara::cap {

struct ColorRange {
    int                      colRange[3][2];  // hsv
    int                      keyCodes[3][2][2];
    std::vector<std::string> name;
};

class FxOpts {
public:
    FxOpts() = default;

    ~FxOpts();

    int  cannyThresh;
    int  cannyKernelSize;
    int  flipH;
    int  width;
    int  height;
    int  medianBlurSize;
    int  mtRows;
    int  mtCols;
    int *thres1;
    int  upSampWidth;
    int  upSampHeight;

    double fbkSrc;
    double fbkDst;

    char *fxName;

    cv::Size upSamp;

    cv::Mat *auxMat       = nullptr;
    cv::Mat *cameraMatrix = nullptr;
    cv::Mat *distCoeffs   = nullptr;
    cv::Mat *fxMat        = nullptr;

    cv::Mat *transmtx = nullptr;

    ColorRange *cr;
};

class CvFxChain {
public:
    CvFxChain(std::vector<std::string> *args, int _width, int _height, int _nrChannels, int _bits = 8,
              int _nrThreads = 4);

    ~CvFxChain();

    uint8_t *procImg(uint8_t *img);

    void startSubThread(int N, cv::Mat *inMat, FxOpts *inOpts);
    void imgProcess(int N, cv::Mat *inRoi, cv::Mat *outRoi, cv::Mat *auxMat, FxOpts *inOpts);
    void doCvRendering();
    // void onKey(int key, int scancode, int action, int mods);

    // getter, setter
    std::vector<FxOpts *> *getFxChain();

    uint8_t *getImg();
    cv::Mat *getFinalImg();
    uint8_t *getFinalImgData();
    uint8_t *getSilImg();

    int getWidth();
    int getHeight();
    int getMaxNumSegments();
    int getMaxNumShapes();
    int getNrOfContours();
    int getContourSizeAt(int index);

    glm::vec2 getContourPointAt(int contourNr, int pointNr);

    glm::vec2 getActDir(int contourNr, int pointNr);

    glm::vec2 getActMidPoint(int ind);

    float getActVel(int ind);
    void  drawNIInfo();

    std::vector<std::vector<glm::vec2> > *getShapes();

    float getScaleX();
    float getScaleY();
    float getOffsX();
    float getOffsY();
    void  setFdbkAmtSrc(double _fdbkAmt);
    void  setFdbkAmtDst(double _fdbkAmt);
    void  setKeyStoneH(float _amt);
    void  setKeyStoneV(float _amt);
    void  setThresh1(int _val);
    void  setSubtrMat(cv::Mat *_subtrMat);

    cv::Mat *frame;
    cv::Mat *result;
    cv::Mat *saveThis;
    cv::Mat  cam_mask;
    cv::Mat  cam_mask_bw;
    cv::Mat  element;

    cv::Point center;

    double dWidth;
    double dHeight;
    int    width;
    int    height;
    int    nrChannels;
    int    bits;

    int dispSrc;

    std::mutex mtx;

private:
    bool threaded;

    int scrHeight;
    int scrWidth;
    int nrThreads;
    int thres1;
    int maxStringSize = 15;
    int cv_format     = 0;

    std::vector<FxOpts *>    *fxChain;
    std::vector<std::string> *args;

    cv::Size upSample;
    cv::Size imgSize;
    cv::Mat  map1, map2;

    cv::Mat  cameraMatrix;
    cv::Mat  distCoeffs;
    cv::Mat  refpic;
    cv::Mat  refpic_bw;
    cv::Mat *element5;

    cv::Mat  **subPics;
    cv::Mat  **subOutPics;
    cv::Mat  **subAuxPics;
    cv::Rect **inRois;
    cv::Rect **outRois;

    int pos;

    bool *doUpdate;
    bool *keystates;

    std::thread **sub_Thread;

    ColorRange  colPointers;
    std::string rootPath;

    cv::Mat transmtx;

    std::vector<cv::Point2f> keyStoneSrc;
    std::vector<cv::Point2f> keyStoneDst;
};
}  // namespace ara::cap

#endif