/**
 * @brief Renders dynamical amount of cameras to a layered fbo. on postRender()
 * all fbos get rendered to the standard framebuffer in respect to the
 * viewpoints set inside the camera definition
 *
 * Author: Sven Hahne
 * last modified on 24.08.22
 *
 */

#include "CameraSets/CsStereoFbo.h"
#include <Utils/Stereo/DistortionMesh.h>
#include <UIApplication.h>
#include <UIWindow.h>

using namespace glm;
using namespace std;

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)
using namespace ara::cap;
#endif

namespace ara {
CsStereoFbo::CsStereoFbo(sceneData* sd) : CameraSet(sd), m_layerTexShdr(nullptr), m_clearShdr(nullptr) {
    if (!s_sd) return;

    m_quad    = make_unique<Quad>(QuadInitParams{ .color = { 1.f, 0.f, 1.f, 1.f} });
    m_plane   = make_unique<VAO>("position:3f", GL_DYNAMIC_DRAW);
    m_colShdr = s_shCol->getStdCol();

    s_viewport  = s_sd->winViewport;
    s_iViewport = s_sd->winViewport;

    uptStereoRender();

    m_camPos = vec3{0.f, 0.f, 1.f};

    if (s_sd->winViewport.z > 0.f && s_sd->winViewport.w > 0.f) {
        for (int i = 0; i < 2; i++) {
            auto f = m_stereoRenderer.getFov((StereoEye)i);

            s_intern_cam.push_back(make_unique<TrackBallCam>(CameraInitParams{
                .cTyp = camType::perspective,
                .screenSize = { m_stereoRenderer.getFov((StereoEye)i)[0], m_stereoRenderer.getFov((StereoEye)i)[2] },
                .rect = { -0.5f, 0.5f, -0.5f, 0.5f },  // left, right, bottom, top
                .cp = { m_camPos.x + m_stereoRenderer.getViewEyeOffs((StereoEye)i), m_camPos.y, m_camPos.z },
                .la = { m_stereoRenderer.getViewEyeOffs((StereoEye)i), 0.f, 0.f},
                .fov = glm::degrees(m_stereoRenderer.getFov((StereoEye)i)[3] +
                             m_stereoRenderer.getFov((StereoEye)i)[2])
            }));  // fov

            s_intern_cam.back()->setUseTrackBall(true);
            s_cam.push_back(make_pair(s_intern_cam.back().get(), this));

            if (i == 0) {
                m_stereoViewAspect =
                    m_stereoRenderer.getFov((StereoEye)i)[0] / m_stereoRenderer.getFov((StereoEye)i)[2];
                setInteractCam(s_intern_cam.back().get());

                // sync both cameras, listen to updates on the first cam and
                // apply it to the second
                s_intern_cam.back()->addTrackBallUpdtCb(this, [this](TbModData& dt) {
                    s_cam.back().first->setTrackBallTrans(dt.trans);
                    s_cam.back().first->setTrackBallRot(dt.rotEuler);
                    s_cam.back().first->updateFromExternal();
                });
            } else if (i == 1) {
                s_intern_cam.back()->setCamSetUpdtCb([this] { buildCamMatrixArrays(); });
            }
        }

        rebuildFbo();
    }

    initLayerTexShdr();

    buildCamMatrixArrays();
}

// setup stereo view lensdistortion correction

void CsStereoFbo::uptStereoRender() {
    auto win = static_cast<UIWindow*>(s_sd->uiWindow);

    auto scr_size = vec2{s_sd->winViewport.z, s_sd->winViewport.w};

#ifdef __ANDROID__
    auto dpi = glm::vec2{win->getApplicationHandle()->m_cmd_data.xdpi, win->getApplicationHandle()->m_cmd_data.ydpi};
#else
    auto dpi = win->getWinHandle()->getDpi() * 4.f;  // TODO: solve this calculation mismatch between android
                                                     // dpi and desktop dpi
#endif

    m_stereoRenderer.setScreenParam(dpi, scr_size);
    m_stereoRenderer.init();
}

void CsStereoFbo::buildCamMatrixArrays() {
    s_modelMatrixList.resize(s_cam.size());
    s_viewMatrixList.resize(s_cam.size());
    s_projectionMatrixList.resize(s_cam.size());
    s_clearColors.resize(s_cam.size());
    s_floorSwitches.resize(s_cam.size());
    s_fishEyeSwitches.resize(s_cam.size());
    s_fishEyeParam.resize(s_cam.size());

    for (size_t i = 0; i < s_cam.size(); i++) {
#ifndef ARA_USE_ARCORE
        s_modelMatrixList[i]      = s_cam[i].first->getModelMatr();
        s_viewMatrixList[i]       = s_cam[i].first->getViewMatr();
        s_projectionMatrixList[i] = s_cam[i].first->getProjectionMatr();
#endif
        s_clearColors[i]     = s_cam[i].first->getClearColor();
        s_floorSwitches[i]   = s_cam[i].first->getFloorSwitch();
        s_fishEyeSwitches[i] = s_cam[i].first->getFishEyeSwitch();
        s_fishEyeParam[i]    = s_cam[i].first->getFishEyeParam();
    }

#ifdef ARA_USE_ARCORE
    buildViewMatrixArrays();
#endif
}

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)

void CsStereoFbo::buildViewMatrixArrays() {
    if (!m_arCore) return;

    for (size_t i = 0; i < s_cam.size(); i++) {
        s_modelMatrixList[i] = m_arCore->getModelMat();
        if (m_viewMode == CsStereoViewMode::Stereo) {
            s_viewMatrixList[i] =
                glm::translate(vec3{m_stereoRenderer.getViewEyeOffs((StereoEye)i) * (i == 0 ? 1.f : -1.f), 0.f, 0.f}) *
                m_arCore->getViewMat();
        } else {
            s_viewMatrixList[i] = m_arCore->getViewMat();
        }

        s_projectionMatrixList[i] = m_arCore->getProjMat() * s_viewMatrixList[i];
    }
}

void CsStereoFbo::setArCore(cap::ARCore* arCore) {
    m_arCore = arCore;
    m_arCore->init(m_glbase);

    auto win = static_cast<UIWindow*>(s_sd->uiWindow);
    if (win && win->getApplicationHandle()) {
        auto app = win->getApplicationHandle();
        m_arCore->onResume(&app->m_cmd_data);
    }

    // set m_arCoreCam shaderCollector
    m_arCoreCam.m_arCore = arCore;
    m_arCoreCam.setShaderCollector(s_shCol);
    m_arCoreCam.setDataPath(s_sd->dataPath);
    m_arCoreCam.setGlBase(m_glbase);
    m_arCoreCam.setWin(win);
    m_arCoreCam.initGLRes(2);

    buildCamMatrixArrays();
}

#endif

void CsStereoFbo::initLayerTexShdr() {
    std::string shdrName = "CsStereoFbo";

    if (s_shCol->hasShader(shdrName)) {
        m_layerTexShdr = s_shCol->get(shdrName);
        return;
    }

    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n out vec2 tex_coord; \n uniform mat4 m_pvm; \n uniform mat4 trans; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * trans * position;
                \n
            });

    vert = s_shCol->getShaderHeader() + "// CsStereoFbo layer texture shader, vert\n" + vert;

#ifdef ARA_USE_GLES31
    string frag = "highp uniform sampler2DArray tex; \n";
#else
    string frag = "uniform sampler2DArray tex; \n";
#endif

    frag += STRINGIFY(
        uniform int useBorder; \n uniform float layerNr; \n uniform float texAsp;\n uniform float stereoAsp;\n uniform int viewMode;\n uniform vec2 centre;\n in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n void
            main() {
                \n

                    vec4 col   = vec4(0.0, 0.0, 0.0, 1.0);  // base colour
                float    alpha = 0.2;                       // lens parameter
                // Left/Right eye are slightly off centre
                // Normalize to [-1, 1] and put the centre to "centre"
                vec2 p1 = vec2(2.0 * tex_coord - 1.0) - centre;
                p1 *= 0.87;
                // Transform
                vec2 p2 = p1 / (1.0 - alpha * length(p1));
                // Back to [0, 1]
                p2 = (p2 + centre + 1.0) * 0.5;

                color = bool(viewMode)
                            ? (all(greaterThanEqual(p2, vec2(0.0))) && all(lessThanEqual(p2, vec2(1.0)))
                                   ? texture(tex, vec3((tex_coord.x + (texAsp - stereoAsp) * 0.5) / texAsp * stereoAsp,
                                                       tex_coord.y, layerNr))
                                   : col)
                            : texture(tex, vec3(tex_coord.x, tex_coord.y, layerNr));
                /*
                            color = bool(viewMode) ? texture(tex,
                   vec3((tex_coord.x + (texAsp - stereoAsp) * 0.5) / texAsp *
                   stereoAsp, tex_coord.y, layerNr)) : texture(tex,
                   vec3(tex_coord.x, tex_coord.y, layerNr)); \n color=vec4(1.0,
                   0.0, 0.0, 1.0);*/
            });

    frag = s_shCol->getShaderHeader() + "// CsStereoFbo layer texture shader, frag\n" + frag;

    m_layerTexShdr = s_shCol->add(shdrName, vert, frag);
}

void CsStereoFbo::initBackTexShdr(size_t nrLayers) {
    std::string shdrName = "CsStereoFbo_backTex_" + std::to_string(nrLayers);

    if (s_shCol->hasShader(shdrName)) {
        m_backTexShdr = s_shCol->get(shdrName);
        return;
    }

    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n uniform vec2 uv[6]; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = vec2(uv[gl_VertexID].x, 1.0 - uv[gl_VertexID].y);
                \n gl_Position = position;
                \n
            });

    vert = s_shCol->getShaderHeader() + "// CsStereoFbo backTex shader, vert\n" + vert;

    std::string shdr_Header_g = s_shCol->getShaderHeader() +
                                "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\nuniform vec4 "
                                "clearCol[" +
                                std::to_string(nrLayers) + "];\n";

    std::string geom = STRINGIFY(in vec2 tex_coord[]; \n out vec2 texCoord; \n void main() {
        \n for (int i = 0; i < gl_in.length(); i++) {
            \n gl_Layer    = gl_InvocationID;
            texCoord       = tex_coord[i];
            \n gl_Position = gl_in[i].gl_Position;
            \n EmitVertex();
            \n
        }
        \n EndPrimitive();
        \n
    });

    geom = shdr_Header_g + "// CsStereoFbo backTex shader, geom\n" + geom;

#ifdef __ANDROID__
    string frag = "uniform samplerExternalOES tex;\n";
#else
    string frag = "uniform sampler2D tex;\n";
#endif

    frag += STRINGIFY(in vec2 texCoord; \n layout(location = 0) out vec4 color; \n void main() {
        \n color = texture(tex, vec2(texCoord.x, 1.0 - texCoord.y));
        \n
    });

#ifdef __ANDROID__
    frag =
        "#version 320 es\n#extension GL_OES_EGL_image_external : "
        "require\n#extension GL_OES_EGL_image_external_essl3 : require\n "
        "precision highp float;\n // CsStereoFbo backTex shader, frag\n" +
        frag;
#else
    frag = s_shCol->getShaderHeader() + "// CsStereoFbo backTex shader, frag\n" + frag;
#endif

    m_backTexShdr = s_shCol->add(shdrName, vert, geom, frag);
}

void CsStereoFbo::initPlaneShdr(size_t nrLayers) {
#ifdef ARA_USE_ARCORE
    std::string shdrName = "CsStereoFbo_PlaneRend_" + std::to_string(nrLayers);
    if (s_shCol->hasShader(shdrName)) {
        m_planeShdr = s_shCol->get(shdrName);
        return;
    }

    std::string vert = STRINGIFY(
        layout(location = 0) in vec3 position;\n out VS_GS { \n float alpha; } vout;\n void main() {
            // Vertex Z value is used as the alpha in this shader.
            vout.alpha  = position.z;
            gl_Position = vec4(position.x, 0.0, position.y, 1.0);
        });
    vert =
        "#version 320 es\nprecision highp float;\n precision highp int; \n // "
        "CsStereoFbo plane shader, vert\n" +
        vert;

    std::string shdr_Header_g = s_shCol->getShaderHeader() +
                                "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\n";
    std::string geom = "uniform mat4 mvp[" + std::to_string(nrLayers) + "];";

    geom += STRINGIFY(
        in VS_GS { \n
            float alpha;
        } vin[];\n
        out GS_FS {\n
            vec2 tex_coord; \n
            float alpha;
        } vout;\n
        uniform vec3 normal; \n
        uniform mat4 model_mat; \n
        uniform float texAsp; \n
        uniform float stereoAsp; \n
        void main() { \n
            for (int i = 0; i < gl_in.length(); i++) {\n
                gl_Layer = gl_InvocationID;
                vout.alpha = vin[i].alpha; \n

                gl_Position = mvp[gl_InvocationID] * gl_in[i].gl_Position; \n
                vec4 world_pos = model_mat * gl_in[i].gl_Position;

                // Construct two vectors that are orthogonal to the normal.
                // This arbitrary choice is not co-linear with either horizontal
                // or vertical plane normals.
                const vec3 arbitrary = vec3(1.0, 1.0, 0.0);
                vec3 vec_u = normalize(cross(normal, arbitrary));
                vec3 vec_v = normalize(cross(normal, vec_u));

                // ProjectBase vertices in world frame onto vec_u and vec_v.
                vout.tex_coord = vec2(dot(world_pos.xyz, vec_u), dot(world_pos.xyz, vec_v));
                EmitVertex(); \n
            }\n
        EndPrimitive(); \n
    });

    geom = shdr_Header_g + "// CsStereoFbo plane shader, geom\n" + geom;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        uniform sampler2D tex;
        in GS_FS {\n
            vec2 tex_coord; \n
            float alpha;
        } vin;\n
        void main(){ \n
            fragColor = vec4(texture(tex, vin.tex_coord).r * vin.alpha); \n
    });

    frag =
        "#version 320 es\nprecision highp int;\nprecision highp float;\n // "
        "CsStereoFbo plane shader, frag\n" +
        frag;

    m_planeShdr = s_shCol->add(shdrName, vert, geom, frag);
#endif
}

void CsStereoFbo::initFocSquareShdr(size_t nrLayers) {
    std::string shdrName = "CsStereoFbo_FocusSquare_" + std::to_string(nrLayers);
    if (s_shCol->hasShader(shdrName)) {
        m_focSqShdr = s_shCol->get(shdrName);
        return;
    }

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position;\n
        layout(location = 2) in vec2 texCoord;\n
        out VS_GS { \n
            vec2 tex_coord; \n
        } vout;\n
        void main() {
            vout.tex_coord = texCoord; \n
            gl_Position = vec4(position.x * 0.05, 0.0, position.y * 0.05, 1.0);\n
    });

    vert =
        "#version 320 es\nprecision highp float;\n precision highp int; \n // "
        "CsStereoFbo focus square shader, vert\n" +
        vert;

    std::string shdr_Header_g = s_shCol->getShaderHeader() +
                                "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\n";

    std::string geom = "uniform mat4 mvp[" + std::to_string(nrLayers) + "];";

    geom += STRINGIFY(
        in VS_GS {\n
            vec2 tex_coord; \n
        } vin[];\n
        out GS_FS {\n
            vec2 tex_coord; \n
        } vout;\n
        void main() { \n
            for (int i = 0; i < gl_in.length(); i++) {\n
                gl_Layer = gl_InvocationID;
                vout.tex_coord = vin[i].tex_coord; \n
                gl_Position = mvp[gl_InvocationID] * gl_in[i].gl_Position; \n
                EmitVertex(); \n
            }\n
            EndPrimitive(); \n
    });

    geom = shdr_Header_g + "// CsStereoFbo focus square shader, shader, geom\n" + geom;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        in GS_FS {\n
            vec2 tex_coord; \n
        } vin;\n
        void main(){ \n
            float r = length(vin.tex_coord -0.5);
            float c = float(r < 0.5 && r > 0.35);
            fragColor = vec4(c); \n
    });

    frag =
        "#version 320 es\nprecision highp int;\nprecision highp float;\n // "
        "CsStereoFbo focus square shader, frag\n" +
        frag;

    m_focSqShdr = s_shCol->add(shdrName, vert, geom, frag);
}

void CsStereoFbo::initClearShader(size_t nrLayers) {
    std::string shdrName = "CsStereoFbo_ClearShader_" + std::to_string(nrLayers);
    if (s_shCol->hasShader(shdrName)) {
        m_clearShdr = s_shCol->get(shdrName);
        return;
    }

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });
    vert             = s_shCol->getShaderHeader() + "// CsStereoFbo clear shader, vert\n" + vert;

    //---------------------------------------------------------

    std::string shdr_Header_g = s_shCol->getShaderHeader() +
                                "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\nuniform vec4 "
                                "clearCol[" +
                                std::to_string(nrLayers) + "];\n";

    std::string geom = STRINGIFY(uniform mat4 m_pvm; out vec4 o_col; void main() {
        \n for (int i = 0; i < gl_in.length(); i++) {
            \n gl_Layer = gl_InvocationID;
            o_col       = clearCol[gl_InvocationID];
            gl_Position = m_pvm * gl_in[i].gl_Position;
            \n EmitVertex();
            \n
        }
        \n EndPrimitive();
        \n
    });

    geom = shdr_Header_g + "// CsStereoFbo clear shader, geom\n" + geom;

    //---------------------------------------------------------

    std::string frag =
        "layout (location = 0) out vec4 color; in vec4 o_col;\n void main() { "
        "color = o_col; }";
    frag = s_shCol->getShaderHeader() + "// CsStereoFbo clear color shader, frag\n" + frag;

    m_clearShdr = s_shCol->add(shdrName, vert, geom, frag);
}

void CsStereoFbo::rebuildFbo() {
    bool nrCamsChanged = m_fbo.getDepth() != (int)s_cam.size();

    if (m_fbo.getWidth() != (int)s_viewport.z || m_fbo.getHeight() != (int)s_viewport.w || nrCamsChanged ||
        m_fbo.getNrSamples() != m_glbase->getNrSamples() || m_fbo.getType() != GL_RGBA8 ||
        m_fbo.getTarget() != GL_TEXTURE_2D_ARRAY || !m_fbo.isInited()) {
        if (nrCamsChanged || !m_fbo.isInited()) {
            initClearShader((int)s_cam.size());
            initBackTexShdr(s_cam.size());
#ifdef ARA_USE_ARCORE
            initBackTexShdr(s_cam.size());
            initPlaneShdr(s_cam.size());
            initFocSquareShdr(s_cam.size());
#endif
        }

        if (m_fbo.isInited()) m_fbo.remove();

        m_fbo.setWidth((int)s_viewport.z);
        m_fbo.setHeight((int)s_viewport.w);
        m_fbo.setDepth((int)s_cam.size());
        m_fbo.setNrSamples(m_glbase->getNrSamples());
        m_fbo.setType(GL_RGBA8);
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

void CsStereoFbo::clearScreen(renderPass pass) {
    if ((pass == GLSG_SCENE_PASS || pass == GLSG_GIZMO_PASS) && m_clearShdr) {
        m_fbo.bind();

        glClearDepthf(1.f);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        glBlendFunc(GL_ONE, GL_ZERO);

        // individual background colors for each layer
        m_clearShdr->begin();
        m_clearShdr->setUniform4fv("clearCol", &s_clearColors[0][0], (int)s_clearColors.size());
        m_clearShdr->setIdentMatrix4fv("m_pvm");
        m_quad->draw();
        m_clearShdr->end();

        renderBackground();

        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_fbo.unbind();

    } else if (pass == GLSG_SHADOW_MAP_PASS || pass == GLSG_OBJECT_MAP_PASS) {
        for (const auto& it : s_shaderProto) {
            it.second->clear(pass);
        }
    }
}

void CsStereoFbo::clearDepth() { m_fbo.clearDepth(); }

void CsStereoFbo::renderTree(SceneNode* node, double time, double dt, uint ctxNr, renderPass pass) {
    if (node->m_calcMatrixStack.load()) {
        s_matrixStack.clear();
    }

    if (pass == GLSG_SCENE_PASS) m_fbo.bind();

    iterateNode(node, time, dt, ctxNr, pass, node->m_calcMatrixStack.load());

    if (node->m_calcMatrixStack.load()) {
        node->m_calcMatrixStack = false;
    }

    if (pass == GLSG_SCENE_PASS) m_fbo.unbind();
}

void CsStereoFbo::render(SceneNode* node, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass) {
    node->update(time, dt, this);

    if (node->m_emptyDrawFunc) return;

    // Iterate through the CameraSets shaderPrototypes
    auto proto = getProtoForPass(pass, node);
    if (proto) {
        s_actFboSize = vec2(s_fScrWidth, s_fScrHeight);

        int  loopNr = 0;
        bool run    = true;

        while (run)  // if ShaderProtoype->end return a value > 0 the scene will
                     // be drawn another time
        {
            if (proto->begin(this, pass, loopNr)) {
                proto->sendPar(this, time, node, parent, pass, loopNr);
                if (proto->getShader(pass, loopNr)) node->draw(time, dt, this, proto->getShader(pass, loopNr), pass);
            }

            run = proto->end(pass, loopNr);
            loopNr++;
        }
    }
}

void CsStereoFbo::renderBackground() {
#ifdef ARA_USE_ARCORE
    if (m_arCore) {
        m_arCore->update();

        // If display rotation changed (also includes view size change), we need
        // to re-query the uv coordinates for the on-screen portion of the
        // camera image.
        m_geometry_changed = 0;
        ArFrame_getDisplayGeometryChanged(m_arCore->getSession(), m_arCore->getFrame(), &m_geometry_changed);
        if (m_geometry_changed != 0 || !m_uvs_initialized) {
            ArFrame_transformCoordinates2d(m_arCore->getSession(), m_arCore->getFrame(),
                                           AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES, 6, &m_Vertices[0],
                                           AR_COORDINATES_2D_TEXTURE_NORMALIZED, &m_transformed_uvs[0]);

            m_uvs_initialized = true;
        }

        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        m_backTexShdr->begin();
        m_backTexShdr->setUniform1i("tex", 0);
        m_backTexShdr->setUniform2fv("uv", &m_transformed_uvs[0], 6);

        if (m_arCore->getCamTex()) m_arCore->getCamTex()->bind(0);

        m_quad->draw();

        glEnable(GL_DEPTH_TEST);

        // focus square
        /*
        if (!m_arCore->isTracking() || (m_arCore->isTracking() &&
        m_arCore->getAnchor()))
        {
            if (!m_renderFocusSq) return;

            ArTrackingState ts = AR_TRACKING_STATE_STOPPED;
            ArAnchor_getTrackingState(m_arCore->getSession(),
        m_arCore->getAnchor(), &ts);

            if (ts == AR_TRACKING_STATE_TRACKING)
                m_arCore->getTransformMatrixFromAnchor(m_arCore->getAnchor(),
        m_arCore->getSession(), &m_focus_circle_mat);

            // render active anchor
            // m_foc_circ_pvm = m_arCore->getProjMat() * m_arCore->getViewMat()
        * m_focus_circle_mat;
            //m_focSqShdr->begin();
            //m_focSqShdr->setUniformMatrix4fv("mvp", &m_foc_circ_pvm[0][0]);

            m_quad->draw();
        }
*/

        //----------------------------------------------------------------------------------------------

        // render planes
        if (m_renderPlanes && m_arCore->getPlaneList() && m_arCore->getPlaneCount()) {
            m_planeShdr->begin();
            m_planeShdr->setUniform1i("tex", 0);
            m_triTex->bind(0);

            for (int i = 0; i < m_arCore->getPlaneCount(); ++i) {
                ArTrackableList_acquireItem(m_arCore->getSession(), m_arCore->getPlaneList(), i, &m_ar_trackable);

                auto plane = ArAsPlane(m_ar_trackable);
                ArTrackable_getTrackingState(m_arCore->getSession(), m_ar_trackable, &m_out_tracking_state);
                ArPlane_acquireSubsumedBy(m_arCore->getSession(), plane, &m_subsume_plane);

                if (m_subsume_plane) {
                    ArTrackable_release(ArAsTrackable(m_subsume_plane));
                    ArTrackable_release(m_ar_trackable);
                    continue;
                }

                if (ArTrackingState::AR_TRACKING_STATE_TRACKING != m_out_tracking_state) {
                    ArTrackable_release(m_ar_trackable);
                    continue;
                }

                drawPlane(plane);

                ArTrackable_release(m_ar_trackable);
                m_ar_trackable = nullptr;
            }

            if (m_viewMode == CsStereoViewMode::Stereo && m_arCore) {
                android_cmd_data d;
                d.x = m_arCore->getDispWidth() / 2;
                d.y = m_arCore->getDispHeight() / 2;
                m_arCore->onTouched(&d);
            }
        }

        //----------------------------------------------------------------------------------------------

        if (m_renderFocusSq) {
            ArHitResult* ah  = nullptr;
            auto         win = static_cast<UIWindow*>(s_sd->uiWindow);
            if (win) ah = m_arCore->hitPlanes(win->getWidthReal() / 2, win->getHeightReal() / 2);
            if (ah) m_arCore->getHitResultModelMat(ah, m_focus_circle_mat);

            if (m_viewMode == CsStereoViewMode::Stereo) {
                m_foc_circ_pvm[0] = m_arCore->getProjMat() *
                                    glm::translate(vec3{m_stereoRenderer.getViewEyeOffs(StereoEye::left), 0.f, 0.f}) *
                                    m_arCore->getViewMat() * m_focus_circle_mat;
            } else {
                m_foc_circ_pvm[0] = m_arCore->getProjMat() * m_arCore->getViewMat() * m_focus_circle_mat;
            }

            m_foc_circ_pvm[1] = m_arCore->getProjMat() *
                                glm::translate(vec3{-m_stereoRenderer.getViewEyeOffs(StereoEye::right), 0.f, 0.f}) *
                                m_arCore->getViewMat() * m_focus_circle_mat;

            m_focSqShdr->begin();
            m_focSqShdr->setUniformMatrix4fv("mvp", &m_foc_circ_pvm[0][0][0], 2);
            m_quad->draw();
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

#ifdef ARA_USE_ARCORE

void CsStereoFbo::drawPlane(ArPlane* plane) {
    updateForPlane(m_arCore->getSession(), plane);

    if (m_viewMode == CsStereoViewMode::Stereo) {
        m_mvp[0] = m_arCore->getProjMat() *
                   glm::translate(vec3{m_stereoRenderer.getViewEyeOffs(StereoEye::left), 0.f, 0.f}) *
                   m_arCore->getViewMat() * m_model_mat;
    } else {
        m_mvp[0] = m_arCore->getProjMat() * m_arCore->getViewMat() * m_model_mat;
    }

    m_mvp[1] = m_arCore->getProjMat() *
               glm::translate(vec3{-m_stereoRenderer.getViewEyeOffs(StereoEye::right), 0.f, 0.f}) *
               m_arCore->getViewMat() * m_model_mat;

    m_planeShdr->setUniformMatrix4fv("mvp", &m_mvp[0][0][0], m_viewMode == CsStereoViewMode::Stereo ? 2 : 1);
    m_planeShdr->setUniformMatrix4fv("model_mat", glm::value_ptr(m_model_mat));
    m_planeShdr->setUniform3fv("normal", &m_normal_vec[0]);

    m_plane->drawElements(GL_TRIANGLES);
}

void CsStereoFbo::updateForPlane(ArSession* ar_session, ArPlane* ar_plane) {
    // The following code generates a triangle mesh filling a convex polygon,
    // including a feathered edge for blending.
    //
    // The indices shown in the diagram are used in comments below.
    // _______________     0_______________1
    // |             |      |4___________5|
    // |             |      | |         | |
    // |             | =>   | |         | |
    // |             |      | |         | |
    // |             |      |7-----------6|
    // ---------------     3---------------2

    m_vertices.clear();
    m_indices.clear();

    int32_t polygon_length;
    ArPlane_getPolygonSize(ar_session, ar_plane, &polygon_length);

    if (polygon_length == 0) {
        LOGE << "PlaneRenderer::UpdatePlane, no valid plane polygon is found";
        return;
    }

    const int32_t          vertices_size = polygon_length / 2;
    std::vector<glm::vec2> raw_vertices(vertices_size);
    ArPlane_getPolygon(ar_session, ar_plane, glm::value_ptr(raw_vertices.front()));

    // Fill vertex 0 to 3. Note that the vertex.xy are used for x and z
    // position. vertex.z is used for alpha. The outer polygon's alpha
    // is 0.
    for (int32_t i = 0; i < vertices_size; ++i)
        m_vertices.emplace_back(glm::vec3{raw_vertices[i].x, raw_vertices[i].y, 0.f});

    ScopedArPose scopedArPose(ar_session);
    ArPlane_getCenterPose(ar_session, ar_plane, scopedArPose.GetArPose());
    ArPose_getMatrix(ar_session, scopedArPose.GetArPose(), glm::value_ptr(m_model_mat));
    m_normal_vec = getPlaneNormal(ar_session, scopedArPose.GetArPose());

    // Fill vertex 4 to 7, with alpha set to 1.
    for (int32_t i = 0; i < vertices_size; ++i) {
        // Vector from plane center to current point.
        glm::vec2       v        = raw_vertices[i];
        const float     scale    = 1.0f - std::min((kFeatherLength / glm::length(v)), kFeatherScale);
        const glm::vec2 result_v = scale * v;

        m_vertices.push_back(glm::vec3(result_v.x, result_v.y, 1.0f));
    }

    const int32_t vertices_length      = m_vertices.size();
    const int32_t half_vertices_length = vertices_length / 2;

    // Generate triangle (4, 5, 6) and (4, 6, 7).
    for (int i = half_vertices_length + 1; i < vertices_length - 1; ++i) {
        m_indices.push_back(half_vertices_length);
        m_indices.push_back(i);
        m_indices.push_back(i + 1);
    }

    // Generate triangle (0, 1, 4), (4, 1, 5), (5, 1, 2), (5, 2, 6),
    // (6, 2, 3), (6, 3, 7), (7, 3, 0), (7, 0, 4)
    for (int i = 0; i < half_vertices_length; ++i) {
        m_indices.push_back(i);
        m_indices.push_back((i + 1) % half_vertices_length);
        m_indices.push_back(i + half_vertices_length);

        m_indices.push_back(i + half_vertices_length);
        m_indices.push_back((i + 1) % half_vertices_length);
        m_indices.push_back((i + half_vertices_length + 1) % half_vertices_length + half_vertices_length);
    }

    m_plane->resize(m_vertices.size());
    m_plane->upload(CoordType::Position, &m_vertices[0][0], m_vertices.size());
    m_plane->setElemIndices(m_indices.size(), &m_indices[0]);
}

glm::vec3 CsStereoFbo::getPlaneNormal(ArSession* ar_session, ArPose* plane_pose) {
    float plane_pose_raw[7] = {0.f};
    ArPose_getPoseRaw(ar_session, plane_pose, plane_pose_raw);
    glm::quat plane_quaternion(plane_pose_raw[3], plane_pose_raw[0], plane_pose_raw[1], plane_pose_raw[2]);
    // Get normal vector, normal is defined to be positive Y-position in local
    // frame.
    return glm::rotate(plane_quaternion, glm::vec3(0., 1.f, 0.));
}

#endif

void CsStereoFbo::postRender(renderPass pass, float* extDrawMatr) {
    for (const auto& it : s_shaderProto | views::values) {
        it->postRender(pass);
    }

    if (pass == GLSG_SCENE_PASS) {
        renderFbos(extDrawMatr);
    }
}

void CsStereoFbo::renderFbos(float* extDrawMatr) {
    if (m_viewMode == CsStereoViewMode::Stereo) {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_layerTexShdr->begin();

    if (extDrawMatr) {
        m_layerTexShdr->setUniformMatrix4fv("m_pvm", extDrawMatr);
    } else {
        m_layerTexShdr->setIdentMatrix4fv("m_pvm");
    }

    m_layerTexShdr->setUniform1i("tex", 0);
    m_layerTexShdr->setUniform1i("useBorder", 0);

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)
    if (m_arCore) m_layerTexShdr->setUniform1f("texAsp", m_arCore->getDispWidth() / m_arCore->getDispHeight());
#else
    m_layerTexShdr->setUniform1f("texAsp", m_stereoViewAspect);
#endif

    m_layerTexShdr->setUniform1f("stereoAsp", m_stereoViewAspect);
    m_layerTexShdr->setUniform1i("viewMode", static_cast<int>(m_viewMode));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_fbo.getColorImg());

    if (m_viewMode == CsStereoViewMode::Stereo) {
        for (GLsizei i = 0; i < s_cam.size(); i++) {
            m_layerTexShdr->setUniform1f("layerNr", static_cast<float>(i));
            m_layerTexShdr->setUniform2f("centre", !i ? 0.1f : -0.1f, 0.0f);
            m_layerTexShdr->setUniformMatrix4fv("trans", m_stereoRenderer.getEyeLensTrans((StereoEye)i));
            m_quad->draw();
        }
    } else {
        m_layerTexShdr->setUniform1f("layerNr", 0.f);
        m_layerTexShdr->setIdentMatrix4fv("trans");
        m_quad->draw();
    }
}

void CsStereoFbo::setViewport(uint x, uint y, uint width, uint height, bool resizeProto) {
    CameraSet::setViewport(x, y, width, height, resizeProto);

    int i = 0;
    for (const auto& c : s_cam | views::keys) {
        if (c) {
            c->setViewport({i * width / 2, 0, width / 2, height});
            c->setScreenSize(width / 2, height);
        }
        ++i;
    }

    rebuildFbo();
    uptStereoRender();

    m_stereoViewAspect = m_stereoRenderer.getFov(StereoEye::left)[0] / m_stereoRenderer.getFov(StereoEye::left)[2];
}

void CsStereoFbo::setViewMode(CsStereoViewMode mode) {
    m_viewMode = mode;
    int nrCam  = mode == CsStereoViewMode::Stereo ? 2 : 1;
#ifdef ARA_USE_ARCORE
    lockOrientation();
    buildViewMatrixArrays();
    initBackTexShdr(nrCam);
    initPlaneShdr(nrCam);
    initFocSquareShdr(nrCam);
#endif
}

#if defined(__ANDROID__) && defined(ARA_USE_ARCORE)
void CsStereoFbo::lockOrientation() {
    auto win = static_cast<UIWindow*>(s_sd->uiWindow);

    if (m_viewMode == CsStereoViewMode::Stereo && !m_orientationLocked) {
        // if in stereo mode, fix orientation to landscape
        if (win->getApplicationHandle() && win->getApplicationHandle()->m_cmd_data.oriCb)
            win->getApplicationHandle()->m_cmd_data.oriCb(1);

        m_orientationLocked = true;

    } else if (m_viewMode == CsStereoViewMode::Single && m_orientationLocked) {
        if (win->getApplicationHandle() && win->getApplicationHandle()->m_cmd_data.resetOri)
            win->getApplicationHandle()->m_cmd_data.resetOri();

        m_orientationLocked = false;
    }
}
#endif

}  // namespace ara
