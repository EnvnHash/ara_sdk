//
//  3DNoiseTexgen.h
//
//  Created by Sven Hahne on 18/5/15.
//

#pragma once

#include <GLUtils/sceneData.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/FBO.h>
#include <glb_common/glb_common.h>

namespace ara {
class Noise3DTexGen {
public:
    Noise3DTexGen(sceneData* _scd, bool color, int nrOctaves, int _width, int _height, int _depth, float _scaleX,
                  float _scaleY, float _scaleZ);
    ~Noise3DTexGen();
    void          initShdr();
    ara::Shaders* initBlendShdrH();
    ara::Shaders* initBlendShdrV();
    GLuint        getTex();

    FBO* fbo;

private:
    Shaders*         noiseShdr;
    Shaders*         stdTexShdr;
    ShaderCollector* shCol;
    GLBase*          m_glbase = nullptr;

    float scaleX;
    float scaleY;
    float scaleZ;

    int   width;
    int   height;
    int   depth;
    int   nrLoops;
    short nrParallelTex;
};
}  // namespace ara
