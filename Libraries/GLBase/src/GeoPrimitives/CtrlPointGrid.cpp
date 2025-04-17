
#include "CtrlPointGrid.h"

#include <GLBase.h>
#include <GLRenderer.h>
#include <Utils/FBO.h>
#include <Utils/VAO.h>
#include <glb_common/catmull_centri.h>

using namespace glm;
using namespace std;

namespace ara {

CtrlPointGrid::CtrlPointGrid(ShaderCollector &shCol)
    : m_shCol(shCol), m_gridSize(2, 2), m_ipolMode(interpolM::CatmullRomCentri), m_epolMode(extrapolM::Mirror),
      m_l1_R(mat4(1.f)), m_l1_modRot(mat4(1.f)), m_l1_postTrans(mat4(1.f)), m_patchResMax(128, 128), m_tessBaseRes(20),
      m_gridLineVaoSize(glm::ivec2(0, 0)), m_tessMode(tessMode::CPU) {
    m_ctrlPoints.resize(2);

    // build Level 1 ctrlPoints index[0]
    // define in counter-clockwise starting from left/bottom
    for (auto i = 0; i < 4; i++) {
        m_ctrlPoints[0].emplace_back();
        m_ctrlPoints[0].back().refPosition = vec2(i == 0 || i == 3 ? -1.f : 1.f, i < 2 ? -1.f : 1.f);
        for (auto j = 0; j < 2; j++) m_ctrlPoints[0].back().position[j] = m_ctrlPoints[0].back().refPosition[j];
    }

    // build level 2 ctrlPoints index[1]
    for (int y = 0; y < m_gridSize.y; y++)
        for (int x = 0; x < m_gridSize.x; x++) {
            m_ctrlPoints[1].emplace_back();
            m_ctrlPoints[1].back().refPosition = vec2((float)x / (float)(m_gridSize.x - 1) * 2.f - 1.f,
                                                      (float)y / (float)(m_gridSize.y - 1) * 2.f - 1.f);
        }

    for (auto &i : m_ctrlPoints[1]) i.position = i.refPosition;

    extrapolBorder();

    m_level1      = glm::mat4(1.f);
    m_l1_axis_mat = glm::mat4(1.f);

    initXmlVals();

    initWarpOffsUVDrawShdr();
    tesselate();  // do a initial tesselation to be able to fill an external
                  // uv-s_fbo when added
}

void CtrlPointGrid::initWarpOffsUVDrawShdr() {
#ifndef __EMSCRIPTEN__
    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n uniform mat4 perspMat; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n
            vec3 toScreen(vec2 inPoint, mat4 distMat) {
                \n vec3 outP   = vec3(inPoint.x, inPoint.y,
                                      max(inPoint.x * distMat[0][2] + inPoint.y * distMat[1][2] + distMat[2][2], 1e-4));
                \n      outP.x = (inPoint.x * distMat[0][0] + inPoint.y * distMat[1][0] + distMat[2][0]) / outP.z;
                \n      outP.y = (inPoint.x * distMat[0][1] + inPoint.y * distMat[1][1] + distMat[2][1]) / outP.z;
                \n return outP;
                \n
            }\n void main() {
                \n vec3 sp     = toScreen(position.xy, perspMat);
                \n vec4 outPos = m_pvm * vec4(sp.x, -sp.y, 0.0, 1.0);
                \n  // we want the z coordinate not being affected by the gui
                    // matrices
                    tex_coord   = texCoord;
                \n  tex_coord.y = 1.0 - tex_coord.y;
                gl_Position     = outPos;

                // gl_Position.z = (sp.z - 0.9999) / 99.0; \n		//
                // convert to NDC not using the homography mat but a standard
                // frustum, offset a tiny but to have the axes appear in front
                // of the wg
            });
    vert = "// warpDrawOffsUVShdr, vert\n" + m_shCol.getShaderHeader() + vert;

    string frag = STRINGIFY(in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n void main() {
        \n color = vec4(tex_coord, 1.0, 1.0);
        \n
    });

    frag = "// warpDrawOffsUVShdr, frag\n" + m_shCol.getShaderHeader() + frag;

#else
    string vert = STRINGIFY(
        precision mediump float; attribute vec4 position; attribute vec2 texCoord; uniform mat4 m_pvm;
        varying vec2 tex_coord; vec3 toScreen(vec2 inPoint, mat4 distMat) {
            \n vec3 outP   = vec3(inPoint.x, inPoint.y,
                                  max(inPoint.x * distMat[0][2] + inPoint.y * distMat[1][2] + distMat[2][2], 1e-4));
            \n      outP.x = (inPoint.x * distMat[0][0] + inPoint.y * distMat[1][0] + distMat[2][0]) / outP.z;
            \n      outP.y = (inPoint.x * distMat[0][1] + inPoint.y * distMat[1][1] + distMat[2][1]) / outP.z;
            \n return outP;
            \n
        }\n void main() {
            tex_coord          = texCoord;
            vec3 sp            = toScreen(position.xy, perspMat);
            \n   gl_Position   = m_pvm * vec4(sp.xy, 0.0, 1.0);
            \n   gl_Position.z = (sp.z - 1.0) / 99.0;
            \n
        });

    string frag = STRINGIFY(precision mediump float; varying vec2 tex_coord; uniform sampler2D tex;
                            uniform float bright; \n uniform float alpha; \n void main() {
                                glFragColor = texture2D(tex, tex_coord) * alpha;
                            });
#endif

    m_warpDrawOffsUVShdr = m_shCol.add("warpDrawOffsUVShdr", vert, frag);
}

void CtrlPointGrid::initSplineCoeffShdr() {
    m_catmRomCoeffFunc = STRINGIFY(SplineSegment catmullRomCentriCalcCoeff2(vec2 points[4]) {
        SplineSegment seg;
        float         dX;
        float         dY;
        float         t0;
        float         t1;
        float         t2;
        float         t3;
        float         f;
        float         alpha = 0.25;
        \n            t0    = 0.0;
        dX                  = points[1].x - points[0].x;
        dY                  = points[1].y - points[0].y;
        t1                  = t0 + max(pow((dX * dX + dY * dY), alpha), 1e-7);
        dX                  = points[2].x - points[1].x;
        dY                  = points[2].y - points[1].y;
        t2                  = t1 + max(pow((dX * dX + dY * dY), alpha), 1e-7);
        dX                  = points[3].x - points[2].x;
        dY                  = points[3].y - points[2].y;
        t3                  = t2 + max(pow((dX * dX + dY * dY), alpha), 1e-7);
        \n float t01        = t0 - t1;
        float    t02        = t0 - t2;
        float    t03        = t0 - t3;
        float    t12        = t1 - t2;
        float    t13        = t1 - t3;
        float    t21        = t2 - t1;
        float    t23        = t2 - t3;
        float    t32        = t3 - t2;
        f                   = t01 * t02 * t13 * t32;
        seg.d.x             = points[1].x;
        seg.c.x             = (t13 * t23 *
                   (points[0].x * t12 * t12 + points[1].x * t02 * (t0 - 2.0 * t1 + t2) - points[2].x * t01 * t01)) /
                  f;
        seg.b.x = (t21 * (2.0f * points[0].x * t12 * t13 * t23 + points[1].x * (t0 + t1 - 2.0 * t3) * t02 * t32 +
                          t01 * (points[2].x * (t0 + t2 - 2.0 * t3) * t13 + points[3].x * t02 * t21))) /
                  f;
        seg.a.x = (t12 * (points[0].x * t12 * t13 * t23 + points[1].x * t02 * t03 * t32 +
                          t01 * (points[2].x * t03 * t13 + points[3].x * t02 * t21))) /
                  f;
        \n seg.d.y = points[1].y;
        seg.c.y    = (t13 * t23 *
                   (points[0].y * t12 * t12 + points[1].y * t02 * (t0 - 2.0 * t1 + t2) - points[2].y * t01 * t01)) /
                  f;
        seg.b.y = (t21 * (2.0f * points[0].y * t12 * t13 * t23 + points[1].y * (t0 + t1 - 2.0 * t3) * t02 * t32 +
                          t01 * (points[2].y * (t0 + t2 - 2.0 * t3) * t13 + points[3].y * t02 * t21))) /
                  f;
        seg.a.y = (t12 * (points[0].y * t12 * t13 * t23 + points[1].y * t02 * t03 * t32 +
                          t01 * (points[2].y * t03 * t13 + points[3].y * t02 * t21))) /
                  f;
        return seg;
    }\n);

    string comp = STRINGIFY(struct SplineSegment {
                                    vec2 a;
                                    vec2 b;
                                    vec2 c;
                                    vec2 d;
                                };\n
                                        layout(std430, binding = 0) buffer SplSegHor{ SplineSegment ssHor[]; }; \n
                                        layout(std430, binding = 2) buffer Position{ vec4 pos[]; }; \n
                                        uniform ivec2 gridSize;\n
                                        \n);

    comp += m_catmRomCoeffFunc;

    comp += STRINGIFY(
        vec2 extraPol(int idx, int xDir) {
            return (xDir == 1) ? (pos[idx].xy * 2 - pos[idx - 1].xy) : (pos[idx].xy * 2 - pos[idx + 1].xy);
        }\n void main() {
            \n ivec2 gPos;
            \n int   i = int(gl_GlobalInvocationID.x);
            \n if (i >= gridSize.x * gridSize.y) return;
            \n gPos.x = i % gridSize.x;
            \n if (gPos.x == gridSize.x - 1) return;
            gPos.y = i / gridSize.x;
            \n vec2 points[4];
            \n      points[0] = gPos.x == 0 ? extraPol(i, -1) : pos[i - 1].xy;
            points[1]         = pos[i].xy;
            points[2]         = pos[i + 1].xy;
            points[3]         = (gPos.x == gridSize.x - 2) ? extraPol(i + 1, 1) : pos[i + 2].xy;
            ssHor[i]          = catmullRomCentriCalcCoeff2(points);
            \n
        });

    string shdr = m_shCol.getShaderHeader() + "layout(local_size_x=" + std::to_string((int)m_work_group_size) +
                  ", local_size_y=1,local_size_z=1) in;\n";
    shdr += comp;

    m_splineCoeffShdr = m_shCol.add("CtrlPointGrid_SplineCoeff", shdr);
}

void CtrlPointGrid::initExtraPolShdr() {
    string splineSegStruct = "struct SplineSegment{ vec2 a; vec2 b; vec2 c; vec2 d; };";

    string vert = splineSegStruct + STRINGIFY(layout(location = 0) in vec4 position;\n
                                                          layout(location = 2) in vec2 texCoord;\n
                                                          out VS_TC { \n
                                                          vec2 texCoord; \n
                                                          vec2 pos; \n
                                                          ivec2 gridPos;
                                                  } vertOut; \n
                                                          void main() { \n
                                                          vertOut.texCoord = texCoord; \n
                                                          vertOut.pos = position.xy; \n
                                                          vertOut.gridPos = ivec2(position.zw); \n
                                                          gl_Position = vec4(position.xy, 0.0, 1.0); \n
                                                  });
    vert = m_shCol.getShaderHeader() + vert;

    string cont = splineSegStruct + STRINGIFY(layout(vertices = 4) out;\n
                                                          in VS_TC { \n
                                                          vec2 texCoord; \n
                                                          vec2 pos; \n
                                                          ivec2 gridPos;
                                                  } vertIn[]; \n
                                                          out TC_TE { \n
                                                          vec2 texCoord; \n
                                                          ivec2 gridPos;
                                                  } vertOut[]; \n
                                                          \n
                                                          uniform vec2 fboSize; \n
                                                          uniform float segsPerPixel; \n
                                                          \n
                                                          void main() { \n
                                                          gl_TessLevelOuter[0] = length(vertIn[0].pos - vertIn[3].pos) * fboSize.y * segsPerPixel;  \n
                                                          gl_TessLevelOuter[1] = length(vertIn[0].pos - vertIn[2].pos) * fboSize.x * segsPerPixel; \n
                                                          gl_TessLevelOuter[2] = length(vertIn[1].pos - vertIn[2].pos) * fboSize.y * segsPerPixel; \n
                                                          gl_TessLevelOuter[3] = length(vertIn[2].pos - vertIn[3].pos) * fboSize.x * segsPerPixel; \n

                                                          gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[2]) * 0.5; \n
                                                          gl_TessLevelInner[1] = (gl_TessLevelOuter[1] + gl_TessLevelOuter[3]) * 0.5; \n

                                                          gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n
                                                          vertOut[gl_InvocationID].texCoord = vertIn[gl_InvocationID].texCoord; \n
                                                          vertOut[gl_InvocationID].gridPos = vertIn[gl_InvocationID].gridPos; \n
                                                  });
    cont = m_shCol.getShaderHeader() + cont;

    string eval = splineSegStruct + STRINGIFY(layout(quads, equal_spacing, ccw) in;
                                                          layout(std430, binding = 0) buffer SplSegHor{
                        SplineSegment ssHor[];}; \n
                                                          \n
                                                          in TC_TE { \n
                                                          vec2 texCoord; \n
                                                          ivec2 gridPos;
                                                  } vertIn[]; \n  // access to all patches vertices from the TC stage, although TE is invocated per Vertex
                                                          out TE_FS { \n
                                                          vec2 texCoord; \n
                                                  } vertOut; \n
                                                          \n);

    eval += m_catmRomCoeffFunc;

    eval += STRINGIFY(vec2 catmullRomCentriGetPoint(SplineSegment seg, float t) {
            return seg.a * t * t * t + seg.b * t * t + seg.c * t + seg.d;
        }
                                  uniform mat4 m_pvm; \n
                                  uniform mat4 perspMat;\n
                                  uniform ivec2 gridSize; \n
                                  uniform int usePerspMat; \n
                                  uniform int linearIntrp; \n
                                  vec3 toScreen(vec2 inPoint, mat4 distMat) { \n
                                  vec3 outP = vec3(inPoint.x, inPoint.y, max(inPoint.x * distMat[0][2] + inPoint.y * distMat[1][2] + distMat[2][2], 1e-4)); \n
                                  outP.x = (inPoint.x * distMat[0][0] + inPoint.y * distMat[1][0] + distMat[2][0]) / outP.z; \n
                                  outP.y = (inPoint.x * distMat[0][1] + inPoint.y * distMat[1][1] + distMat[2][1]) / outP.z; \n
                                  return outP; \n
                          }\n
                                  vec2 getCatmIntrpPoint(){ \n
                                  vec2 points[4];
                                  points[1] = catmullRomCentriGetPoint(ssHor[vertIn[0].gridPos.x + vertIn[0].gridPos.y * gridSize.x], gl_TessCoord.x);
                                  points[2] = catmullRomCentriGetPoint(ssHor[vertIn[3].gridPos.x + vertIn[3].gridPos.y * gridSize.x], gl_TessCoord.x);

                                  // calculate a third point, which would be the next horizontal segment in +y direction
                                  points[0] = vertIn[0].gridPos.y == 0 ? (points[1] * 2 - points[2])
                                  : catmullRomCentriGetPoint(ssHor[vertIn[0].gridPos.x + (vertIn[0].gridPos.y-1) * gridSize.x], gl_TessCoord.x);
                                  points[3] = vertIn[3].gridPos.y == (gridSize.y -1) ? (points[2] * 2 - points[1])
                                  : catmullRomCentriGetPoint(ssHor[vertIn[3].gridPos.x + (vertIn[3].gridPos.y+1) * gridSize.x], gl_TessCoord.x);

                                  SplineSegment seg = catmullRomCentriCalcCoeff2(points);
                                  vec2 pos = catmullRomCentriGetPoint(seg, gl_TessCoord.y);
                                  vec3 sp = toScreen(pos, perspMat);\n
                                  return sp.xy;
                          }\n
                                  void main() { \n
                                  gl_Position = m_pvm * (
                                  bool(linearIntrp) ?
                                  mix( \n
                                  mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x), \n
                                  mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x), \n
                                  gl_TessCoord.y)
                                  : vec4(getCatmIntrpPoint(), 0.0, 1.0)
                          ); \n

                                  vertOut.texCoord = mix( \n
                                  mix(vertIn[0].texCoord, vertIn[1].texCoord, gl_TessCoord.x), \n
                                  mix(vertIn[3].texCoord, vertIn[2].texCoord, gl_TessCoord.x), \n
                                  gl_TessCoord.y); \n
                          });
    eval = m_shCol.getShaderHeader() + eval;

    string frag = splineSegStruct + STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                                          in TE_FS { \n
                                                          vec2 texCoord; \n
                                                  } vertIn; \n  // access to all patches vertices from the TC stage, although TE is invocated per Vertex
                                                          uniform sampler2D tex;\n
                                                          uniform int asUvMap;\n
                                                          void main() { \n
                                                          fragColor = bool(asUvMap) ? vec4(vertIn.texCoord.x, 1.0 - vertIn.texCoord.y, 1.0, 1.0) : texture(tex, vertIn.texCoord); \n
                                                  });
    frag = m_shCol.getShaderHeader() + frag;

    m_tessShader = m_shCol.add("CtrlPointGrid", vert, cont, eval, std::string(), frag);
}

void CtrlPointGrid::calcLineGrippers() {
    // we just have to find the index into the vertices array
    // there the interpolated values already exist
    uint32_t srcInd, dstInd, patchInd, offs;

    // resize the gripper array if necessary
    uint32_t lineGripperSize = (m_gridSize.x - 1) * m_gridSize.y + (m_gridSize.y - 1) * m_gridSize.x;
    if (m_lineGripper.size() != lineGripperSize) m_lineGripper.resize(lineGripperSize);

    // run through the ctrlPoints[1] from bottom to top
    // process the horizontal lines
    for (auto y = 0; y < m_gridSize.y; y++) {
        for (auto x = 0; x < m_gridSize.x - 1; x++) {
            dstInd = y * (m_gridSize.x - 1) + y * (m_gridSize.x) + x;

            // calculate the intermediate point between two consequtive
            // ctrlPoints[1]
            m_lineGripper[dstInd].refGridPos[0] = y * m_gridSize.x + x;
            m_lineGripper[dstInd].refGridPos[1] = y * m_gridSize.x + x + 1;
            m_lineGripper[dstInd].patchInd      = getGridPosPatchInd(x, y);
            m_lineGripper[dstInd].position =
                vec2(m_vertices[getGridPosVInd(x, y) + getPatchNrSeg(m_lineGripper[dstInd].patchInd).x / 2]);
            m_lineGripper[dstInd].refPosition =
                vec2((float)(x * 2 + 1) / (float)((m_gridSize.x - 1) * 2), (float)y / (float)(m_gridSize.y - 1));
            m_lineGripper[dstInd].setMouseIcon(MouseIcon::vresize);
            m_lineGripper[dstInd].mDir = CtrlPoint::UI_CTRL_VERT;
        }
    }

    //  process the vertical lines
    for (auto x = 0; x < m_gridSize.x; x++) {
        for (auto y = 0; y < m_gridSize.y - 1; y++) {
            dstInd = (y + 1) * (m_gridSize.x - 1) + y * m_gridSize.x + x;

            // in the last column we need to decrement x since there are
            // only (gridSize.x-1) * (gridSize.y-1) patches
            patchInd = getGridPosPatchInd(x, y);
            offs     = x < (m_gridSize.x - 1) ? 0 : getPatchNrSeg(patchInd).x;
            srcInd   = m_patch_ind_offs[patchInd] + getPatchNrSeg(patchInd).y / 2 * getPatchNrVert(patchInd).x + offs;

            m_lineGripper[dstInd].refGridPos[0] = y * m_gridSize.x + x;
            m_lineGripper[dstInd].refGridPos[1] = (y + 1) * m_gridSize.x + x;
            m_lineGripper[dstInd].patchInd      = getGridPosPatchInd(x, y);
            m_lineGripper[dstInd].refPosition =
                vec2((float)x / (float)(m_gridSize.x - 1), (float)(y * 2 + 1) / (float)((m_gridSize.y - 1) * 2));
            m_lineGripper[dstInd].position = vec2(m_vertices[srcInd]);
            m_lineGripper[dstInd].setMouseIcon(MouseIcon::hresize);
            m_lineGripper[dstInd].mDir = CtrlPoint::UI_CTRL_HORI;
        }
    }
}

void CtrlPointGrid::extrapolBorder() {
    uint32_t baseInd, ind, dstInd;

    // create a new vector<vec2> with extrapolated control points on each side
    if (m_ctrlPointsExtra.size() != (m_gridSize.x + 2) * (m_gridSize.y + 2))
        m_ctrlPointsExtra.resize((m_gridSize.x + 2) * (m_gridSize.y + 2));

    // first run through all rows, copy the points and add a point at the
    // beginning and end
    for (int y = 0; y < m_gridSize.y; y++) {
        for (int x = 0; x < m_gridSize.x + 2; x++) {
            ind    = y * m_gridSize.x + x;
            dstInd = (y + 1) * (m_gridSize.x + 2) + x;

            if (x == 0)
                m_ctrlPointsExtra[dstInd] = 2.f * m_ctrlPoints[1][ind].position - m_ctrlPoints[1][ind + 1].position;
            else if (x != 0 && x != m_gridSize.x + 1)
                m_ctrlPointsExtra[dstInd] = m_ctrlPoints[1][ind - 1].position;
            else
                m_ctrlPointsExtra[dstInd] = 2.f * m_ctrlPoints[1][ind - 2].position - m_ctrlPoints[1][ind - 3].position;

            // normalize to [0,1]
            m_ctrlPointsExtra[dstInd] = m_ctrlPointsExtra[dstInd] * 0.5f + 0.5f;
        }
    }

    // now insert a line at the top and at the bottom
    for (int x = 0; x < m_gridSize.x + 2; x++)
        m_ctrlPointsExtra[x] =
            2.f * m_ctrlPointsExtra[x + (m_gridSize.x + 2)] - m_ctrlPointsExtra[x + (m_gridSize.x + 2) * 2];

    for (int x = 0; x < m_gridSize.x + 2; x++) {
        baseInd = (m_gridSize.x + 2) * (m_gridSize.y - 1);
        m_ctrlPointsExtra[baseInd + (m_gridSize.x + 2) * 2 + x] =
            2.f * m_ctrlPointsExtra[baseInd + x + m_gridSize.x + 2] - m_ctrlPointsExtra[baseInd + x];
    }
}

void CtrlPointGrid::tesselate(tessMode tMode, float *pvm, float *perspMat, FBO *uvFbo) {
    if (tMode == tessMode::CPU)
        tesselateCPU(uvFbo);
    else
        tesselateGPU(uvFbo, pvm, perspMat);
}

void CtrlPointGrid::tesselateGPU(FBO *uvFbo, float *pvm, float *perspMat) {
    createBaseGridVao();

    int gridTotSize = m_gridSize.x * m_gridSize.y;
    if (!m_splineSegHor)
        m_splineSegHor = new ShaderBuffer<SplineSegment>(gridTotSize);
    else if (m_splineSegHor->getSize() != gridTotSize)
        m_splineSegHor->resize(gridTotSize);

    // calculate spline coeficients for each point
    if (!m_splineCoeffShdr) initSplineCoeffShdr();

    m_splineCoeffShdr->begin();
    m_splineCoeffShdr->bindProgramPipeline();
    m_splineCoeffShdr->setUniform2i("gridSize", m_gridSize.x, m_gridSize.y);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_splineSegHor->getBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_baseMeshVaoGPU->getVBO(CoordType::Position));

    // round up
    auto nrExecutions = (GLuint)std::ceil((float)(m_gridSize.x * m_gridSize.y) / m_work_group_size);
    glDispatchCompute(nrExecutions, 1, 1);

    // We need to block here on compute completion to ensure that the
    // computation is done before we render
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);

    ara::Shaders::unbindProgramPipeline();
    ara::Shaders::end();

    rebuildGridLineVao(m_baseMeshVaoGPU, &m_lines_indices, &m_lines_indices_counts);

    if (uvFbo) updateUvFboGpu(uvFbo, pvm, perspMat);
}

void CtrlPointGrid::runTessShader(float *pvm, int texId, float fboWidth, float fboHeight, float *perspMat) {
#ifndef ARA_USE_GLES31
    if (!m_baseMeshVaoGPU) return;
    if (!m_tessShader) initExtraPolShdr();

    glPatchParameteri(GL_PATCH_VERTICES, 4);

    m_tessShader->begin();
    m_tessShader->setUniform1i("linearIntrp", int(m_ipolMode == interpolM::Bilinear));
    m_tessShader->setUniform2i("gridSize", m_gridSize.x, m_gridSize.y);
    m_tessShader->setUniformMatrix4fv("perspMat", &m_level1[0][0]);

    if (pvm)
        m_tessShader->setUniformMatrix4fv("m_pvm", pvm);
    else
        m_tessShader->setIdentMatrix4fv("m_pvm");

    m_tessShader->setUniform2f("fboSize", fboWidth, fboHeight);
    m_tessShader->setUniform1f("segsPerPixel", 0.75f);

    if (texId > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        m_tessShader->setUniform1i("tex", 0);
        m_tessShader->setUniform1i("asUvMap", 0);
    } else {
        m_tessShader->setUniform1i("asUvMap", 1);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_splineSegHor->getBuffer());

    m_baseMeshVaoGPU->enableVertexAttribs();
    m_baseMeshVaoGPU->bindElementBuffer();

    glDrawElements(GL_PATCHES, (int)m_baseIndices.size(), GL_UNSIGNED_INT, nullptr);

    ara::VAO::unbindElementBuffer();
    m_baseMeshVaoGPU->disableVertexAttribs();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

    ara::Shaders::end();
#endif
}

void CtrlPointGrid::createBaseGridVao()  // only used when tessMode::GPU
{
    if (m_baseVertices.size() != (m_gridSize.x * m_gridSize.y)) {
        m_baseVertices.resize(m_gridSize.x * m_gridSize.y);
        m_baseTexCoords.resize(m_gridSize.x * m_gridSize.y);
    }

    uint32_t nrVertices = (m_gridSize.x - 1) * (m_gridSize.y - 1) * 4;
    if (m_baseIndices.size() != nrVertices) m_baseIndices.resize(nrVertices);

    uint32_t ind;
    for (int y = 0; y < m_gridSize.y; y++)
        for (int x = 0; x < m_gridSize.x; x++) {
            ind                  = x + y * m_gridSize.x;
            m_baseVertices[ind]  = vec4(m_ctrlPoints[1][ind].position, (float)x, (float)y);
            m_baseTexCoords[ind] = m_ctrlPoints[1][ind].refPosition * 0.5f + 0.5f;
        }

    ind = 0;
    for (int y = 0; y < m_gridSize.y - 1; y++)
        for (int x = 0; x < m_gridSize.x - 1; x++) {
            m_baseIndices[ind++] = x + y * m_gridSize.x;
            m_baseIndices[ind++] = x + y * m_gridSize.x + 1;
            m_baseIndices[ind++] = x + (y + 1) * (m_gridSize.x) + 1;
            m_baseIndices[ind++] = x + (y + 1) * (m_gridSize.x);
        }

    // update opengl buffers
    if (!m_baseMeshVaoGPU) m_baseMeshVaoGPU = new VAO("position:4f,texCoord:2f", GL_DYNAMIC_DRAW);

    // OpenGL Buffers can't be resized to we have to delete and rebuild them
    if (m_baseMeshVaoGPU->getNrVertices() < m_baseVertices.size())
        m_baseMeshVaoGPU->resize((GLuint)m_baseVertices.size());

    m_baseMeshVaoGPU->upload(CoordType::Position, &m_baseVertices[0][0], (uint32_t)m_baseVertices.size());
    m_baseMeshVaoGPU->upload(CoordType::TexCoord, &m_baseTexCoords[0][0], (uint32_t)m_baseTexCoords.size());
    m_baseMeshVaoGPU->setElemIndices((GLuint)m_baseIndices.size(), &m_baseIndices[0]);
}

void CtrlPointGrid::tesselateCPU(FBO *uvFbo) {
    m_vertices.resize(0);
    m_texCoords.resize(0);
    m_indices.resize(0);
    m_patch_ind_offs.resize(0);

    vector<vector<float>> horline_seg_length;
    vector<float>         horline_tot_length;
    vector<vector<float>> vertline_seg_length;
    vector<float>         vertline_tot_length;

    uint32_t patchInd;
    uint32_t vBufIndOffs = 0;

    vec2 patchSize;
    vec2 initPatchSize;
    vec2 initPatchOffs;
    vec2 interpCoord;
    vec2 hv_iPoint;
    vec2 i_points[4];
    vec2 relPos;

    extrapolBorder();

    // make sure the sizes of the vectors correspond to the actual
    // grid resolution
    if (m_patchRes.size() != (m_gridSize.y - 1) * (m_gridSize.x - 1))
        m_patchRes.resize((m_gridSize.y - 1) * (m_gridSize.x - 1));

    // run through all warpoints / patches, row by row
    // we are rendering indexed buffers, so we only generate Corner-Points and
    // save respective indices in the same pass we also generate indices for the
    // grid lines for emscripten we can't use Primitive restarting, so we need
    // seperate Element Buffers for each line first estimate the size of the
    // resulting patches
    m_nrMeshVertices = 0;
    m_nrMeshIndices  = 0;

#pragma omp parallel for
    for (int y = 0; y < m_gridSize.y - 1; y++) {
        for (int x = 0; x < m_gridSize.x - 1; x++) {
            // aproximate patch size to estimate nr of SubDivisions
            uint32_t ind = x + m_gridSize.x * y;
            patchInd     = y * (m_gridSize.x - 1) + x;

            float medDiag = (length(m_ctrlPoints[1][ind + m_gridSize.x + 1].position - m_ctrlPoints[1][ind].position) +
                             length(m_ctrlPoints[1][ind + m_gridSize.x].position - m_ctrlPoints[1][ind + 1].position)) *
                            0.5f;

            patchSize = vec2(medDiag, medDiag);

            // patch Res should be always even, so that we can take the line
            // grippers from the vertices array
            m_patchRes[patchInd] = ivec2(std::min((uint32_t)std::max(patchSize.x * (float)m_tessBaseRes, 2.f) / 2 * 2,
                                                  (uint32_t)m_patchResMax.x),
                                         std::min((uint32_t)std::max(patchSize.y * (float)m_tessBaseRes, 2.f) / 2 * 2,
                                                  (uint32_t)m_patchResMax.y));

            m_nrMeshVertices += (m_patchRes[patchInd].x + 1) * (m_patchRes[patchInd].y + 1);
            m_nrMeshIndices += m_patchRes[patchInd].x * m_patchRes[patchInd].y * 6;
        }
    }

#pragma omp parallel for
    for (int y = 0; y < m_gridSize.y - 1; y++) {
#pragma omp parallel for
        for (int x = 0; x < m_gridSize.x - 1; x++) {
            uint32_t ind = x + m_gridSize.x * y;
            patchInd     = y * (m_gridSize.x - 1) + x;

            initPatchSize =
                vec2((m_ctrlPoints[1][ind + 1].refPosition.x - m_ctrlPoints[1][ind].refPosition.x) * 0.5f,
                     (m_ctrlPoints[1][ind + m_gridSize.x].refPosition.y - m_ctrlPoints[1][ind].refPosition.y) * 0.5f);
            initPatchOffs = vec2(m_ctrlPoints[1][ind].refPosition * 0.5f + 0.5f);

            // for the generation of the grid-lines element buffer we need
            // the vertices index offset to the beginning of each patch
            if (x == 0 && y == 0) m_patch_ind_offs.emplace_back(0);

            if (!(x == m_gridSize.x - 2 && y == m_gridSize.y - 2))
                m_patch_ind_offs.emplace_back(m_patch_ind_offs.back() +
                                              (m_patchRes[patchInd].x + 1) * (m_patchRes[patchInd].y + 1));

            // --------- linear Interpolation -----------
            if (m_ipolMode == interpolM::Bilinear) {
                // run through the path row by row
                for (int py = 0; py < m_patchRes[patchInd].y + 1; py++) {
                    // subdivide each row
                    // first we interpolate along the x-axis two points on the
                    // upper and lower border of the patch at the relative
                    // x-position then we interpolate between these value by the
                    // relative y-position
                    for (int px = 0; px < m_patchRes[patchInd].x + 1; px++) {
                        relPos =
                            vec2((float)px / (float)m_patchRes[patchInd].x, (float)py / (float)m_patchRes[patchInd].y);

                        i_points[0] = mix(m_ctrlPoints[1][ind].position, m_ctrlPoints[1][ind + 1].position, relPos.x);
                        i_points[1] = mix(m_ctrlPoints[1][ind + m_gridSize.x].position,
                                          m_ctrlPoints[1][ind + m_gridSize.x + 1].position, relPos.x);
                        interpCoord = mix(i_points[0], i_points[1], relPos.y);

                        m_vertices.emplace_back(interpCoord, 0.f, 1.f);
                        m_texCoords.emplace_back(relPos * initPatchSize + initPatchOffs);

                        if (px < m_patchRes[patchInd].x && py < m_patchRes[patchInd].y) {
                            uint32_t baseInd = (uint32_t)m_vertices.size() - 1;

                            // two counter clockwise triangles from a quad,
                            // start at lower left corner, upper right corner is
                            // both last point from first triangle and first
                            // points of second triangle
                            m_indices.emplace_back(baseInd);
                            m_indices.emplace_back(baseInd + 1);
                            for (int i = 0; i < 2; i++) m_indices.emplace_back(baseInd + m_patchRes[patchInd].x + 1);
                            m_indices.emplace_back(baseInd + 1);
                            m_indices.emplace_back(baseInd + m_patchRes[patchInd].x + 2);
                        }
                    }
                }
            } else {
                // ----------- Bicubic interpolation ------------
                // generate the coefficiantes for catmull-rom interpolation
                // for the four consecutive vertical grid lines
                m_gridPosShift = ivec2(x + 1, y + 1);
                for (int i = 0; i < 4; i++) {
                    vec2 *points[4] = {
                        &m_ctrlPointsExtra[m_gridPosShift.x - 1 + i + (m_gridPosShift.y - 1) * (m_gridSize.x + 2)],
                        &m_ctrlPointsExtra[m_gridPosShift.x - 1 + i + (m_gridPosShift.y) * (m_gridSize.x + 2)],
                        &m_ctrlPointsExtra[m_gridPosShift.x - 1 + i + (m_gridPosShift.y + 1) * (m_gridSize.x + 2)],
                        &m_ctrlPointsExtra[m_gridPosShift.x - 1 + i + (m_gridPosShift.y + 2) * (m_gridSize.x + 2)]};
                    catmullRomCentriCalcCoeff2(m_seg[i], (vec2 **)&points);
                }

                // generate vertices and indices for this patch
                // we need +1 more columns and rows to completely define each
                // segment of each patch
                //
                // to calculate the texture coordinates since we are using
                // centripetal catmull-rom, our coordinates are non equidistant,
                // so get the correct texturecoordinate we need to calculate the
                // length of each interpolated grid lines (horizontal and
                // vertical) and calculate the texture coordinate as a relation
                // between the total length of the line to the line up to the
                // actual point for this reason we will collect the lengths of
                // the linesegements while iterating
                horline_seg_length.resize(0);
                horline_tot_length.resize(0);
                m_raw_vertices.resize(0);
                // uint32_t vertStartInd = raw_vertices.size();

                for (int py = 0; py < m_patchRes[patchInd].y + 1; py++) {
                    horline_seg_length.emplace_back();
                    horline_tot_length.emplace_back(0.f);

                    // since centriptal catmull-rom generates non-uniform
                    for (int px = 0; px < m_patchRes[patchInd].x + 1; px++) {
                        // interpolate the position
                        // estimate patch relative normalized coordinates
                        interpCoord =
                            vec2((float)px / (float)m_patchRes[patchInd].x, (float)py / (float)m_patchRes[patchInd].y);

                        // get four interpolated points on the four consecutive
                        // vertical lines of this patch
                        for (int i = 0; i < 4; i++) catmullRomCentriGetPoint(m_seg[i], i_points[i], interpCoord.y);

                        // interpolate horizontally between these four points
                        vec2 *cpoints[4] = {&i_points[0], &i_points[1], &i_points[2], &i_points[3]};
                        catmullRomCentriCalcCoeff2(m_hSeg, (vec2 **)&cpoints);
                        catmullRomCentriGetPoint(m_hSeg, hv_iPoint, interpCoord.x);

                        if (px == 0) {
                            horline_seg_length.back().emplace_back(0.f);
                        } else {
                            horline_tot_length.back() += length(hv_iPoint - m_raw_vertices.back());
                            horline_seg_length.back().emplace_back(horline_tot_length.back());
                        }

                        m_raw_vertices.emplace_back(hv_iPoint);

                        if (py < m_patchRes[patchInd].y && px < m_patchRes[patchInd].x) {
                            uint32_t baseInd = vBufIndOffs + (m_patchRes[patchInd].x + 1) * py + px;

                            // two counter clockwise triangles from a quad,
                            // start at lower left corner, upper right corner is
                            // both last point from first triangle and first
                            // points of second triangle
                            m_indices.emplace_back(baseInd);
                            m_indices.emplace_back(baseInd + 1);
                            for (int i = 0; i < 2; i++) m_indices.emplace_back(baseInd + m_patchRes[patchInd].x + 1);
                            m_indices.emplace_back(baseInd + 1);
                            m_indices.emplace_back(baseInd + m_patchRes[patchInd].x + 2);
                        }
                    }
                }

                // now run through all vertical lines again and calculate their
                // distances
                vertline_seg_length.resize(0);
                vertline_tot_length.resize(0);
                for (int px = 0; px < m_patchRes[patchInd].x + 1; px++) {
                    vertline_seg_length.emplace_back();
                    vertline_tot_length.emplace_back(0.f);

                    // since centriptal catmull-rom generates non-uniform
                    for (int py = 0; py < m_patchRes[patchInd].y + 1; py++) {
                        if (py == 0) {
                            vertline_seg_length.back().emplace_back(0.f);
                        } else {
                            uint32_t vertInd0 = (py - 1) * (m_patchRes[patchInd].x + 1) + px;
                            uint32_t vertInd1 = py * (m_patchRes[patchInd].x + 1) + px;
                            vertline_tot_length.back() +=
                                length(vec2(m_raw_vertices[vertInd0]) - vec2(m_raw_vertices[vertInd1]));
                            vertline_seg_length.back().emplace_back(vertline_tot_length.back());
                        }
                    }
                }

                // now run again through all points
                // since we now have for each interpolation step t an arclength
                // s we can get specific curve points in relation of s by
                // interpolating between the vertice position we already
                // calculated
                for (int py = 0; py < m_patchRes[patchInd].y + 1; py++) {
                    for (int px = 0; px < m_patchRes[patchInd].x + 1; px++) {
                        uint32_t bInd = py * (m_patchRes[patchInd].x + 1) + px;
                        relPos =
                            vec2((float)px / (float)m_patchRes[patchInd].x, (float)py / (float)m_patchRes[patchInd].y);

                        if ((px == 0 && py == 0) || (px == m_patchRes[patchInd].x && py == m_patchRes[patchInd].y)) {
                            m_vertices.emplace_back(m_raw_vertices[bInd].x * 2.f - 1.f,
                                                    m_raw_vertices[bInd].y * 2.f - 1.f, 0.f, 1.f);
                        } else {
                            // calculate the relative position inside the patch,
                            // and multiply it with the total length of the
                            // linesegment -> this corresponds to s
                            vec2 relPosS = vec2(relPos.x * horline_tot_length[py], relPos.y * vertline_tot_length[px]);
                            vec2 newCoord;

                            if (px > 0 && px < m_patchRes[patchInd].x) {
                                // get the relative i-1 and i+1 points of the
                                // line segment
                                uint32_t hIntInd[2] = {0, 0};
                                while (relPosS.x >= horline_seg_length[py][hIntInd[1]] &&
                                       hIntInd[1] < (int)horline_seg_length[py].size()) {
                                    hIntInd[1]++;
                                }
                                hIntInd[0] = hIntInd[1] - 1;

                                float hBlendPos =
                                    (relPosS.x - horline_seg_length[py][hIntInd[0]]) /
                                    (horline_seg_length[py][hIntInd[1]] - horline_seg_length[py][hIntInd[0]]);

                                newCoord.x =
                                    mix(m_raw_vertices[py * (m_patchRes[patchInd].x + 1) + hIntInd[0]].x,
                                        m_raw_vertices[py * (m_patchRes[patchInd].x + 1) + hIntInd[1]].x, hBlendPos);
                            } else {
                                newCoord.x = m_raw_vertices[bInd].x;
                            }

                            if (py > 0 && py < m_patchRes[patchInd].y) {
                                // get the relative i-1 and i+1 points of the
                                // line segment
                                uint32_t hIntInd[2] = {0, 0};
                                while (relPosS.y >= vertline_seg_length[px][hIntInd[1]] &&
                                       hIntInd[1] < (int)vertline_seg_length[px].size()) {
                                    hIntInd[1]++;
                                }

                                hIntInd[0] = hIntInd[1] - 1;

                                float vBlendPos =
                                    (relPosS.y - vertline_seg_length[px][hIntInd[0]]) /
                                    (vertline_seg_length[px][hIntInd[1]] - vertline_seg_length[px][hIntInd[0]]);

                                newCoord.y =
                                    mix(m_raw_vertices[hIntInd[0] * (m_patchRes[patchInd].x + 1) + px].y,
                                        m_raw_vertices[hIntInd[1] * (m_patchRes[patchInd].x + 1) + px].y, vBlendPos);
                            } else {
                                newCoord.y = m_raw_vertices[bInd].y;
                            }

                            m_vertices.emplace_back(newCoord.x * 2.f - 1.f, newCoord.y * 2.f - 1.f, 0.f, 1.f);
                        }
                        m_texCoords.emplace_back(relPos * initPatchSize + initPatchOffs);
                    }
                }

                vBufIndOffs += (m_patchRes[patchInd].y + 1) * (m_patchRes[patchInd].x + 1);
            }
        }
    }

    // run again through all patches and collect indices for displaying
    // the grid lines
    // we do it first by columns and then by rows
    // cannot be parallelized
    uint32_t indOffs = 0;

    m_lines_indices.resize(0);
    m_lines_indices_counts.resize(0);

    for (int crd = 0; crd < 2; crd++) {
        for (int i = 0; i < m_gridSize[crd]; i++) {
            m_lines_indices_counts.emplace_back(0);

            for (int j = 0; j < m_gridSize[(crd + 1) % 2] - 1; j++) {
                // if we reached the last index of rows or columns, we need
                // to take the right or upper side of the patch to process
                // we also need to limit the base patch index
                uint32_t i_lim = std::min(i, m_gridSize[crd] - 2);

                if (crd == 0)
                    patchInd = i_lim + j * (m_gridSize[crd] - 1);
                else
                    patchInd = i_lim * (m_gridSize[(crd + 1) % 2] - 1) + j;

                m_lines_indices_counts.back() += m_patchRes[patchInd][(crd + 1) % 2] + 1;

                // if we reached the last patch, use the upper/right side of the
                // patch instead
                if (i == m_gridSize[crd] - 1)
                    indOffs =
                        crd == 0 ? m_patchRes[patchInd][crd] : (m_patchRes[patchInd][0] + 1) * m_patchRes[patchInd][1];
                else
                    indOffs = 0;

                for (int k = 0; k < m_patchRes[patchInd][(crd + 1) % 2] + 1; k++) {
                    if (crd == 0)
                        m_lines_indices.emplace_back(m_patch_ind_offs[patchInd] + k * (m_patchRes[patchInd][crd] + 1) +
                                                     indOffs);
                    else
                        m_lines_indices.emplace_back(m_patch_ind_offs[patchInd] + k + indOffs);
                }
            }
        }
    }

    calcLineGrippers();

    //------------------------------------------------------------------------

    // update opengl buffers
    if (!m_baseMeshVao) m_baseMeshVao = new VAO("position:4f,texCoord:2f", GL_DYNAMIC_DRAW);

    // OpenGL Buffers can't be resized to we have to delete and rebuild them
    if (m_baseMeshVao->getNrVertices() < m_vertices.size()) {
        m_baseMeshVao->resize((GLuint)m_vertices.size());
    }

    m_baseMeshVao->upload(CoordType::Position, &m_vertices[0][0], (uint32_t)m_vertices.size());
    m_baseMeshVao->upload(CoordType::TexCoord, &m_texCoords[0][0], (uint32_t)m_texCoords.size());
    m_baseMeshVao->setElemIndices((GLuint)m_indices.size(), &m_indices[0]);

    if (uvFbo) updateUvFboCpu(uvFbo);

    rebuildGridLineVao(m_baseMeshVao, &m_lines_indices, &m_lines_indices_counts);
}

void CtrlPointGrid::rebuildGridLineVao(VAO *meshVao, vector<uint32_t> *lines_indices,
                                       vector<uint32_t> *lines_indices_counts) {
    if (lines_indices->empty() || lines_indices_counts->empty()) return;

    if (!m_gridLineVbo.empty()) glDeleteBuffers((GLsizei)m_gridLineVbo.size(), &m_gridLineVbo[0]);

    size_t newSize = m_gridSize.x + m_gridSize.y;
    m_gridLineVbo.resize(newSize);
    glGenBuffers((GLsizei)newSize, &m_gridLineVbo[0]);

    uint32_t lineIndOffs = 0;
    for (size_t i = 0; i < newSize; i++) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gridLineVbo[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * lines_indices_counts->at(i),
                     &(lines_indices->at(lineIndOffs)), GL_STATIC_DRAW);
        lineIndOffs += lines_indices_counts->at(i);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_gridLineVaoSize = ivec2(m_gridSize);
}

void CtrlPointGrid::drawGridLines() {
    uint32_t lineIndOffs = 0;

    for (size_t i = 0; i < m_gridLineVbo.size(); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, m_baseMeshVao->getVBO(CoordType::Position));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gridLineVbo[i]);
        glDrawElements(GL_LINE_STRIP, m_lines_indices_counts[i], GL_UNSIGNED_INT, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);

        lineIndOffs += m_lines_indices_counts[i];
    }
}

void CtrlPointGrid::updateUvFboCpu(FBO *uvFbo) {
    // render the pure uv coordinates instead of content to a separate s_fbo,
    // this will be used during vwf rendering for later exporting the warping as
    // a new vwf file
    if (uvFbo && m_baseMeshVao && m_warpDrawOffsUVShdr) {
        // draw it to an s_fbo in order to use it elsewhere again
        uvFbo->bind();
        uvFbo->clear();

        if (!(uvFbo->getType() == GL_RGBA32F)) LOGE << "not RGBA32F!!!";

        m_warpDrawOffsUVShdr->begin();
        // since we are darwing upside down, the y movement of m_l1_postTrans
        // has to be mirrored
        m_l1_post_yMirror = m_l1_postTrans;
        m_l1_post_yMirror[3][1] *= -1.f;
        m_warpDrawOffsUVShdr->setUniformMatrix4fv("m_pvm", &m_l1_post_yMirror[0][0]);
        m_warpDrawOffsUVShdr->setUniformMatrix4fv("perspMat", &m_level1[0][0]);
        m_warpDrawOffsUVShdr->setUniform1i("usePersp", 1);

        m_baseMeshVao->enableVertexAttribs();
        m_baseMeshVao->bindElementBuffer();
        glDrawElements(GL_TRIANGLES, m_baseMeshVao->getNrIndices(), GL_UNSIGNED_INT, nullptr);
        ara::VAO::unbindElementBuffer();
        m_baseMeshVao->disableVertexAttribs();

        uvFbo->unbind();
    }

    if (m_updtCb) {
        m_updtCb();
    }
}

void CtrlPointGrid::updateUvFboGpu(FBO *uvFbo, float *pvm, float *perspMat) {
    if (uvFbo && m_baseMeshVaoGPU) {
        // draw it to an s_fbo in order to use it elsewhere again
        uvFbo->bind();
        uvFbo->clear();

        runTessShader(pvm, 0, (float)uvFbo->getWidth(), (float)uvFbo->getHeight(), perspMat);

        uvFbo->unbind();

        if (m_updtCb) m_updtCb();
    }
}

// add n-1 rows
void CtrlPointGrid::addRow() {
    CtrlPoint *p_from;
    CtrlPoint *p_to;
    vec2       ip_ref_pos;
    vec2       ip_disp_pos;

    uint32_t nrNewPointsPerCol = m_gridSize.y - 1;
    uint32_t baseInd           = 0;
    uint32_t offs              = 0;
    uint32_t yOffs             = 0;

    // go through rows
    for (int y = 0; y < m_gridSize.y - 1; y++) {
        // for each point on the row calculate a sudivinding new point
        // for the upper vertical line segment
        for (int x = 0; x < m_gridSize.x; x++) {
            baseInd = (y + yOffs) * m_gridSize.x + x;
            p_from  = &m_ctrlPoints[1][baseInd];
            p_to    = &m_ctrlPoints[1][baseInd + m_gridSize.x + offs];  // plus one row

            // reference positions always get interpolated linearly
            ip_ref_pos = (p_to->refPosition - p_from->refPosition) * 0.5f + p_from->refPosition;

            if (m_ipolMode == interpolM::Bilinear)
                ip_disp_pos = (p_to->position - p_from->position) * 0.5f + p_from->position;
            else
                ip_disp_pos = (p_to->position - p_from->position) * 0.5f + p_from->position;

            // insert at the corresponding x pos of the next row
            auto it           = m_ctrlPoints[1].insert(m_ctrlPoints[1].begin() + baseInd + m_gridSize.x, CtrlPoint());
            (*it).refPosition = ip_ref_pos;
            (*it).position    = ip_disp_pos;
            (*it).selected    = false;

            offs++;
        }
        offs = 0;
        yOffs++;
    }

    m_gridSize.y += nrNewPointsPerCol;
    resetIds();
}

void CtrlPointGrid::delRow() {
    vector<uint32_t> toKill;
    // TODO optimize by doing ascending in-place-copy of good values
    // vector::erase() is O(n), and would have copied the whole list anyways
    if (m_gridSize.y > 2) {
        // remove the request amount of columns
        // first collect the corresponding iterator, afterward delete them
        for (int y = 1; y < m_gridSize.y; y += 2)
            // remove all points that are part of this column along the y-axis
            for (int x = 0; x < m_gridSize.x; x++) toKill.emplace_back(y * m_gridSize.x + x);

        // sort toKill in reverse order
        sort(toKill.begin(), toKill.end(), greater<>());

        // kill!
        for (const auto &i : toKill) m_ctrlPoints[1].erase(m_ctrlPoints[1].begin() + i);

        m_gridSize.y = std::max(m_gridSize.y - (m_gridSize.y / 2), 2);
        resetIds();
    }
}

// add n-1 rows
void CtrlPointGrid::addCol() {
    CtrlPoint *p_from;
    CtrlPoint *p_to;
    vec2       ip_ref_pos;
    vec2       ip_disp_pos;

    uint32_t nrNewPointsPerRow = m_gridSize.x - 1;
    uint32_t baseInd           = 0;

    // go through rows
    for (int y = 0; y < m_gridSize.y; y++) {
        uint32_t offs = 0;

        // calculate an interpolated new point for each horizontal line segment
        for (int x = 0; x < m_gridSize.x - 1; x++) {
            baseInd = y * (m_gridSize.x + nrNewPointsPerRow) + x + offs;
            p_from  = &m_ctrlPoints[1][baseInd];
            p_to    = &m_ctrlPoints[1][baseInd + 1];

            // reference positions always get interpolated linearly
            ip_ref_pos = (p_to->refPosition - p_from->refPosition) * 0.5f + p_from->refPosition;

            if (m_ipolMode == interpolM::Bilinear) {
                ip_disp_pos = (p_to->position - p_from->position) * 0.5f + p_from->position;
            } else {
                ip_disp_pos = (p_to->position - p_from->position) * 0.5f + p_from->position;
            }
            offs++;

            auto it           = m_ctrlPoints[1].insert(m_ctrlPoints[1].begin() + baseInd + 1, CtrlPoint());
            (*it).refPosition = ip_ref_pos;
            (*it).position    = ip_disp_pos;
            (*it).selected    = false;
        }
    }

    m_gridSize.x += static_cast<int>(nrNewPointsPerRow);
    resetIds();
}

void CtrlPointGrid::delCol() {
    vector<uint32_t> toKill;

    if (m_gridSize.x > 2) {
        // remove the request amount of columns
        // first collect the corresponding iterator, afterwards delete them
        for (int x = 1; x < m_gridSize.x; x += 2)
            // remove all points that are part of this column along the y-axis
            for (int y = 0; y < m_gridSize.y; y++) toKill.emplace_back(y * m_gridSize.x + x);

        // sort toKill in reverse order
        sort(toKill.begin(), toKill.end(), greater<>());

        // kill!
        for (const auto &i : toKill) m_ctrlPoints[1].erase(m_ctrlPoints[1].begin() + i);

        m_gridSize.x = std::max(m_gridSize.x - (m_gridSize.x / 2), 2);
        resetIds();
    }
}

void CtrlPointGrid::resetIds() {
    for (int y = 0; y < m_gridSize.y; y++)
        for (int x = 0; x < m_gridSize.x; x++)
            m_ctrlPoints[1][y * m_gridSize.x + x].setId(y * m_gridSize.x + x + m_objIdOffs);
}

void CtrlPointGrid::freeGLResources() {
    if (m_baseMeshVao) {
        delete m_baseMeshVao;
        m_baseMeshVao = nullptr;
    }

    if (m_baseMeshVaoGPU) {
        delete m_baseMeshVaoGPU;
        m_baseMeshVaoGPU = nullptr;
    }

    delete m_splineSegHor;
}

// helper functions

uint32_t CtrlPointGrid::getStartVInd(uint32_t patchInd) { return m_patch_ind_offs[patchInd]; }

// get the index of the patch at the x,y position of the grid
// we always refer to the patch which left lower GridPoint refers to the x,y
// arguments
uint32_t CtrlPointGrid::getGridPosPatchInd(int x, int y) const {
    return std::min(x, m_gridSize.x - 2) + std::min(y, m_gridSize.y - 2) * (m_gridSize.x - 1);
}

uint32_t CtrlPointGrid::getGridPosVInd(int x, int y) {
    // all patches are oriented to the left lower corner
    // only the grid points on the right and top border of the grid
    // are mapped to right / top Vertices of a patch

    uint32_t vOffs    = 0;
    uint32_t patchInd = getGridPosPatchInd(x, y);

    // if we are on the right border of the grid
    // patches always have patchRes segments, but patchRes +1 vertices per side
    if (x == m_gridSize.x - 1) vOffs += getPatchNrVert(patchInd).x;
    // if we are on the top border of the grid
    if (y == m_gridSize.y - 1) vOffs += (getPatchNrVert(patchInd).y - 1) * getPatchNrVert(patchInd).x;

    return m_patch_ind_offs[patchInd] + vOffs;
}

uint32_t CtrlPointGrid::getPatchPosVInd(uint32_t patchInd, int x, int y) {
    return m_patch_ind_offs[patchInd] + y * getPatchNrVert(patchInd).x + x;
}

// get the Number of Segments in this Patch, or so to say the patch Resolution
ivec2 CtrlPointGrid::getPatchNrSeg(uint32_t patchInd) { return m_patchRes[patchInd]; }

ivec2 CtrlPointGrid::getPatchNrVert(uint32_t patchInd) { return m_patchRes[patchInd] + 1; }

void CtrlPointGrid::getPatchRow(uint32_t patchInd, patchRow pRow, vector<vec4> *vec) {
    if (vec->size() != getPatchNrVert(patchInd).x) vec->resize(getPatchNrVert(patchInd).x);

    auto offs = static_cast<int>(getStartVInd(patchInd));
    if (pRow == VC_P_ROW_TOP) offs += getPatchNrVert(patchInd).x * (getPatchNrVert(patchInd).y - 1);

    auto vIt   = m_vertices.begin() + offs;
    auto dstIt = vec->begin();
    for (; vIt < m_vertices.begin() + offs + vec->size(); ++vIt, ++dstIt) (*dstIt) = vec4(*vIt);
}

// get the tesselated vertices from one column of a patch
void CtrlPointGrid::getPatchCol(uint32_t patchInd, patchCol pCol, vector<vec4> *vec) {
    // resize the destination vector to fit the amount of
    // vertices of one patch column
    if (vec->size() != getPatchNrVert(patchInd).y) vec->resize(getPatchNrVert(patchInd).y);

    // get the first vertex index of the patch
    auto offs = getStartVInd(patchInd);

    // if we are at the right side of a patch
    // apply the offset of one row
    if (pCol == VC_P_ROW_RIGHT) offs += getPatchNrVert(patchInd).x - 1;

    auto vIt = m_vertices.begin() + offs;
    // auto endIt = m_vertices.begin() + offs + getPatchNrVert(patchInd).x *
    // (getPatchNrVert(patchInd).y - 1) + 1;
    auto dstIt = vec->begin();
    for (auto i = 0; i < getPatchNrVert(patchInd).y; i++) {
        (*dstIt) = vec4(*vIt);
        if (i != getPatchNrVert(patchInd).y - 1) {
            vIt += getPatchNrVert(patchInd).x;
            dstIt++;
        }
    }
}

void CtrlPointGrid::initXmlVals() {
    m_xmlMat4.emplace_back(&m_level1, "level1");
    m_xmlMat4.emplace_back(&m_level1_inv, "level1_inv");
    m_xmlMat4.emplace_back(&m_l1_postTrans, "l1_postTrans");
    m_xmlMat4.emplace_back(&m_l1_modRot, "l1_modRot");
    m_xmlMat4.emplace_back(&m_l1_axis_mat, "l1_axis_mat");
    m_xmlMat4.emplace_back(&m_l1_R, "l1_R");

    m_xmlVec3.emplace_back(&l1_rot, "l1_rot");
    m_xmlVec3.emplace_back(&l1_lastRot, "l1_lastRot");
    m_xmlVec3.emplace_back(&l1_rotOffs, "l1_rotOffs");
    m_xmlVec3.emplace_back(&l1_trans, "l1_trans");
    m_xmlVec3.emplace_back(&l1_lastTrans, "l1_lastTrans");
    m_xmlVec3.emplace_back(&l1_transOffs, "l1_transOffs");

    m_xmlVec2.emplace_back(&l1_2D_lastTrans, "l1_2D_lastTrans");
    m_xmlVec2.emplace_back(&l1_2D_transOffs, "l1_2D_transOffs");
    m_xmlVec2.emplace_back(&l1_2D_trans, "l1_2D_trans");

    m_xmlIvec2.emplace_back(&m_gridSize, "gridSize");
}

void CtrlPointGrid::updtXmlVals() {
    auto mat4It       = m_xmlMat4.begin();
    (mat4It++)->first = &m_level1;
    (mat4It++)->first = &m_level1_inv;
    (mat4It++)->first = &m_l1_postTrans;
    (mat4It++)->first = &m_l1_modRot;
    (mat4It++)->first = &m_l1_axis_mat;
    (mat4It++)->first = &m_l1_R;

    auto vec3It       = m_xmlVec3.begin();
    (vec3It++)->first = &l1_rot;
    (vec3It++)->first = &l1_lastRot;
    (vec3It++)->first = &l1_rotOffs;
    (vec3It++)->first = &l1_trans;
    (vec3It++)->first = &l1_lastTrans;
    (vec3It++)->first = &l1_transOffs;

    auto vec2It       = m_xmlVec2.begin();
    (vec2It++)->first = &l1_2D_lastTrans;
    (vec2It++)->first = &l1_2D_transOffs;
    (vec2It++)->first = &l1_2D_trans;

    m_xmlIvec2.begin()->first = &m_gridSize;
}

void CtrlPointGrid::serializeToXml(pugi::xml_node &parent) {
    string compNames[3] = {"x", "y", "z"};

    for (auto &it : m_xmlIvec2) {
        auto node = parent.append_child("ivec2");
        node.append_attribute("propertyName").set_value(it.second.c_str());
        for (int i = 0; i < 2; i++) node.append_attribute(compNames[i].c_str()).set_value((*it.first)[i]);
    }
    for (auto &it : m_xmlVec2) {
        auto node = parent.append_child("vec2");
        node.append_attribute("propertyName").set_value(it.second.c_str());
        for (int i = 0; i < 2; i++) node.append_attribute(compNames[i].c_str()).set_value((*it.first)[i]);
    }
    for (auto &it : m_xmlVec3) {
        auto node = parent.append_child("vec3");
        node.append_attribute("propertyName").set_value(it.second.c_str());
        for (int i = 0; i < 3; i++) node.append_attribute(compNames[i].c_str()).set_value((*it.first)[i]);
    }
    for (auto &it : m_xmlMat4) {
        auto node = parent.append_child("mat4");
        node.append_attribute("propertyName").set_value(it.second.c_str());
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                node.append_attribute(("v" + std::to_string(i * 4 + j)).c_str()).set_value((*it.first)[i][j]);
    }

    auto node = parent.append_child("UInt");
    node.text().set(m_objIdOffs);
    node.append_attribute("propertyName").set_value("objIdOffs");
    node = parent.append_child("Int");
    node.text().set((int)m_ipolMode);
    node.append_attribute("propertyName").set_value("ipolMode");
    node = parent.append_child("Int");
    node.text().set((int)m_epolMode);
    node.append_attribute("propertyName").set_value("epolMode");

    for (size_t i = 0; i < 2; i++) {
        node = parent.append_child("Vector");
        node.append_attribute("size").set_value(m_ctrlPoints[i].size());
        node.append_attribute("propertyName").set_value(("ctrlPoints_" + std::to_string(i)).c_str());
        for (auto &it : m_ctrlPoints[i]) {
            auto sub = node.append_child("CtrlPoint");
            it.serializeToXml(sub);
        }
    }

    node = parent.append_child("Vector");
    node.append_attribute("size").set_value(m_lineGripper.size());
    node.append_attribute("propertyName").set_value("lineGripper");
    for (auto &it : m_lineGripper) {
        auto sub = node.append_child("CtrlPoint");
        it.serializeToXml(sub);
    }
}

void CtrlPointGrid::parseFromXml(pugi::xml_node &node) {
    pugi::xml_attribute attr;

    string compNames[3] = {"x", "y", "z"};
    auto   child        = node.first_child();

    for (auto &it : m_xmlIvec2) {
        for (int i = 0; i < 2; i++)
            if ((attr = child.attribute(compNames[i].c_str()))) (*it.first)[i] = attr.as_int();
        child = child.next_sibling();
    }
    for (auto &it : m_xmlVec2) {
        for (int i = 0; i < 2; i++)
            if ((attr = child.attribute(compNames[i].c_str()))) (*it.first)[i] = attr.as_float();
        child = child.next_sibling();
    }
    for (auto &it : m_xmlVec3) {
        for (int i = 0; i < 3; i++)
            if ((attr = child.attribute(compNames[i].c_str()))) (*it.first)[i] = attr.as_float();
        child = child.next_sibling();
    }
    for (auto &it : m_xmlMat4) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if ((attr = child.attribute(("v" + std::to_string(i * 4 + j)).c_str())))
                    (*it.first)[i][j] = attr.as_float();
        child = child.next_sibling();
    }

    m_objIdOffs = child.text().as_uint();
    child       = child.next_sibling();
    m_ipolMode  = (interpolM)child.text().as_int();
    child       = child.next_sibling();
    m_epolMode  = (extrapolM)child.text().as_int();
    child       = child.next_sibling();

    for (size_t i = 0; i < 2; i++) {
        size_t arSize = (size_t)child.attribute("size").as_uint();
        if (m_ctrlPoints[i].size() != arSize) m_ctrlPoints[i].resize(arSize);
        auto   children = child.children();
        size_t childIdx = 0;
        for (auto &it : children) {
            m_ctrlPoints[i][childIdx].parseFromXml(it);
            childIdx++;
        }
        child = child.next_sibling();
    }

    size_t arSize = (size_t)child.attribute("size").as_uint();
    if (m_lineGripper.size() != arSize) m_lineGripper.resize(arSize);
    auto   children = child.children();
    size_t childIdx = 0;
    for (auto &it : children) {
        m_lineGripper[childIdx].parseFromXml(it);
        childIdx++;
    }
    child = child.next_sibling();
}

CtrlPointGrid::~CtrlPointGrid() = default;

}  // namespace ara
