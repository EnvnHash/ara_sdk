//
// Created by sven on 7/2/22.
//
#ifdef ARA_USE_ARCORE

#include "ARCoreCam.h"

#include "UIApplication.h"

using namespace glm;
using namespace std;
using namespace ara::cap;

namespace ara {

ARCoreCam::ARCoreCam() : Image() { setName(getTypeName<ARCoreCam>()); }

ARCoreCam::ARCoreCam(std::string&& styleClass) : Image(std::move(styleClass)) { setName(getTypeName<ARCoreCam>()); }

void ARCoreCam::createArSession() {
    auto app = (UIApplication*)getApp();
    m_arCore->onResume(&app->m_cmd_data);
}

void ARCoreCam::init() {
    Image::init();

    m_arCore->init(m_glbase);
    createArSession();

    // hook up android app callbacks
    auto app = (UIApplication*)getApp();
    app->m_appStateCbs[android_app_cmd::onPause].emplace_back([this](android_cmd_data* cd) { m_arCore->onPause(cd); });
    app->m_appStateCbs[android_app_cmd::onResume].emplace_back(
        [this](android_cmd_data* cd) { m_arCore->onResume(cd); });

    m_dataPath = getSharedRes()->dataPath.string();
    m_win      = getWindow();

    initGLRes(1);
}

void ARCoreCam::initGLRes(int nrLayers) {
    m_camShdr   = getCamShdr(nrLayers);
    m_planeShdr = getPlaneShdr();
    m_focSqShdr = getFocSquareShdr();

    m_bgQuad = make_unique<VAO>("position:2f,texCoord:2f", GL_DYNAMIC_DRAW);
    m_plane  = make_unique<VAO>("position:3f", GL_DYNAMIC_DRAW);
    m_quad   = make_unique<Quad>(QuadInitParams{-0.05f, -0.05f, 0.1f, 0.1f});

    m_triTex = std::make_unique<Texture>(m_glbase);
    m_triTex->loadTexture2D(m_dataPath + "/trigrid.png");
}

Shaders* ARCoreCam::getCamShdr(int nrLayers) {
    std::string vert = STRINGIFY(layout(location = 0) in vec2 position;\n
        layout(location = 2) in vec2 texCoord; \n
        out VS_GS { \n
            vec2 tex_coord; \n
        } vout;\n
        void main() { \n
            vout.tex_coord = texCoord; \n
            gl_Position = vec4(position, 0.0, 1.0);\n
        });

    vert = m_shCol->getShaderHeader() + "// ARCoreCam cam shader, vert\n" + vert;

    std::string shdr_Header_g = m_shCol->getShaderHeader() +
                                "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\n";

    std::string geom = STRINGIFY(
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
                gl_Position = gl_in[i].gl_Position; \n
                EmitVertex(); \n
            }\n
            EndPrimitive(); \n
    });

    geom = shdr_Header_g + "// ARCoreCam cam shader, geom\n" + geom;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        uniform samplerExternalOES sTexture;
        in GS_FS {\n
            vec2 tex_coord; \n
        } vin;\n
        void main(){ \n
            fragColor = texture(sTexture, vin.tex_coord); \n
        });

    frag =
        "#version 320 es\n#extension GL_OES_EGL_image_external : "
        "require\n#extension GL_OES_EGL_image_external_essl3 : require\n "
        "precision highp float;\n // ARCoreCam cam shader, frag\n" +
        frag;

    return m_shCol->add("ARCoreCam", vert, geom, frag);
}

Shaders* ARCoreCam::getPlaneShdr() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec3 position;\n

        out VS_FS { \n
            vec2 tex_coord; \n
            float alpha;
        } vout;\n

        uniform mat4 mvp;
        uniform mat4 model_mat;
        uniform vec3 normal;

        void main() {
            // Vertex Z value is used as the alpha in this shader.
            vout.alpha = position.z;

            vec4 local_pos = vec4(position.x, 0.0, position.y, 1.0);
            gl_Position = mvp * local_pos;
            vec4 world_pos = model_mat * local_pos;

            // Construct two vectors that are orthogonal to the normal.
            // This arbitrary choice is not co-linear with either horizontal
            // or vertical plane normals.
            const vec3 arbitrary = vec3(1.0, 1.0, 0.0);
            vec3 vec_u = normalize(cross(normal, arbitrary));
            vec3 vec_v = normalize(cross(normal, vec_u));

            // Project vertices in world frame onto vec_u and vec_v.
            vout.tex_coord = vec2(dot(world_pos.xyz, vec_u), dot(world_pos.xyz, vec_v));
        });

    vert =
        "#version 320 es\nprecision highp float;\n precision highp int; \n // "
        "ARCoreCam plane shader, vert\n" +
        vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        uniform sampler2D tex;
        in VS_FS {\n
            vec2 tex_coord; \n
            float alpha;
       } vin;\n
        void main(){ \n
            fragColor = vec4(texture(tex, vin.tex_coord).r * vin.alpha); \n
        });

    frag =
        "#version 320 es\nprecision highp int;\nprecision highp float;\n // "
        "ARCoreCam plane shader, frag\n" +
        frag;

    return m_shCol->add("ARCoreCam_PlaneRend", vert, frag);
}

Shaders* ARCoreCam::getFocSquareShdr() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position;\n
        layout(location = 2) in vec2 texCoord;\n
        uniform mat4 mvp;
        out VS_FS { \n
            vec2 tex_coord; \n
        } vout;\n
        void main() {
            vout.tex_coord = texCoord; \n
            gl_Position = mvp * vec4(position.x, 0.0, position.y, 1.0);\n
        });

    vert =
        "#version 320 es\nprecision highp float;\n precision highp int; \n // "
        "ARCoreCam focus square shader, vert\n" +
        vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        in VS_FS {\n
            vec2 tex_coord; \n
        } vin;\n
        void main(){ \n
            float r = length(vin.tex_coord -0.5);
            float c = float(r < 0.5 && r > 0.35);
            fragColor = vec4(c); \n
    });

    frag =
        "#version 320 es\nprecision highp int;\nprecision highp float;\n // "
        "ARCoreCam focus square shader, frag\n" +
        frag;

    return m_shCol->add("ARCoreCam_FocusSquare", vert, frag);
}

bool ARCoreCam::draw(uint32_t* objId) {
    Image::draw(objId);
    return drawFunc(objId);
}

bool ARCoreCam::drawIndirect(uint32_t* objId) {
    Image::drawIndirect(objId);
    if (m_sharedRes && m_sharedRes->drawMan) {
        m_tempObjId = *objId;
        m_sharedRes->drawMan->pushFunc([this] {
            m_dfObjId = m_tempObjId;
            drawFunc(&m_dfObjId);
        });
    }

    return true;
}

void ARCoreCam::updateAR() {
    if (m_arCore->isInited()) {
        /*
                // check if display config has changed
                int displayRotation = getApp()->get_orientation();
                if (displayRotation != m_arCore->getDispRot()
                    || getWindow()->getWidthReal() != m_arCore->getDispWidth()
                    || getWindow()->getHeightReal() !=
           m_arCore->getDispHeight())
                {
                    m_arCore->onDisplayGeometryChanged(displayRotation,
           getWindow()->getWidthReal(), getWindow()->getHeightReal());
                }
        */
        m_arCore->update();
    }
}

bool ARCoreCam::drawFunc(uint32_t* objId) {
    updateAR();

    if (!createBgQuad()) return false;

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    /*
    // render background image
    m_camShdr->begin();
    m_camShdr->setUniform1i("sTexture", 0);
    m_arCore->getCamTex()->bind(0);
    m_bgQuad->draw(GL_TRIANGLE_STRIP);

    if (getSharedRes())
        getSharedRes()->requestRedraw = true;

    -----    if (!m_arCore->isTracking() || (m_arCore->isTracking() &&
    m_arCore->getAnchor()))
    {
        if (!m_renderFocusSq) return true;

        ArTrackingState ts = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(m_arCore->getSession(), m_arCore->getAnchor(),
    &ts);

        if (ts == AR_TRACKING_STATE_TRACKING)
            m_arCore->getTransformMatrixFromAnchor(m_arCore->getAnchor(),
    m_arCore->getSession(), &m_focus_circle_mat);

        // render active anchor
        m_foc_circ_pvm = m_arCore->getProjMat() * m_arCore->getViewMat() *
    m_focus_circle_mat;

        m_focSqShdr->begin();
        m_focSqShdr->setUniformMatrix4fv("mvp", &m_foc_circ_pvm[0][0]);
        m_quad->draw();

        return true;
    }

    m_planeShdr->begin();
    m_planeShdr->setUniform1i("tex", 0);
    m_triTex->bind(0);

    // render planes
    if (m_renderPlanes && m_arCore->getPlaneList() && m_arCore->getPlaneCount())
    {
        for (int i = 0; i < m_arCore->getPlaneCount(); ++i)
        {
            ArTrackableList_acquireItem(m_arCore->getSession(),
    m_arCore->getPlaneList(), i, &m_ar_trackable);

            auto plane = ArAsPlane(m_ar_trackable);
            ArTrackable_getTrackingState(m_arCore->getSession(), m_ar_trackable,
    &m_out_tracking_state); ArPlane_acquireSubsumedBy(m_arCore->getSession(),
    plane, &m_subsume_plane);

            if (m_subsume_plane)
            {
                ArTrackable_release(ArAsTrackable(m_subsume_plane));
                ArTrackable_release(m_ar_trackable);
                continue;
            }

            if (ArTrackingState::AR_TRACKING_STATE_TRACKING !=
    m_out_tracking_state) { ArTrackable_release(m_ar_trackable); continue;
            }

            drawPlane(plane);

            ArTrackable_release(m_ar_trackable);
            m_ar_trackable = nullptr;
        }
    }

    -----    // render pointing circle

    ArHitResult* ah = nullptr;
    if (m_win) ah = m_arCore->hitPlanes(m_win->getWidthReal() /2,
    m_win->getHeightReal() /2); if (ah) m_arCore->getHitResultModelMat(ah,
    m_focus_circle_mat);

    m_foc_circ_pvm = m_arCore->getProjMat() * m_arCore->getViewMat() *
    m_focus_circle_mat;

    m_focSqShdr->begin();
    m_focSqShdr->setUniformMatrix4fv("mvp", &m_foc_circ_pvm[0][0]);
    m_quad->draw();

    -----    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/
    return true;
}

bool ARCoreCam::createBgQuad() {
    if (!m_arCore->isInited() || !m_camShdr || !m_arCore->getCamTex()) return false;

    // If display rotation changed (also includes view size change), we need to
    // re-query the uv coordinates for the on-screen portion of the camera
    // image.
    if (m_arCore->getDisplayGeomChanged() != 0 || !m_uvs_initialized) {
        ArFrame_transformCoordinates2d(m_arCore->getSession(), m_arCore->getFrame(),
                                       AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES, 4, &m_Vertices[0],
                                       AR_COORDINATES_2D_TEXTURE_NORMALIZED, &m_transformed_uvs[0]);
        m_bgQuad->resize(4);
        m_bgQuad->upload(CoordType::Position, &m_Vertices[0], 4);
        m_bgQuad->upload(CoordType::TexCoord, &m_transformed_uvs[0], 4);
        m_uvs_initialized = true;
    }

    // Suppress rendering if the camera did not produce the first frame yet.
    // This is to avoid drawing possible leftover data from previous sessions if
    // the texture is reused.
    if (!m_arCore->getTimestamp()) return false;

    return true;
}

void ARCoreCam::drawPlane(ArPlane* plane) {
    updateForPlane(m_arCore->getSession(), plane);

    m_mvp = m_arCore->getProjMat() * m_arCore->getViewMat() * m_model_mat;

    m_planeShdr->setUniformMatrix4fv("mvp", &m_mvp[0][0]);
    m_planeShdr->setUniformMatrix4fv("model_mat", glm::value_ptr(m_model_mat));
    m_planeShdr->setUniform3fv("normal", &m_normal_vec[0]);

    m_plane->drawElements(GL_TRIANGLES);
}

void ARCoreCam::updateForPlane(ArSession* ar_session, ArPlane* ar_plane) {
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

glm::vec3 ARCoreCam::getPlaneNormal(ArSession* ar_session, ArPose* plane_pose) {
    float plane_pose_raw[7] = {0.f};
    ArPose_getPoseRaw(ar_session, plane_pose, plane_pose_raw);
    glm::quat plane_quaternion(plane_pose_raw[3], plane_pose_raw[0], plane_pose_raw[1], plane_pose_raw[2]);
    // Get normal vector, normal is defined to be positive Y-position in local
    // frame.
    return glm::rotate(plane_quaternion, glm::vec3(0., 1.f, 0.));
}

}  // namespace ara

#endif
