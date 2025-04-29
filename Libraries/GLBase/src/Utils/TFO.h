/*
 *  TFO.h
 *
 *  Created by Sven Hahne on 25.05.11.
 *  Copyright 2011 the PEBRE community. All rights reserved..
 *
 *  Transformation Feedback Object Wrapper
 */

#pragma once

#include "glb_common/glb_common.h"

namespace ara {
class TFO {
public:
    TFO();
    TFO(int bufSize, const std::vector<std::string> &parNames);
    ~TFO();

    void init();
    void bind(bool _resetCounters = true);
    void begin(GLenum _mode);
    void offsetBuf() const;
    void pauseAndOffsetBuf(GLenum _mode);
    void incCounters(int nrVertices);
    void decCounters(int nrVertices);
    void draw(GLenum mode, TFO *tfo, GLenum recToMode, int geoAmpAmt) const;
    void draw(GLenum mode, GLuint offset, GLuint count, TFO *tfo, GLenum recToMode, int geoAmpAmt) const;
    void setIndices(GLuint count, GLenum type, const GLvoid *indices);
    void drawElements(GLenum mode, GLuint count, GLenum type, TFO *tfo, GLenum recToMode, int geoAmpAmt) const;

    void drawElementsBaseVertex(GLenum mode, GLuint count, GLenum type, GLint basevertex, TFO *tfo,
                                GLenum recToMode, int geoAmpAmt) const;

    static void unbind();
    static void setVaryingsToRecord(const std::vector<std::string> *names, GLuint prog);
    static void pause() { glPauseTransformFeedback(); }
    static void resume() { glResumeTransformFeedback(); }
    static void end() { glEndTransformFeedback(); }

    void                                         setVaryingsToRecordInternNames(GLuint _prog) const;
    void                                         resizeTFOBuf(CoordType nr, uint32_t size);
    GLuint                                       getTFOBuf(CoordType nr);
    uint32_t                                     getTFOBufFragSize(CoordType nr);
    GLuint                                       getTFOBufSize(CoordType nr);
    [[nodiscard]] GLuint                         getId() const { return m_tfo; }
    [[nodiscard]] GLuint                         getTbo(CoordType coord) const { return m_tbos[toType(coord)]; }
    [[nodiscard]] GLuint                         getRecVertOffs() const { return m_recVertOffs; }
    [[nodiscard]] GLuint                         getTotalPrimWritten() const { return m_totalPrimsWritten; }
    [[nodiscard]] GLuint                         getBufSize() const { return m_bufSize; }
    [[nodiscard]] GLuint                         getVao() const { return m_resVAO; }
    [[nodiscard]] glm::vec4                      getColor(int ind) const { return m_recColors[ind]; }
    [[nodiscard]] GLenum                         getLastMode() const { return m_lastMode; }
    std::vector<std::pair<uint32_t, uint32_t> > *getObjOffsets() { return &m_objOffset; }

    void setBlendMode(GLenum srcMode, GLenum dstMode);
    void setSceneNodeColors(const glm::vec4 *cols);
    void setVaryingsNames(std::vector<std::string> &names);
    void setRecVertOffs(int nr) { m_recVertOffs = nr; }
    void setObjOffset();
    void resetObjOffset() { m_objOffset.clear(); }
    void incRecVertOffs(int nr) { m_recVertOffs += nr; }
    void addTexture(int unit, int texInd, GLenum target, const std::string &_name);
    void enableDepthTest() { m_depthTest = true; }
    void disableDepthTest() { m_depthTest = false;}
    void recallDepthTestState() const;
    void recallBlendMode() const { glBlendFunc(m_srcMode, m_dstMode); }

    GLuint                 m_primitivesWritten{};
    GLuint                 m_totalPrimsWritten{};
    GLuint                 m_bufSize;
    std::vector<auxTexPar> m_textures;

private:
    GLuint                   m_tfo{};
    GLuint                   m_resVAO{};
    GLuint                   m_resElementBuffer{};
    GLuint                   m_resElementBufferSize{};
    std::vector<GLuint>      m_buffers;
    std::vector<GLuint>      m_bufferSizes;
    std::vector<GLuint>      m_tbos;
    glm::vec4                m_statColor{};
    std::vector<glm::vec4>   m_recColors;

    int                      m_recVertOffs{};
    uint32_t                 m_nrPar{};
    std::vector<std::string> m_parNames;
    std::vector<std::string> m_varyingsNames;
    std::vector<uint32_t>    m_fragSizes;
    std::vector<uint32_t>    m_locations;

    std::vector<std::pair<uint32_t, uint32_t> > m_objOffset;

    bool         m_depthTest{};
    GLenum       m_srcMode{};
    GLenum       m_dstMode{};
    GLenum       m_lastMode{};
};
}  // namespace ara