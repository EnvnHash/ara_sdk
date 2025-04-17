/*
 * Warping.h
 *
 *  Created on: Jun 20, 2019
 *      Author: sven
 */

#pragma once

#include <hps/hps.h>

#include "GeoPrimitives/CtrlPoint.h"
#include "Shaders/ShaderUtils/ShaderBuffer.h"
#include "StopWatch.h"

namespace ara {

class FBO;
class VAO;
class GLRenderer;
class Shaders;

class CtrlPointGrid {
public:
    enum patchRow { VC_P_ROW_BOTTOM = 0, VC_P_ROW_TOP = 1 };
    enum patchCol { VC_P_ROW_LEFT = 0, VC_P_ROW_RIGHT = 1 };
    enum class tessMode : int { CPU = 0, GPU = 1 };

    explicit CtrlPointGrid(ShaderCollector &shCol);
    ~CtrlPointGrid();

    void initWarpOffsUVDrawShdr();
    void initExtraPolShdr();
    void initSplineCoeffShdr();
    void initXmlVals();
    void updtXmlVals();
    void createBaseGridVao();
    void calcLineGrippers();
    void extrapolBorder();

    void tesselate(tessMode tMode = tessMode::CPU, float *pvm = nullptr, float *perspMat = nullptr,
                   FBO *uvFbo = nullptr);

    void tesselateGPU(FBO *uvFbo = nullptr, float *pvm = nullptr, float *perspMat = nullptr);
    void tesselateCPU(FBO *uvFbo = nullptr);
    void runTessShader(float *pvm, int texId, float fboWidth, float fboHeight, float *perspMat = nullptr);

    void rebuildGridLineVao(VAO *meshVao, std::vector<uint32_t> *lines_indices,
                            std::vector<uint32_t> *lines_indices_counts);

    void updateUvFboCpu(FBO *uvFbo = nullptr);
    void updateUvFboGpu(FBO *uvFbo = nullptr, float *pvm = nullptr, float *perspMat = nullptr);
    void drawGridLines();
    void addRow();
    void delRow();
    void addCol();
    void delCol();
    void resetIds();
    void freeGLResources();

    // helper functions
    uint32_t getStartVInd(uint32_t patchInd);

    [[nodiscard]] uint32_t getGridPosPatchInd(int x, int y) const;

    uint32_t getGridPosVInd(int x, int y);
    uint32_t getPatchPosVInd(uint32_t patchInd, int x, int y);

    glm::ivec2 getPatchNrSeg(uint32_t patchInd);

    glm::ivec2 getPatchNrVert(uint32_t patchInd);

    void getPatchRow(uint32_t patchInd, patchRow pRow, std::vector<glm::vec4> *vec);
    void getPatchCol(uint32_t patchInd, patchCol pCol, std::vector<glm::vec4> *vec);

    VAO                    *getBaseMeshVao() { return m_baseMeshVao; }
    glm::ivec2             *getGridSize() { return &m_gridSize; }
    std::vector<uint32_t>  *getLinesIncides() { return &m_lines_indices; }
    std::vector<uint32_t>  *getLinesIncidesCount() { return &m_lines_indices_counts; }
    std::vector<CtrlPoint> *getCtrPoints(size_t level) { return &m_ctrlPoints[level]; }
    std::vector<GLuint>    *getGridLineVbo() { return &m_gridLineVbo; }
    GLRenderer             *getRenderer() { return m_renderer; }

    std::vector<CtrlPoint> *getCtrlPoints(size_t idx) {
        return m_ctrlPoints.size() > idx ? &m_ctrlPoints[idx] : nullptr;
    }

    CtrlPoint *getCtrlPoints(size_t lvl, size_t idx) {
        return (m_ctrlPoints.size() > lvl && m_ctrlPoints[lvl].size() > idx) ? &m_ctrlPoints[lvl][idx] : nullptr;
    }

    std::vector<CtrlPoint> *getLineGripper() { return &m_lineGripper; }
    glm::vec3              *getL1RotOffs() { return &l1_rotOffs; }
    glm::vec3              *getL1TransOffs() { return &l1_transOffs; }
    glm::vec3              *getL1Trans() { return &l1_trans; }
    glm::vec3              *getL1Rot() { return &l1_rot; }
    glm::vec3              *getL1LastRot() { return &l1_lastRot; }
    glm::mat4              *getL1PostTrans() { return &m_l1_postTrans; }
    glm::mat4              *getL1AxisMat() { return &m_l1_axis_mat; }
    float                  *getL1AxisMatPtr() { return &m_l1_axis_mat[0][0]; }
    float                  *getLevel1MatPtr() { return &m_level1[0][0]; }
    glm::mat4              *getLevel1Mat() { return &m_level1; }
    glm::mat4              *getL1RotMat() { return &m_l1_R; }
    glm::mat4              *getL1MatInv() { return &m_level1_inv; }
    glm::vec2              *getL12DTrans() { return &l1_2D_trans; }
    glm::vec2              *getL12DTransOffs() { return &l1_2D_transOffs; }
    glm::vec2              *getL12DLastTrans() { return &l1_2D_lastTrans; }

    // void setUvFbo(FBO* s_fbo)                              { m_uvFbo = s_fbo;
    // tesselate(); }
    void setL1LastRot(glm::vec3 vec) { l1_lastRot = vec; }
    void setL1RotMat(glm::mat4 mat) { m_l1_R = mat; }
    void setL1RotVec(glm::vec3 vec) { l1_rot = vec; }
    void setL1AxisMat(glm::mat4 mat) { m_l1_axis_mat = mat; }
    void setL1MatInv(glm::mat4 mat) { m_level1_inv = mat; }
    void setL1Trans(int idx, float val) { l1_trans[idx] = val; }
    void setL1LastTrans(glm::vec3 vec) { l1_lastTrans = vec; }
    void setL1PostTrans(glm::mat4 mat) { m_l1_postTrans = mat; }
    void setL12DTrans(glm::vec2 vec) { l1_2D_trans = vec; }
    void setL12DLastTrans(glm::vec2 vec) { l1_2D_lastTrans = vec; }
    void setL12DTransOffs(glm::vec2 vec) { l1_2D_transOffs = vec; }

    void serializeToXml(pugi::xml_node &parent);
    void parseFromXml(pugi::xml_node &node);

    template <class B>
    void serialize(B &buf) const {
        for (auto &it : m_xmlMat4)
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++) buf << (*it.first)[i][j];
        for (auto &it : m_xmlVec3)
            for (int j = 0; j < 3; j++) buf << (*it.first)[j];
        for (auto &it : m_xmlVec2)
            for (int j = 0; j < 2; j++) buf << (*it.first)[j];
        for (auto &it : m_xmlIvec2)
            for (int j = 0; j < 2; j++) buf << (*it.first)[j];

        buf << m_ctrlPointsExtra.size();
        for (auto &it : m_ctrlPointsExtra) buf << it[0] << it[1];

        buf << m_objIdOffs << m_ctrlPoints << m_lineGripper << (int)m_ipolMode << (int)m_epolMode;
    }

    template <class B>
    void parse(B &buf) {
        for (auto &it : m_xmlMat4)
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++) buf >> (*it.first)[i][j];
        for (auto &it : m_xmlVec3)
            for (int j = 0; j < 3; j++) buf >> (*it.first)[j];
        for (auto &it : m_xmlVec2)
            for (int j = 0; j < 2; j++) buf >> (*it.first)[j];
        for (auto &it : m_xmlIvec2)
            for (int j = 0; j < 2; j++) buf >> (*it.first)[j];

        size_t ctrlPointsExtraSize;
        buf >> ctrlPointsExtraSize;
        m_ctrlPointsExtra.resize(ctrlPointsExtraSize);
        for (auto &it : m_ctrlPointsExtra) buf >> it[0] >> it[1];

        buf >> m_objIdOffs >> m_ctrlPoints >> m_lineGripper;

        int temp;
        buf >> temp;
        m_ipolMode = (interpolM)temp;
        buf >> temp;
        m_epolMode = (extrapolM)temp;
    }

    glm::ivec2 *getSize() { return &m_gridSize; }
    interpolM   getIpMode() { return m_ipolMode; }
    void        setIpMode(interpolM _mode) { m_ipolMode = _mode; }
    void        setUpdtCb(std::function<void()> func) { m_updtCb = std::move(func); }
    tessMode    getTessMode() { return m_tessMode; }

private:
    glm::ivec2 m_gridSize{};
    glm::ivec2 m_patchResMax{};
    glm::mat4  m_level1{};
    glm::mat4  m_level1_inv{};
    glm::mat4  m_l1_axis_mat{};
    glm::mat4  m_l1_postTrans{};
    glm::mat4  m_l1_modRot{};
    glm::mat4  m_l1_post_yMirror{};
    glm::mat4  m_l1_R{};
    glm::vec2  l1_2D_trans{};
    glm::vec2  l1_2D_lastTrans{};
    glm::vec2  l1_2D_transOffs{};
    glm::vec2  l2_2D_transOffs{};
    glm::vec3  l1_rot{};
    glm::vec3  l1_lastRot{};
    glm::vec3  l1_rotOffs{};
    glm::vec3  l1_trans{};
    glm::vec3  l1_lastTrans{};
    glm::vec3  l1_transOffs{};

    std::vector<std::vector<CtrlPoint>> m_ctrlPoints;
    std::vector<glm::vec2>              m_ctrlPointsExtra;
    std::vector<CtrlPoint>              m_lineGripper;

    std::vector<glm::vec2> m_raw_vertices;
    std::vector<glm::vec4> m_vertices;
    std::vector<glm::vec4> m_baseVertices;  // only tessMode::GPU
    std::vector<glm::vec2> m_texCoords;
    std::vector<glm::vec2> m_baseTexCoords;  // only tessMode::GPU

    std::vector<glm::ivec2> m_patchRes;

    std::vector<uint32_t> m_indices;
    std::vector<uint32_t> m_baseIndices;  // only tessMode::GPU
    std::vector<uint32_t> m_patch_ind_offs;
    std::vector<uint32_t> m_lines_indices;
    std::vector<uint32_t> m_lines_indices_counts;

    ShaderBuffer<SplineSegment> *m_splineSegHor = nullptr;

private:
    interpolM m_ipolMode;
    extrapolM m_epolMode;
    tessMode  m_tessMode;

    float m_work_group_size = 128.f;

    uint32_t m_width          = 0;
    uint32_t m_height         = 0;
    uint32_t m_nrMeshIndices  = 0;
    uint32_t m_nrMeshVertices = 0;
    uint32_t m_tessBaseRes;
    uint32_t m_objIdOffs = 0;

    VAO                *m_baseMeshVao    = nullptr;
    VAO                *m_baseMeshVaoGPU = nullptr;
    std::vector<GLuint> m_gridLineVbo;
    // FBO* m_uvFbo=nullptr;
    Shaders         *m_warpDrawOffsUVShdr = nullptr;
    Shaders         *m_tessShader         = nullptr;
    Shaders         *m_splineCoeffShdr    = nullptr;
    Shaders         *m_stdCol             = nullptr;
    ShaderCollector &m_shCol;
    GLRenderer      *m_renderer = nullptr;

    // variables for tesselation
    SplineSegment m_seg[4];
    SplineSegment m_hSeg;

    glm::ivec2 m_gridPosShift;
    glm::ivec2 m_gridLineVaoSize;

    std::vector<std::pair<glm::mat4 *, std::string>>  m_xmlMat4;
    std::vector<std::pair<glm::vec3 *, std::string>>  m_xmlVec3;
    std::vector<std::pair<glm::vec2 *, std::string>>  m_xmlVec2;
    std::vector<std::pair<glm::ivec2 *, std::string>> m_xmlIvec2;

    std::function<void()> m_updtCb;

    std::string m_catmRomCoeffFunc;

    StopWatch m_tessWatch;
    StopWatch m_updtFbo;
};
}  // namespace ara
