//
// SSAO.h
//
//  Created by Sven Hahne on 19.08.14.
//

#pragma once

//#include <GeoPrimitives/Quad.h>
//#include <Shaders/ShaderUtils/ShaderBuffer.h>
//#include <Utils/FBO.h>
//#include <Utils/Geometry.h>
//#include <Utils/MersenneTwister.h>
//#include <Utils/Texture.h>
//#include <Utils/Vertex.h>
#include <GlbCommon/GlbCommon.h>

//#include "GLUtils/Noise3DTexGen.h"
//#include "SceneNodes/SceneNode.h"

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

class CameraSet;
class Quad;
class Shaders;
class ShaderCollector;
class FBO;

template <class T>
class ShaderBuffer;

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
        GLuint scene = 0;
        GLuint depthlinear = 0;
        GLuint viewnormal = 0;
        GLuint hbao_calc = 0;
        GLuint hbao2_deinterleave = 0;
        GLuint hbao2_calc = 0;
    };

    struct ssaoTextures {
        GLuint scene_color = 0;
        GLuint scene_depthstencil = 0;
        GLuint scene_depthlinear = 0;
        GLuint scene_viewnormal = 0;
        GLuint hbao_result = 0;
        GLuint hbao_blur = 0;
        GLuint hbao_random = 0;
        std::array<GLuint, MAX_SAMPLES> hbao_randomview{};
        GLuint hbao2_deptharray = 0;
        std::array<GLuint, HBAO_RANDOM_ELEMENTS> hbao2_depthview{};
        GLuint hbao2_resultarray = 0;
    };

    // corresponds to the uniform block
    struct HBAOData {
        float RadiusToScreen = 0;  // radius
        float R2 = 0;              // 1/radius
        float NegInvR2 = 0;        // radius * radius
        float NDotVBias = 0;

        glm::vec2 InvFullResolution{};
        glm::vec2 InvQuarterResolution{};

        float     AOMultiplier = 0;
        float     PowExponent = 0;
        glm::vec2 _pad0{};

        glm::vec4 projInfo{};
        glm::vec2 projScale{};
        int       projOrtho = 0;
        int       _pad1 = 0;

        std::array<glm::vec4, AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE> float2Offsets;
        std::array<glm::vec4, AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE> jitters;
    };

    SSAO(GLBase* glbase, uint inFboWidth, uint inFboHeight, AlgorithmType _algorithm = ALGORITHM_HBAO_CACHEAWARE,
         bool _blur = true, float _intensity = 8.f, float _blurSharpness = 30.0f);
    ~SSAO();

    void bind();
    static void clear(glm::vec4 clearCol = glm::vec4(0.f));
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
    //GLuint      createShaderPipelineProgram(GLuint target, const char* src);

    void prepareHbaoData(CameraSet* cs, int width, int height);
    void drawLinearDepth(CameraSet* cs, int width, int height, int sampleIdx);
    void drawHbaoBlur(int width, int height, int sampleIdx);
    void drawHbaoClassic(CameraSet* cs, int width, int height, int sampleIdx);
    void drawHbaoCacheAware(CameraSet* cs, int width, int height, int sampleIdx);
    bool initMisc();
    bool initFramebuffers(int width, int height, int samples);

    void newBuffer(GLuint& glid);
    void newTexture(GLuint& glid);
    void newFramebuffer(GLuint& glid);

    GLuint getSceneFboColorTex();
    void   resize(uint _width, uint _height);

private:
    AlgorithmType algorithm;
    bool          blur;
    float         blurSharpness;
    float         intensity;
    float         bias = 0.1f;
    float         radius = 0.2f;
    int           samples;
    uint          fboWidth;
    uint          fboHeight;

    std::array<GLint, 4> csVp;

    ssaoFbos     fbos;
    ssaoTextures textures;

    ShaderCollector& shCol;
    GLBase*          m_glbase = nullptr;

    HBAOData  hbaoUbo;
    glm::vec4 hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES];

    std::unique_ptr<Quad> quad;

    ShaderBuffer<glm::vec4>* modu_pos;
    ShaderBuffer<glm::vec4>* m_vel;
    ShaderBuffer<glm::vec4>* ref_pos;

    Shaders* depth_linearize = nullptr;
    Shaders* depth_linearize_msaa = nullptr;
    Shaders* viewnormal = nullptr;
    Shaders* bilateralblur = nullptr;
    Shaders* texShdr = nullptr;
    Shaders* texNoAlpha = nullptr;

    Shaders* hbao_calc = nullptr;
    Shaders* hbao_calc_blur = nullptr;
    Shaders* hbao_blur = nullptr;
    Shaders* hbao_blur2 = nullptr;

    Shaders* hbao2_deinterleave = nullptr;
    Shaders* hbao2_calc = nullptr;
    Shaders* hbao2_calc_blur = nullptr;
    Shaders* hbao2_reinterleave = nullptr;
    Shaders* hbao2_reinterleave_blur = nullptr;

    Shaders* debugDepth = nullptr;

    std::unique_ptr<FBO> sceneFbo;

    bool inited = false;

    std::string vertShdr;
    std::string basicVert;
    std::string com;
    std::string fullScrQuad;
    std::string fullScrQuadGeo;

    GLint     lastBoundFbo{};
    GLboolean lastMultiSample{};
    GLuint    defaultVAO{};
    GLuint    hbao_ubo{};

    double lastUpdt=0;
};
}  // namespace ara
