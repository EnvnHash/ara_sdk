/**
 * @brief Renders dynamical amount of cameras to a layered fbo. on postRender()
 * all fbos get rendered to the standard framebuffer in respect to the
 * viewpoints set inside the camera definition
 *
 * Author: Sven Hahne
 * last modified on 24.08.22
 *
 */

#include "CameraSets/CsPerspFbo.h"

using namespace glm;
using namespace std;

namespace ara {
CsPerspFbo::CsPerspFbo(sceneData* sd)
    : CameraSet(sd), m_savedNr(0), m_reqSnapshot(false), m_layerTexShdr(nullptr), m_clearShdr(nullptr) {
    if (!s_sd) return;

    m_camPos     = vec3{0.f, 0.f, 1.f};
    float aspect = s_sd->winViewport.z / s_sd->winViewport.w;
    m_quad       = make_unique<Quad>(QuadInitParams{});
    s_viewport   = s_sd->winViewport;
    s_iViewport  = s_sd->winViewport;

    if (s_sd->winViewport.z > 0.f && s_sd->winViewport.w > 0.f) {
        s_intern_cam.emplace_back(make_unique<TrackBallCam>(CameraInitParams{
            .cTyp = camType::perspective,
            .screenSize { s_sd->winViewport.z, s_sd->winViewport.w},
            .rect = { -aspect, aspect, -1.0f, 1.0f },  // left, right, bottom, top
            .cp = m_camPos
        }));

        s_intern_cam.back()->setUseTrackBall(true);
        s_cam.emplace_back(s_intern_cam.back().get(), this);
        setInteractCam(s_intern_cam.back().get());

        rebuildFbo();
    }

    initLayerTexShdr();
    buildCamMatrixArrays();
}

void CsPerspFbo::initLayerTexShdr() {
    if (m_layerTexShdr) s_shCol->deleteShader("CsPerspFbo");

    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 1) in vec4 normal; \n
        layout(location = 2) in vec2 texCoord; \n
        layout(location = 3) in vec4 color; \n
        out vec2 tex_coord; \n
        uniform mat4 m_pvm; \n
        void main() { \n
            tex_coord   = texCoord;\n
            gl_Position = m_pvm * position;\n
        });

    vert = s_shCol->getShaderHeader() + "// CsPerspFbo layer texture shader, vert\n" + vert;

#if defined(ARA_USE_GLES31)
    string frag = "highp uniform sampler2DArray tex; \n";
#else
    string frag = "uniform sampler2DArray tex; \n";
#endif

    frag += STRINGIFY(
        uniform int useBorder; \n
        uniform vec2 borderWidth; \n
        uniform vec4 borderColor; \n
        uniform float layerNr; \n
        in vec2 tex_coord; \n
        layout(location = 0) out vec4 color; \n void
        main() { \n
            vec2 border = vec2( \n
                tex_coord.x < borderWidth.x ? \n
                    1.0 : tex_coord.x > (1.0 - borderWidth.x) ? \n
                        1.0 : 0.0, \n
                tex_coord.y < borderWidth.y ? \n
                    1.0 : tex_coord.y > (1.0 - borderWidth.y) ? \n
                        1.0 : 0.0); \n);

    frag += STRINGIFY(
            color = bool(useBorder) ? \n
                mix(texture(tex, vec3(tex_coord, float(layerNr))), borderColor, max(border.x, border.y)) \n
                : texture(tex, vec3(tex_coord, float(layerNr)));
            });

    frag = s_shCol->getShaderHeader() + "// CsPerspFbo layer texture shader, frag\n" + frag;

    m_layerTexShdr = s_shCol->add("CsPerspFbo", vert, frag);
}

void CsPerspFbo::initClearShader(int nrLayers) {
    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });
    vert             = s_shCol->getShaderHeader() + "// CsPerspFbo clear shader, vert\n" + vert;

    //---------------------------------------------------------

    std::string shdr_Header_g = s_shCol->getShaderHeader() +
                                "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\nuniform vec4 "
                                "clearCol[" +
                                std::to_string(nrLayers) + "];\n";

    std::string geom = STRINGIFY(uniform mat4 m_pvm; out vec4 o_col;
        void main() {\n
            for (int i = 0; i < gl_in.length(); i++) {\n
                gl_Layer = gl_InvocationID;
                o_col       = clearCol[gl_InvocationID];
                gl_Position = m_pvm * gl_in[i].gl_Position;\n
                EmitVertex();\n
            }\n
            EndPrimitive();\n
    });

    geom = shdr_Header_g + "// CsPerspFbo clear shader, geom\n" + geom;

    //---------------------------------------------------------

    std::string frag = "layout (location = 0) out vec4 color; in vec4 o_col;\n void main() { "
        "color = o_col; }";
    frag = s_shCol->getShaderHeader() + "// CsPerspFbo clear color shader, frag\n" + frag;

    m_clearShdr = s_shCol->add("CsPerspFbo_ClearShader", vert, geom, frag);
}

void CsPerspFbo::rebuildFbo() {
    bool nrCamsChanged = m_fbo.getDepth() != (int)s_cam.size();

    if (m_fbo.getWidth() != (int)s_viewport.z
        || m_fbo.getHeight() != (int)s_viewport.w
        || nrCamsChanged
        || m_fbo.getNrSamples() != m_glbase->getNrSamples()
        || m_fbo.getType() != GL_RGBA8
        || m_fbo.getTarget() != GL_TEXTURE_2D_ARRAY
        || !m_fbo.isInited()) {
        if (nrCamsChanged || !m_fbo.isInited()) {
            if (m_clearShdr) {
                s_shCol->deleteShader("CsPerspFbo_ClearShader");
            }
            initClearShader((int)s_cam.size());
        }

        if (m_fbo.isInited()) {
            m_fbo.remove();
        }

        m_fbo.setWidth((int)s_viewport.z);
        m_fbo.setHeight((int)s_viewport.w);

        m_fbo.setNrSamples(m_glbase->getNrSamples());
        m_fbo.setType(GL_RGBA8);
        m_fbo.setDepth((int)s_cam.size());
        m_fbo.setTarget(GL_TEXTURE_2D_ARRAY);  // m_fbo.setTarget(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
        m_fbo.setHasDepthBuffer(true);
        m_fbo.setMipMapLevels(4);
        m_fbo.setGlbase(m_glbase);
        m_fbo.init();

        if (!s_updtCb.empty()) {
            for (const auto& cbList : s_updtCb | views::values) {
                for (const auto& cb : cbList) {
                    cb();
                }
            }
        }
    }
}

void CsPerspFbo::clearScreen(renderPass pass) {
    if ((pass == GLSG_SCENE_PASS || pass == GLSG_GIZMO_PASS) && m_clearShdr) {
        m_fbo.bind();

        glClearDepthf(1.f);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        glBlendFunc(GL_ONE, GL_ZERO);

        // individual background colors for each layer
        m_clearShdr->begin();
        m_clearShdr->setUniform4fv("clearCol", &s_clearColors[0][0], (int)s_clearColors.size());
        m_clearShdr->setIdentMatrix4fv("m_pvm");
        m_quad->draw();
        Shaders::end();

        m_fbo.unbind();

        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    } else if (pass == GLSG_SHADOW_MAP_PASS || pass == GLSG_OBJECT_MAP_PASS) {
        for (const auto& it : s_shaderProto | views::values) {
            it->clear(pass);
        }
    }
}

void CsPerspFbo::clearDepth() { m_fbo.clearDepth(); }

void CsPerspFbo::renderTree(SceneNode* node, double time, double dt, uint ctxNr, renderPass pass) {
    if (node->m_calcMatrixStack.load()) {
        s_matrixStack.clear();
    }

    if (pass == GLSG_SCENE_PASS) {
        m_fbo.bind();
    }

    iterateNode(node, time, dt, ctxNr, pass, node->m_calcMatrixStack.load());

    if (node->m_calcMatrixStack.load()) {
        node->m_calcMatrixStack = false;
    }

    if (pass == GLSG_SCENE_PASS) {
        m_fbo.unbind();
    }
}

void CsPerspFbo::render(SceneNode* node, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass) {
    node->update(time, dt, this);

    if (node->m_emptyDrawFunc) {
        return;
    }

    // Iterate through the CameraSets shaderPrototypes
    auto proto = getProtoForPass(pass, node);
    if (proto) {
        s_actFboSize = vec2(s_fScrWidth, s_fScrHeight);

        int  loopNr = 0;
        bool run    = true;

        while (run) { // if ShaderProtoype->end return a value > 0 the scene will be drawn another time
            if (proto->begin(this, pass, loopNr)) {
                proto->sendPar(this, time, node, parent, pass, loopNr);
                if (proto->getShader(pass, loopNr)) {
                    node->draw(time, dt, this, proto->getShader(pass, loopNr), pass);
                }
            }

            run = proto->end(pass, loopNr);
            ++loopNr;
        }
    }
}

void CsPerspFbo::postRender(renderPass _pass, float* extDrawMatr) {
    for (const auto& it : s_shaderProto) {
        it.second->postRender(_pass);
    }

    if (_pass == GLSG_SCENE_PASS) {
        renderFbos(extDrawMatr);
    }
}

void CsPerspFbo::renderFbos(float* extDrawMatr) {
    // s_fbo contains an 2d texture array, render them all in parallel
    m_layerTexShdr->begin();
    if (extDrawMatr)
        m_layerTexShdr->setUniformMatrix4fv("m_pvm", extDrawMatr);
    else
        m_layerTexShdr->setIdentMatrix4fv("m_pvm");
    m_layerTexShdr->setUniform1i("tex", 0);
    m_layerTexShdr->setUniform1i("useBorder", 0);
    m_layerTexShdr->setUniform1f("layerNr", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_fbo.getColorImg());
    glDisable(GL_DEPTH_TEST);

    m_quad->draw();
}

void CsPerspFbo::setViewport(uint x, uint y, uint width, uint height, bool resizeProto) {
    CameraSet::setViewport(x, y, width, height, resizeProto);
    rebuildFbo();
}

vector<pair<TrackBallCam*, void*>>::iterator CsPerspFbo::addCamera(TrackBallCam* camDef, void* name) {
    s_cam.emplace_back(camDef, name);

    buildCamMatrixArrays();
    rebuildFbo();
    initLayerTexShdr();

    for (auto&[fst, snd] : s_shaderProto) {
        snd->setNrCams(static_cast<int>(s_cam.size()));
    }

    return s_cam.end() - 1;
}

}  // namespace ara
