//
// SSAO.h
//
//  Created by Sven Hahne on 19.08.14.
//

#pragma once

#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderUtils/ShaderBuffer.h>
#include <Utils/FBO.h>
#include <Utils/Geometry.h>
#include <Utils/MersenneTwister.h>
#include <Utils/Texture.h>
#include <Utils/Vertex.h>
#include <glb_common/glb_common.h>

#include "GLUtils/Noise3DTexGen.h"
#include "SceneNodes/SceneNode.h"

// optimizes blur, by storing depth along with ssao calculation
// avoids accessing two different textures
#define USE_AO_SPECIALBLUR 1

// optimizes the cache-aware technique by rendering all temporary layers at once
// instead of individually
#define AO_LAYERED_OFF 0
#define AO_LAYERED_IMAGE 1
#define AO_LAYERED_GS 2
#define AO_RANDOMTEX_SIZE 4
#define USE_AO_LAYERED_SINGLEPASS AO_LAYERED_GS
#define NV_BUFFER_OFFSET(i) ((char*)NULL + (i))

namespace ara {

class SSAO {
public:
    static const int  NUM_MRT              = 8;
    static const uint HBAO_RANDOM_SIZE     = AO_RANDOMTEX_SIZE;
    static const uint HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE;
    static const uint MAX_SAMPLES          = 8;

    enum AlgorithmType {
        ALGORITHM_NONE = 0,
        ALGORITHM_HBAO_CACHEAWARE,
        ALGORITHM_HBAO_CLASSIC,
        NUM_ALGORITHMS,
    };

    struct ssaoFbos {
        GLuint scene, depthlinear, viewnormal, hbao_calc, hbao2_deinterleave, hbao2_calc;

        ssaoFbos() : scene(0), depthlinear(0), viewnormal(0), hbao_calc(0), hbao2_deinterleave(0), hbao2_calc(0) {}
    };

    struct ssaoTextures {
        GLuint scene_color, scene_depthstencil, scene_depthlinear, scene_viewnormal, hbao_result, hbao_blur,
            hbao_random, hbao_randomview[MAX_SAMPLES], hbao2_deptharray, hbao2_depthview[HBAO_RANDOM_ELEMENTS],
            hbao2_resultarray;

        // ara::Texture
        //	hbao_random;

        ssaoTextures()
            : scene_color(0), scene_depthstencil(0), scene_depthlinear(0), scene_viewnormal(0), hbao_result(0),
              hbao_blur(0), hbao_random(0), hbao2_deptharray(0), hbao2_resultarray(0) {
            for (auto i = 0; i < MAX_SAMPLES; i++) hbao_randomview[i] = 0;

            for (auto i = 0; i < HBAO_RANDOM_ELEMENTS; i++) hbao2_depthview[i] = 0;
        }
    };

    // corresponds to the uniform block
    struct HBAOData {
        float RadiusToScreen;  // radius
        float R2;              // 1/radius
        float NegInvR2;        // radius * radius
        float NDotVBias;

        glm::vec2 InvFullResolution;
        glm::vec2 InvQuarterResolution;

        float     AOMultiplier;
        float     PowExponent;
        glm::vec2 _pad0;

        glm::vec4 projInfo;
        glm::vec2 projScale;
        int       projOrtho;
        int       _pad1;

        glm::vec4 float2Offsets[AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE];
        glm::vec4 jitters[AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE];
    };

    SSAO(GLBase* glbase, uint inFboWidth, uint inFboHeight, AlgorithmType _algorithm = ALGORITHM_HBAO_CACHEAWARE,
         bool _blur = true, float _intensity = 8.f, float _blurSharpness = 30.0f);
    ~SSAO();

    void bind();
    void clear(glm::vec4 clearCol = glm::vec4(0.f));
    void unbind();
    void copyFbo(CameraSet* cs);
    void proc(CameraSet* cs);
    void drawBlit(CameraSet* cs, bool copyDepth = false);
    void drawAlpha(CameraSet* cs, float alpha);
    void blitDepthBuffer(CameraSet* cs);
    void        initShaders();
    void        initFullScrQuad();
    void        initBilateralblur();
    std::string initDepthLinearize(int msaa);
    void        initViewNormal();
    std::string initHbaoCalc(int _blur, int deinterl);
    std::string initHbaoBlur(int _blur);
    void        initDeinterleave();
    Shaders*    initDebugDepth();
    std::string initReinterleave(int _blur);
    GLuint      createShaderPipelineProgram(GLuint target, const char* src);
    void prepareHbaoData(CameraSet* cs, int width, int height);
    void drawLinearDepth(CameraSet* cs, int width, int height, int sampleIdx);
    void drawHbaoBlur(int width, int height, int sampleIdx);
    void drawHbaoClassic(CameraSet* cs, int width, int height, int sampleIdx);
    void drawHbaoCacheAware(CameraSet* cs, int width, int height, int sampleIdx);
    bool initMisc();
    bool initFramebuffers(int width, int height, int samples);

    void newBuffer(GLuint& glid) {
        if (glid) {
            glDeleteBuffers(1, &glid);
        }
        glGenBuffers(1, &glid);
    }

    void newTexture(GLuint& glid) {
        if (glid) {
#ifndef ARA_USE_GLES31
            glInvalidateTexImage(glid, 0);
#endif
            glDeleteTextures(1, &glid);
        }
        glGenTextures(1, &glid);
    }

    void newFramebuffer(GLuint& glid) {
        if (glid) {
            glDeleteFramebuffers(1, &glid);
        }
        glGenFramebuffers(1, &glid);
    }

    GLuint getSceneFboColorTex();
    void   resize(uint _width, uint _height);

private:
    AlgorithmType algorithm;
    bool          blur;
    float         blurSharpness;
    float         intensity;
    float         bias;
    float         radius;
    int           samples;
    uint          fboWidth;
    uint          fboHeight;

    std::array<GLint, 4> csVp;

    ssaoFbos     fbos;
    ssaoTextures textures;

    ShaderCollector* shCol;
    GLBase*          m_glbase = nullptr;

    HBAOData  hbaoUbo;
    glm::vec4 hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES];

    std::unique_ptr<Quad> quad;

    ShaderBuffer<glm::vec4>* modu_pos;
    ShaderBuffer<glm::vec4>* m_vel;
    ShaderBuffer<glm::vec4>* ref_pos;

    Shaders* depth_linearize;
    Shaders* depth_linearize_msaa;
    Shaders* viewnormal;
    Shaders* bilateralblur;
    Shaders* texShdr;
    Shaders* texNoAlpha;

    Shaders* hbao_calc;
    Shaders* hbao_calc_blur;
    Shaders* hbao_blur;
    Shaders* hbao_blur2;

    Shaders* hbao2_deinterleave;
    Shaders* hbao2_calc;
    Shaders* hbao2_calc_blur;
    Shaders* hbao2_reinterleave;
    Shaders* hbao2_reinterleave_blur;

    Shaders* debugDepth;

    std::unique_ptr<FBO> sceneFbo;

    bool inited;

    std::string vertShdr;
    std::string basicVert;
    std::string com;
    std::string fullScrQuad;
    std::string fullScrQuadGeo;
    std::string shdr_Header;

    GLint     lastBoundFbo;
    GLboolean lastMultiSample;
    GLuint    defaultVAO;
    GLuint    hbao_ubo;

    double lastUpdt;
};
}  // namespace ara
