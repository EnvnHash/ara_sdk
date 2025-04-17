//
// Created by user on 30.03.2022.
//

#pragma once

#include <DataModel/Item.h>
#include <DataModel/ItemRef.h>
#include <Property.h>

namespace ara {

class GLSLChromaKeyPar : public ItemRef, public ItemUi {
public:
    GLSLChromaKeyPar();
    virtual ~GLSLChromaKeyPar() = default;

    PropertyPtr<bool>        freeze;
    PropertyPtr<bool>        chromaKey;
    PropertyPtr<bool>        removeSpill;
    PropertyPtr<bool>        showMask;
    PropertyPtr<bool>        showHue;
    PropertyPtr<bool>        showSat;
    PropertyPtr<bool>        procHisto;
    PropertyPtr<bool>        backImg;
    PropertyPtr<std::string> ndiInIP;
    PropertyPtr<int>         width;
    PropertyPtr<int>         height;
    PropertyPtr<int>         ndiColorFmt;
    PropertyPtr<float>       blurIt;

    PropertyPtr<bool> actMask1;
    PropertyPtr<bool> actMask2;

    PropertyPtr<float> keyCol_r;
    PropertyPtr<float> keyCol_g;
    PropertyPtr<float> keyCol_b;

    PropertyPtr<float> keyCol2_r;
    PropertyPtr<float> keyCol2_g;
    PropertyPtr<float> keyCol2_b;

    PropertyPtr<float> range_x;
    PropertyPtr<float> range_y;

    PropertyPtr<float> range2_x;
    PropertyPtr<float> range2_y;

    PropertyPtr<float> spillRange_x;
    PropertyPtr<float> spillRange_y;

    PropertyPtr<float> maskGamma;
    PropertyPtr<float> maskGamma2;
    PropertyPtr<float> maskBlurSize;
    PropertyPtr<float> satu;

    PropertyPtr<float> maskCut_x;
    PropertyPtr<float> maskCut_y;
};

}  // namespace ara