//
// SNParticles.cpp
//
//  Created by Sven Hahne, last modified on 22.08.17
//
//  the "update" method has to be called!!!

#include "SceneNodes/SNParticles.h"
#include "CameraSets/CameraSet.h"
#include <Shaders/ShaderCollector.h>
#include <Shaders/ShaderUtils/ShaderBuffer.h>
#include <Utils/NoiseTexNV.h>
#include <GLBase.h>
#include "GLUtils/GLSLParticleSystemCS.h"

using namespace glm;
using namespace std;

namespace ara {

SNParticles::SNParticles(sceneData* sd) : SceneNode(sd) {
    // get screen Proportions from roomDim
    m_propo = s_sd->roomDim->x / s_sd->roomDim->y;

    // addPar("alpha", &alpha);
    addPar("noiseFreq", &m_noiseFreq);
    addPar("noiseStren", &m_noiseStren);
    addPar("spriteSize", &m_spriteSize);
    addPar("initAmt", &m_initAmt);

    // setup custom matrix
    m_view        = lookAt(vec3{0.f, 0.f, m_globalScale},  // s_camPos
                           vec3(0.f),                       // s_lookAt
                           vec3{0.f, 1.f, 0.f});         // upVec

    // float aspect = float(s_sd->winViewport.z) / float(s_sd->winViewport.w);
    float f_near = 0.1f;
    float f_far  = m_globalScale * 10.f;
    m_proj         = glm::perspective(0.2f, m_propo, f_near, f_far);

    // default vao with whatever, needed for the shader storage buffers
    m_testVAO       = make_unique<VAO>("position:3f", GL_STATIC_DRAW);
    std::array pos = {vec3{-0.5f, -0.5f, 0.0f}, vec3{0.5f, -0.5f, 0.0f}, vec3{0.5f, 0.5f, 0.0f}, vec3{-0.5f, 0.5f, 0.0f}};
    m_testVAO->upload(CoordType::Position, &pos[0][0], pos.size());
    std::array<GLuint, 6> ind = {0, 1, 3, 1, 3, 2};
    m_testVAO->setElemIndices(ind.size(), ind.data());

    // create ubo and initialize it with the structure data
    glGenBuffers(1, &m_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &m_shaderParams, GL_STREAM_DRAW);

    // create simple single-vertex VBO
    vec4 vtx_data = {0.f, 0.f, 0.f, 1.f};
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), &vtx_data[0], GL_STATIC_DRAW);

    // For now, scale back the particle count on mobile.
    m_particles = make_unique<GLSLParticleSystemCS>(m_numParticles, ShaderCollector::getShaderHeader());
    m_particles->reset({m_propo, 1.f, 1.f});

    m_partColors[0] = { 0.3f, 0.5f, 0.8f, 1.f};
    m_partColors[1] = {0.3f, 0.53f, 0.81f, 1.f };
    m_partColors[2] = { 0.7f, 0.23f, 0.41f, 1.f };
    m_partColors[3] = { 0.3f, 0.75f, 0.8f, 1.f };
    m_partColors[4] = { 0.2f, 0.33f, 0.21f, 1.f };
    m_partColors[5] = { 0.1f, 0.56f, 0.61f, 1.f };
}

void SNParticles::initShdr() {
    std::string shdr_Header = "#version 430\n#define WORK_GROUP_SIZE 128\n";
    auto vert = shdr_Header + ara::GLSLParticleSystemCS::getShaderParUBlock() + getVertShader();

    shdr_Header = "#version 430\n";
    auto frag = getFragShader();
    frag = shdr_Header + "// SNParticles frag\n" + frag;

    m_renderProg = m_shCol->add("ParticleCSShader3234", vert, frag);
}

std::string SNParticles::getVertShader() {
    return STRINGIFY(
        layout( std140, binding=1 ) buffer Pos { vec4 pos[]; }; \n
                                                                \n
        uniform vec4[6] colors;                                 \n
        uniform sampler3D noiseCube;                            \n
                                                                \n
        out gl_PerVertex {                                      \n
            vec4 gl_Position;                                   \n
        };                                                      \n
                                                                \n
        out block {                                             \n
            vec4 color;                                         \n
            vec2 texCoord;                                      \n
        } Out;                                                  \n
                                                                \n
        void main() {                                           \n
            // expand points to quads without using GS
            int particleID = gl_VertexID >> 2;                  \n // 4 vertices per particle
            vec4 particlePos = pos[particleID];                 \n
            particlePos.x *= 3.0;                               \n
            particlePos.z *= 3.0;                               \n
                                                                \n
            Out.color = mix(colors[1], colors[2], min ( max( texture(noiseCube, particlePos.xyz * 0.1) + 0.3, 0.0), 1.0));\n
            Out.color.a *= 0.2;                                 \n
                                                                \n
            //map vertex ID to quad vertex\n
            vec2 quadPos = vec2( float(((gl_VertexID - 1) & 2) >> 1), float((gl_VertexID & 2) >> 1));\n
            Out.texCoord = quadPos;                             \n
                                                                \n
            quadPos = quadPos * 2.0 - 1.0;                      \n
            vec4 particlePosEye = ModelView * particlePos;      \n
            vec4 vertexPosEye = particlePosEye + vec4(quadPos * spriteSize, 0, 0);\n
                                                                \n
            gl_Position = ProjectionMatrix * vertexPosEye;      \n
    });
}

std::string SNParticles::getFragShader() {
    return STRINGIFY(
        in block {                                      \n
            vec4 color;                                 \n
            vec2 texCoord;                              \n
        } In;                                           \n
                                                        \n
        layout(location=0) out vec4 fragColor;          \n
        uniform float alpha;                            \n
                                                        \n
        void main() {                                   \n
            // Quick fall-off computation
            float r = length(In.texCoord*2.0-1.0)*3.0;  \n
            float i = exp(-r*r);                        \n
            i *= i*i;                                   \n
            if (i < 0.01) discard;                      \n
                                                        \n
            fragColor = vec4(In.color.rgb * 1.4, i * In.color.a * alpha);\n
    });
}

void SNParticles::draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo) {
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_CLAMP);
    glActiveTexture(GL_TEXTURE0);

    m_shaderParams.ModelView           = cs->getModelMatr() * m_model;
    m_shaderParams.ModelViewProjection = cs->getProjectionMatr() * cs->getViewMatr() * m_model;
    m_shaderParams.ProjectionMatrix    = cs->getProjectionMatr();

    // bind the buffer for the UBO, and update it with the latest values from
    // the CPU-side struct
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &m_shaderParams);

    // this buffer is also used by the compute shader inside GLSLParticleSystem
    // make framerate independent
    if (time - m_lastPartUpdt > 0.016) {
        m_particles->update();
        m_lastPartUpdt = time;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);  // if using glScissor

    // draw particles
    m_renderProg->begin();
    m_renderProg->setUniform4fv("colors", &m_partColors[0][0], 6);
    m_renderProg->setUniform1f("alpha", m_alpha);
    m_renderProg->setUniform1i("noiseCube", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_particles->getNoiseTex());

    // reference the compute shader buffer, which we will use for the particle
    // if there is no vao bound, glDrawElements doesn't work...
    m_testVAO->bind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_particles->getPosBuffer().getBuffer());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_particles->getIndexBuffer().getBuffer());

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_particles->getSize()) * 6, GL_UNSIGNED_INT, nullptr);

    Shaders::end();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_BLEND);
}

void SNParticles::update(double time, double dt) {
    if (!m_shCol) {
        m_shCol = &m_glbase->shaderCollector();
        initShdr();
    }

    m_model = glm::rotate(static_cast<float>(M_PI * time * 0.02), glm::vec3(0.f, 1.f, 0.f)) * glm::translate(glm::vec3(0.f, 0.f, 0.f));

    // update struct representing UBO
    m_shaderParams.numParticles  = static_cast<uint>(m_particles->getSize());
    m_shaderParams.noiseStrength = m_noiseStren + static_cast<float>(sin(time * 0.02f)) * 0.0008f;
    m_shaderParams.spriteSize    = m_spriteSize;
    m_initAmt                     = static_cast<float>(sin(time * 0.125)) * 0.0025f + 0.0025f;
    m_shaderParams.initAmt       = m_initAmt;
    m_shaderParams.noiseFreq     = m_noiseFreq + static_cast<float>(sin(time * 0.02)) * 0.5f;

    if (mEnableAttractor) {
        // move attractor
        constexpr float speed         = 0.1f;
        m_shaderParams.attractor.x = sin(m_time * speed + static_cast<float>(sin(time * 0.01)) * 0.01f);
        m_shaderParams.attractor.y = sin(m_time * speed * 1.3f);
        m_shaderParams.attractor.z = cos(m_time * speed);
        m_time += static_cast<float>(dt);

        m_shaderParams.attractor.w = 0.00002f;
    } else {
        m_shaderParams.attractor.w = 0.0f;
    }
}

void SNParticles::onKey(int key, int scancode, int action, int mods) {}

SNParticles::~SNParticles() {
    glDeleteBuffers(1, &m_UBO);
    glDeleteBuffers(1, &m_VBO);
}

}  // namespace ara
