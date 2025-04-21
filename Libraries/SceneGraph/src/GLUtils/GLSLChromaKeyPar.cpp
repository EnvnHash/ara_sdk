//
// Created by user on 30.03.2022.
//

#include "GLUtils/GLSLChromaKeyPar.h"

using namespace std;

namespace ara {

GLSLChromaKeyPar::GLSLChromaKeyPar() : ItemUi() {
    setTypeName<GLSLChromaKeyPar>();
    displayName = "ChromaKey";

    freeze = addProp<bool>("freeze", "Freeze");
    freeze = false;

    chromaKey = addProp<bool>("chromaKey", "Chroma-Key");
    chromaKey = true;

    showMask = addProp<bool>("showMask", "Show Mask");
    showMask = false;

    showHue = addProp<bool>("showHue", "Show Cb");
    showHue = false;

    showSat = addProp<bool>("showSat", "Show Cr");
    showSat = false;

    backImg = addProp<bool>("backImg", "Show Bg Img");
    backImg = false;

    procHisto = addProp<bool>("procHisto", "AutoCol");
    procHisto = true;

    // ndiInIP = addProp<std::string>("ndiIp", "NDI In IP");
    // ndiInIP = "172.17.0.1:5961";

    //------------------------------------------------------------------------------------------------------------------

    actMask1 = addProp<bool>("actMask1", "Enable Mask1");
    actMask1 = true;

    keyCol_r = addProp<float>("keyCol_r", "KeyCol R");
    keyCol_r.setMinMax(0.f, 255.f);
    keyCol_r.setStep(1.f);
    keyCol_r = 0.f;

    keyCol_g = addProp<float>("keyCol_g", "KeyCol G");
    keyCol_g.setStep(1.f);
    keyCol_g.setMinMax(0.f, 255.f);
    keyCol_g = 255.f;

    keyCol_b = addProp<float>("keyCol_b", "KeyCol B");
    keyCol_b.setStep(1.f);
    keyCol_b.setMinMax(0.f, 255.f);
    keyCol_b = 0.f;

    range_x = addProp<float>("range_x", "ColRange Min");
    range_x.setMinMax(0.f, 1.f);
    range_x.setStep(0.001f);
    range_x = 0.054f;

    range_y = addProp<float>("range_y", "ColRange Max");
    range_y.setMinMax(0.f, 0.5f);
    range_y.setStep(0.001f);
    range_y = 0.159f;

    maskGamma = addProp<float>("maskGamma", "Mask Gamma");
    maskGamma.setMinMax(0.f, 2.f);
    maskGamma.setStep(0.010f);
    maskGamma = 0.87f;

    //------------------------------------------------------------------------------------------------------------------

    actMask2 = addProp<bool>("actMask2", "Enable Mask2");
    actMask2 = true;

    keyCol2_r = addProp<float>("keyCol2_r", "KeyCol2 R");
    keyCol2_r.setMinMax(0.f, 255.f);
    keyCol2_r.setStep(1.f);
    keyCol2_r = 65.f;

    keyCol2_g = addProp<float>("keyCol2_g", "KeyCol2 G");
    keyCol2_g.setStep(1.f);
    keyCol2_g.setMinMax(0.f, 255.f);
    keyCol2_g = 117.f;

    keyCol2_b = addProp<float>("keyCol2_b", "KeyCol2 B");
    keyCol2_b.setStep(1.f);
    keyCol2_b.setMinMax(0.f, 255.f);
    keyCol2_b = 43.f;

    range2_x = addProp<float>("range2_x", "ColRange2 Min");
    range2_x.setMinMax(0.f, 1.f);
    range2_x.setStep(0.001f);
    range2_x = 0.054f;

    range2_y = addProp<float>("range2_y", "ColRange2 Max");
    range2_y.setMinMax(0.f, 0.5f);
    range2_y.setStep(0.001f);
    range2_y = 0.25f;

    maskGamma2 = addProp<float>("maskGamma2", "Mask2 Gamma");
    maskGamma2.setMinMax(0.f, 2.f);
    maskGamma2.setStep(0.010f);
    maskGamma2 = 0.64f;

    //------------------------------------------------------------------------------------------------------------------

    removeSpill = addProp<bool>("removeSpill", "Remove Spill");
    removeSpill = false;

    spillRange_x = addProp<float>("spillMin", "Spill Min");
    spillRange_x.setMinMax(0.f, 1.f);
    spillRange_x.setStep(0.01f);
    spillRange_x = 0.12f;

    spillRange_y = addProp<float>("spillMax", "Spill Max");
    spillRange_y.setMinMax(0.f, 1.f);
    spillRange_y.setStep(0.01f);
    spillRange_y = 0.29f;

    satu = addProp<float>("satu", "Saturation");
    satu.setMinMax(0.f, 2.f);
    satu.setStep(0.010f);
    satu = 1.18f;

    //------------------------------------------------------------------------------------------------------------------

    maskBlurSize = addProp<float>("maskBlurSize", "Mask Blur Size");
    maskBlurSize.setMinMax(0.f, 1.5f);
    maskBlurSize.setStep(0.010f);
    maskBlurSize = 1.f;

    blurIt = addProp<float>("maskBlurIt", "Mask Blur Iter");
    blurIt.setMinMax(1.f, 10.f);
    blurIt.setStep(1.f);
    blurIt = 1.f;

    maskCut_x = addProp<float>("maskCutX", "Mask Cut Low");
    maskCut_x.setMinMax(0.f, 1.f);
    maskCut_x.setStep(0.010f);
    maskCut_x = 0.16f;

    maskCut_y = addProp<float>("maskCutX", "Mask Cut High");
    maskCut_y.setMinMax(0.f, 1.f);
    maskCut_y.setStep(0.010f);
    maskCut_y = 1.f;

    //------------------------------------------------------------------------------------------------------------------

    width = addProp<int>("strWidth", "Width", false);
    width = 3840;

    height = addProp<int>("strHeight", "Height", false);
    height = 2160;

    ndiColorFmt = addProp<int>("colorFmt", "ColorFmt", false);
    ndiColorFmt.setMinMax(0, 100);
    ndiColorFmt.setStep(1);
}

}  // namespace ara
