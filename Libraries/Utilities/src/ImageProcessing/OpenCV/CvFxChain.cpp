/*
 *  CvFxChain.cpp
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 07.07.12.
 *  Copyright 2012 Sven Hahne. All rights reserved.
 *
 *  opencv effekt chain
 *	holt sich direkt die pixel vom der KinectInput Klasse
 *	läuft als separater thread
 *
 *  possible effects: "blur",  "colorThreshold", "copyto",  "feedback", "flipH",
 *"flipV", "inv", "keyStone", "mask", "medianBlur", "morpho", "raw", "resize",
 *"subtr", "thres", "thres2", "undistort", "denoising"
 */

#ifdef ARA_USE_OPENCV

#include "CvFxChain.h"

namespace ara::cap {

CvFxChain::CvFxChain(std::vector<std::string> *_args, int _width, int _height, int _nrChannels, int _bits,
                     int _nrThreads)
    : height(_height), width(_width), nrChannels(_nrChannels), bits(_bits), nrThreads(_nrThreads), args(_args) {
    // make sure nrThreads is multiple of two
    nrThreads = nrThreads / 2 * 2;

    dWidth  = static_cast<float>(width);
    dHeight = static_cast<float>(height);

    imgSize  = cv::Size(width, height);

    // init frame
    cv_format = CV_8U + ((_bits / 16) * 2);
    frame     = new cv::Mat(height, width, CV_MAKETYPE(cv_format, nrChannels));
    fxChain   = new std::vector<FxOpts *>();
    dispSrc   = 0;

    for (std::vector<std::string>::iterator it = args->begin(); it != args->end(); ++it) {
        const char *routine = (*it).c_str();

        fxChain->push_back(new FxOpts());
        std::vector<FxOpts *>::iterator fxIt = fxChain->end();
        --fxIt;

        // ------ name ----------

        (*fxIt)->fxName = strdup((*it).c_str());
        if (std::strncmp(routine, "flipH", maxStringSize) == 0) {
            (*fxIt)->fxName = (char *)"flip";
            (*fxIt)->flipH  = 1;
        }
        if (std::strncmp(routine, "flipV", maxStringSize) == 0) {
            (*fxIt)->fxName = (char *)"flip";
            (*fxIt)->flipH  = 0;
        }
        if (std::strncmp(routine, "raw", maxStringSize) != 0) {
            (*fxIt)->fxMat = new cv::Mat(height, width, CV_MAKETYPE(CV_8U, nrChannels));
        } else {
            (*fxIt)->fxMat = frame;
        }

        // ------ auxMat ----------

        (*fxIt)->auxMat = NULL;
        // if ( std::strncmp( routine, "subtr", maxStringSize ) == 0 )
        // (*fxIt)->auxMat = &refpic_bw;
        if (std::strncmp(routine, "mask", maxStringSize) == 0) (*fxIt)->auxMat = &cam_mask_bw;
        if (std::strncmp(routine, "undistort", maxStringSize) == 0) {
            (*fxIt)->cameraMatrix = &cameraMatrix;
            (*fxIt)->distCoeffs   = &distCoeffs;
        }
        if (std::strncmp(routine, "morpho", maxStringSize) == 0) {
            // der letzte parameter scheint keine auswirkungen zu haben
            // je niedriger die ersten beiden werte, desto feiner die konturen.
            // (*fxIt)->auxMat = new cv::Mat(3,3,CV_8U,cv::Scalar(nrChannels));
        }
        if (std::strncmp(routine, "erosion", maxStringSize) == 0) {
            // der letzte parameter scheint keine auswirkungen zu haben je niedriger die ersten beiden werte, desto
            // feiner die konturen.
            int erosion_size = 10;
            element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                                                cv::Point(erosion_size, erosion_size));
            (*fxIt)->auxMat = &element;
        }
        if (std::strncmp(routine, "colorThreshold", maxStringSize) == 0 ||
            std::strncmp(routine, "feedback", maxStringSize) == 0) {
            (*fxIt)->auxMat = new cv::Mat(height, width, CV_MAKETYPE(CV_8U, nrChannels));
            (*fxIt)->cr     = &colPointers;
        }
        if (std::strncmp(routine, "keyStone", maxStringSize) == 0) {
            keyStoneDst.push_back(cv::Point2f(0.f, 0.f));
            keyStoneDst.push_back(cv::Point2f((float)width, 0.f));
            keyStoneDst.push_back(cv::Point2f((float)width, (float)height));
            keyStoneDst.push_back(cv::Point2f(0.f, (float)height));

            keyStoneSrc.push_back(cv::Point2f(0.f, 0.f));
            keyStoneSrc.push_back(cv::Point2f((float)width, 0.f));
            keyStoneSrc.push_back(cv::Point2f((float)width, (float)height));
            keyStoneSrc.push_back(cv::Point2f(0.f, (float)height));

            (*fxIt)->transmtx = new cv::Mat[nrThreads];
            // cv::getPerspectiveTransform(keyStoneSrc, keyStoneDst);
        }

        (*fxIt)->upSamp          = upSample;
        (*fxIt)->width           = static_cast<int>(width);
        (*fxIt)->height          = static_cast<int>(height);
        (*fxIt)->mtRows          = static_cast<int>(sqrt(static_cast<float>(nrThreads)));
        (*fxIt)->mtCols          = nrThreads / (*fxIt)->mtRows;
        (*fxIt)->upSampWidth     = static_cast<int>(static_cast<float>(width) * upSample.width);
        (*fxIt)->upSampHeight    = static_cast<int>(static_cast<float>(height) * upSample.height);
        (*fxIt)->upSamp          = cv::Size((*fxIt)->upSampWidth, (*fxIt)->upSampHeight);
        (*fxIt)->fbkSrc          = 0.5;  // new image
        (*fxIt)->fbkDst          = 0.7;  // old image
        (*fxIt)->thres1          = &thres1;
        (*fxIt)->medianBlurSize  = 5;
        (*fxIt)->cannyThresh     = 190;
        (*fxIt)->cannyKernelSize = 3;
    }

    //----- Threading init

    subPics    = new cv::Mat *[nrThreads];
    subOutPics = new cv::Mat *[nrThreads];
    subAuxPics = new cv::Mat *[nrThreads];
    sub_Thread = new std::thread *[nrThreads];

    inRois  = new cv::Rect *[nrThreads];
    outRois = new cv::Rect *[nrThreads];

    for (int i = 0; i < nrThreads; i++) {
        sub_Thread[i] = new std::thread;
        subPics[i]    = new cv::Mat;
        subOutPics[i] = new cv::Mat;  // resultat für berechnung in grau
        subAuxPics[i] = new cv::Mat;
    }

    result = NULL;
}

uint8_t *CvFxChain::procImg(uint8_t *img) {
    frame->data = img;
    doCvRendering();
    return result->data;
}

void CvFxChain::doCvRendering() {
    result   = frame;
    saveThis = result;

    // process fxChain
    mtx.lock();
    for (std::vector<FxOpts *>::iterator it = fxChain->begin(); it != fxChain->end(); ++it) {
        // hack keystone noch nicht threaded...
        if (std::strcmp((*it)->fxName, "keyStone") == 0) {
            transmtx = cv::getPerspectiveTransform(keyStoneSrc, keyStoneDst);
            cv::warpPerspective(*result, *result, transmtx, cv::Size(width, height));
        } else {
            for (int i = 0; i < nrThreads; i++) startSubThread(i, result, (*it));
            for (int i = 0; i < nrThreads; i++) {
                sub_Thread[i]->join();
                delete sub_Thread[i];
            }
            result = (*it)->fxMat;
        }
    }
    mtx.unlock();
}

void CvFxChain::imgProcess(int N, cv::Mat *inRoi, cv::Mat *outRoi, cv::Mat *auxMat, FxOpts *inOpts) {
    char *routine = inOpts->fxName;

    if (std::strncmp(routine, "blur", maxStringSize) == 0) {
        cv::blur(*inRoi, *outRoi, cv::Size(20, 20));

    } else if (std::strncmp(routine, "canny", maxStringSize) == 0) {
        cv::Canny(*inRoi, *outRoi, inOpts->cannyThresh, inOpts->cannyThresh * 2, inOpts->cannyKernelSize);

    } else if (std::strncmp(routine, "colorThreshold", maxStringSize) == 0) {
        cv::cvtColor(*inRoi, *auxMat, cv::COLOR_RGB2HSV);
        cv::inRange(
            *auxMat, cv::Scalar(inOpts->cr->colRange[0][0], inOpts->cr->colRange[1][0], inOpts->cr->colRange[2][0]),
            cv::Scalar(inOpts->cr->colRange[0][1], inOpts->cr->colRange[1][1], inOpts->cr->colRange[2][1]), *outRoi);
    } else if (std::strncmp(routine, "copyto", maxStringSize) == 0) {
        inRoi->copyTo(*outRoi);

    } else if (std::strncmp(routine, "denoising", maxStringSize) == 0) {
        cv::fastNlMeansDenoising(*inRoi, *outRoi, 3,  // h
                                 7,                   // templateWindowSize
                                 21);                 // searchWindowSize

    } else if (std::strncmp(routine, "erosion", maxStringSize) == 0) {
        cv::erode(*inRoi, *outRoi, *auxMat);

    } else if (std::strncmp(routine, "feedback", maxStringSize) == 0) {
        cv::addWeighted(*inRoi, inOpts->fbkSrc, *auxMat, inOpts->fbkDst, 0.0f, *outRoi);
        outRoi->copyTo(*auxMat);

    } else if (std::strncmp(routine, "flip", maxStringSize) == 0) {
        cv::flip(*inRoi, *outRoi, inOpts->flipH);

    } else if (std::strncmp(routine, "inv", maxStringSize) == 0) {
        bitwise_not(*inRoi, *outRoi);

    } else if (std::strncmp(routine, "keyStone", maxStringSize) == 0) {
        std::vector<cv::Point2f> thisKeyStoneSrc;
        std::vector<cv::Point2f> thisKeyStoneDst;

        for (int in = 0; in < 4; in++) {
            thisKeyStoneSrc.push_back(cv::Point2f(
                std::min(std::max(keyStoneSrc[in].x, (float)inRois[N]->x), (float)(inRois[N]->x + inRois[N]->width)),
                std::min(std::max(keyStoneSrc[in].y, (float)inRois[N]->y), (float)(inRois[N]->y + inRois[N]->height))));
            thisKeyStoneDst.push_back(cv::Point2f(
                std::min(std::max(keyStoneDst[in].x, (float)inRois[N]->x), (float)(inRois[N]->x + inRois[N]->width)),
                std::min(std::max(keyStoneDst[in].y, (float)inRois[N]->y), (float)(inRois[N]->y + inRois[N]->height))));
        }
        inOpts->transmtx[N] = cv::getPerspectiveTransform(thisKeyStoneSrc, thisKeyStoneDst);
        cv::warpPerspective(*inRoi, *outRoi, inOpts->transmtx[N], inRoi->size());

    } else if (std::strncmp(routine, "mask", maxStringSize) == 0) {
        cv::min(*inRoi, *auxMat, *outRoi);

    } else if (std::strncmp(routine, "medianBlur", maxStringSize) == 0) {
        cv::medianBlur(*inRoi, *outRoi, inOpts->medianBlurSize);

    } else if (std::strncmp(routine, "morpho", maxStringSize) == 0) {
        int     morph_elem = 1;
        int     morph_size = 9;
        cv::Mat melement   = cv::getStructuringElement(morph_elem, cv::Size(2 * morph_size + 1, 2 * morph_size + 1),
                                                       cv::Point(morph_size, morph_size));
        // cv::morphologyEx(*inRoi, *outRoi, cv::MORPH_CLOSE, melement,
        // cv::Point(-1,-1), 8); // bei hohen werten weniger cpu und gröbere
        // kanten

    } else if (std::strncmp(routine, "resize", maxStringSize) == 0) {
        cv::resize(*inRoi, *outRoi, cv::Size(outRoi->cols, outRoi->rows));

    } else if (std::strncmp(routine, "subtr", maxStringSize) == 0) {
        if (auxMat != 0) cv::subtract(*inRoi, *auxMat, *outRoi);

    } else if (std::strncmp(routine, "thres", maxStringSize) == 0) {
        cv::inRange(*inRoi, cv::Scalar(*inOpts->thres1), cv::Scalar(255), *outRoi);
        //		cv::inRange(*inRoi,
        //					cv::Scalar(*inOpts->thres1,
        //*inOpts->thres1, *inOpts->thres1), 					cv::Scalar(255, 255, 255),
        //*outRoi
        //					);
    } else if (std::strncmp(routine, "thres2", maxStringSize) == 0) {
        cv::inRange(*inRoi, cv::Scalar(maxStringSize), cv::Scalar(255), *outRoi);

    } else if (std::strncmp(routine, "undistort", maxStringSize) == 0) {
        cv::undistort(*inRoi, *outRoi, *inOpts->cameraMatrix, *inOpts->distCoeffs, *inOpts->cameraMatrix);
    }
}

void CvFxChain::startSubThread(int N, cv::Mat *inMat, FxOpts *inOpts) {
    int count = 0;
    for (int y = 0; y < inOpts->mtRows; y++) {
        for (int x = 0; x < inOpts->mtCols; x++) {
            inRois[count] = new cv::Rect(inMat->cols / inOpts->mtCols * x, inMat->rows / inOpts->mtRows * y,
                                         inMat->cols / inOpts->mtCols, inMat->rows / inOpts->mtRows);

            if (std::strncmp(inOpts->fxName, "flip", maxStringSize) != 0) {
                outRois[count] =
                    new cv::Rect(inOpts->fxMat->cols / inOpts->mtCols * x, inOpts->fxMat->rows / inOpts->mtRows * y,
                                 inOpts->fxMat->cols / inOpts->mtCols, inOpts->fxMat->rows / inOpts->mtRows);
            } else {
                if (inOpts->flipH == 1) {
                    outRois[count] =
                        new cv::Rect(inOpts->fxMat->cols / inOpts->mtCols * (inOpts->mtCols - x - 1),
                                     inOpts->fxMat->rows / inOpts->mtRows * y, inOpts->fxMat->cols / inOpts->mtCols,
                                     inOpts->fxMat->rows / inOpts->mtRows);
                } else {
                    outRois[count] =
                        new cv::Rect(inOpts->fxMat->cols / inOpts->mtCols * x,
                                     inOpts->fxMat->rows / inOpts->mtRows * (inOpts->mtRows - y - 1),
                                     inOpts->fxMat->cols / inOpts->mtCols, inOpts->fxMat->rows / inOpts->mtRows);
                }
            }
            count++;
        }
    }

    *subPics[N]    = cv::Mat(*inMat, *inRois[N]);
    *subOutPics[N] = cv::Mat(*inOpts->fxMat, *outRois[N]);
    if (inOpts->auxMat != NULL && (std::strncmp(inOpts->fxName, "morpho", maxStringSize) != 0))
        *subAuxPics[N] = cv::Mat(*inOpts->auxMat, *inRois[N]);

    sub_Thread[N] = new std::thread(&CvFxChain::imgProcess, this, N, subPics[N], subOutPics[N], subAuxPics[N], inOpts);
}

/*
void CvFxChain::onKey(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            // reset colors
            case GLFW_KEY_R:
                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 2; j++)
                        colPointers.colRange[i][j] = -1;
                break;

                // save new ref pic
            case GLFW_KEY_S:
                cv::imwrite("/projekte/programmierung/tav/data/refpic.jpg",
                            *saveThis);
                std::cout << "save new pic" << std::endl;
                refpic =
cv::imread("/projekte/programmierung/tav/data/refpic.jpg"); cv::cvtColor(refpic,
refpic_bw, cv::COLOR_BGR2GRAY); break;

                // set threshold
            case GLFW_KEY_Y:
                thres1 -= 1;
                std::cout << "thres1 :" << thres1 << std::endl;
                break;

            case GLFW_KEY_A:
                thres1 += 1;
                std::cout << "thres1 :" << thres1 << std::endl;
                break;

                // select source to display
            case 93: // key +
                if ((dispSrc + 1) < fxChain->size())
                    dispSrc += 1;
                std::cout << "displaying src nr: " << dispSrc << ": "
                          << fxChain->at(dispSrc)->fxName << std::endl;
                break;

            case 92:   // key #
                if (dispSrc > 0)
                    dispSrc -= 1;
                std::cout << "displaying src nr: " << dispSrc << ": "
                          << fxChain->at(dispSrc)->fxName << std::endl;
                break;
        }
    }
}*/

std::vector<FxOpts *> *CvFxChain::getFxChain() { return fxChain; }

int CvFxChain::getWidth() { return static_cast<int>(width); }

int CvFxChain::getHeight() { return static_cast<int>(height); }

uint8_t *CvFxChain::getImg() { return fxChain->at(dispSrc)->fxMat->data; }

cv::Mat *CvFxChain::getFinalImg() { return result; }

uint8_t *CvFxChain::getFinalImgData() { return result->data; }

void CvFxChain::setFdbkAmtSrc(double _fdbkAmt) {
    int ind = 0;
    for (std::vector<std::string>::iterator it = args->begin(); it != args->end(); ++it) {
        if (std::strcmp((*it).c_str(), "feedback") == 0) fxChain->at(ind)->fbkSrc = _fdbkAmt;
        ind++;
    }
}

void CvFxChain::setFdbkAmtDst(double _fdbkAmt) {
    int ind = 0;
    for (std::vector<std::string>::iterator it = args->begin(); it != args->end(); ++it) {
        if (std::strcmp((*it).c_str(), "feedback") == 0) fxChain->at(ind)->fbkDst = _fdbkAmt;
        ind++;
    }
}

void CvFxChain::setKeyStoneH(float _amt) {
    keyStoneSrc[0].x = std::min(_amt, 0.f) * -1.f * (float)width;   // left down corner
    keyStoneSrc[1].x = (1.f + std::min(_amt, 0.f)) * (float)width;  // right down corner
    keyStoneSrc[2].x = (1.f - std::max(_amt, 0.f)) * (float)width;  // right up corner
    keyStoneSrc[3].x = std::max(_amt, 0.f) * (float)width;          // left up corner
}

void CvFxChain::setKeyStoneV(float _amt) {
    // positiv heisst rechts quetschen
    keyStoneSrc[0].y = std::min(_amt, 0.f) * -1.f * (float)height;   // left down corner
    keyStoneSrc[1].y = std::max(_amt, 0.f) * (float)height;          // right down corner
    keyStoneSrc[2].y = (1.f - std::max(_amt, 0.f)) * (float)height;  // right up corner
    keyStoneSrc[3].y = (1.f + std::min(_amt, 0.f)) * (float)height;  // left up corner
}

void CvFxChain::setThresh1(int _val) { thres1 = _val; }

void CvFxChain::setSubtrMat(cv::Mat *_subtrMat) {
    for (std::vector<FxOpts *>::iterator it = fxChain->begin(); it != fxChain->end(); ++it)
        if (std::strcmp((*it)->fxName, "subtr") == 0) (*it)->auxMat = _subtrMat;
}

CvFxChain::~CvFxChain() {
    delete element5;

    delete frame;
    fxChain->clear();
    delete fxChain;

    for (int i = 0; i < nrThreads; i++) {
        delete sub_Thread[i];
        delete subPics[i];
        delete subOutPics[i];
        delete subAuxPics[i];
    }
    delete subPics;
    delete subOutPics;
    delete subAuxPics;
}

}  // namespace ara::cap

#endif