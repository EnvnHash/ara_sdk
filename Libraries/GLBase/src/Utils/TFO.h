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
    TFO(int _bufSize, std::vector<std::string> &_parNames);

    TFO();

    ~TFO();

    void init();
    void bind(bool _resetCounters = true);
    void begin(GLenum _mode);
    void offsetBuf();
    void pauseAndOffsetBuf(GLenum _mode);
    void incCounters(int nrVertices);
    void decCounters(int nrVertices);
    void draw(GLenum _mode, TFO *_tfo, GLenum _recToMode, int geoAmpAmt) const;
    void draw(GLenum _mode, GLuint offset, GLuint count, TFO *_tfo, GLenum _recToMode, int geoAmpAmt) const;
    void setIndices(GLuint _count, GLenum _type, GLvoid *_indices);
    void drawElements(GLenum _mode, GLuint _count, GLenum _type, TFO *_tfo, GLenum _recToMode, int geoAmpAmt);

    void drawElementsBaseVertex(GLenum _mode, GLuint _count, GLenum _type, GLint basevertex, TFO *_tfo,
                                GLenum _recToMode, int geoAmpAmt);

    static void unbind();
    static void setVaryingsToRecord(std::vector<std::string> *names, GLuint _prog);
    static void pause();
    static void resume();
    static void end();

    void                                         setVaryingsToRecordInternNames(GLuint _prog);
    void                                         resizeTFOBuf(CoordType _nr, uint32_t size);
    GLuint                                       getTFOBuf(CoordType _nr);
    uint32_t                                     getTFOBufFragSize(CoordType _nr);
    GLuint                                       getTFOBufSize(CoordType _nr);
    [[nodiscard]] GLuint                         getId() const;
    GLuint                                       getTbo(CoordType _coord);
    [[nodiscard]] GLuint                         getRecVertOffs() const;
    [[nodiscard]] GLuint                         getTotalPrimWritten() const;
    [[nodiscard]] GLuint                         getBufSize() const;
    [[nodiscard]] GLuint                         getVao() const;
    glm::vec4                                    getColor(int ind);
    [[nodiscard]] GLenum                         getLastMode() const;
    std::vector<std::pair<uint32_t, uint32_t> > *getObjOffsets();

    void setBlendMode(GLenum _srcMode, GLenum _dstMode);
    void setSceneNodeColors(glm::vec4 *_cols);
    void setSceneProtoName(std::string *_protoName);
    void setVaryingsNames(std::vector<std::string> &_names);
    void setRecNrVert(int _nr);
    void setRecVertOffs(int _nr);
    void setObjOffset();
    void resetObjOffset();
    void incRecVertOffs(int _nr);
    void addTexture(int _unit, int _texInd, GLenum _target, const std::string &_name);
    void enableDepthTest();
    void disableDepthTest();
    void recallDepthTestState() const;
    void recallBlendMode() const;

    GLuint                 primitivesWritten{};
    GLuint                 totalPrimsWritten{};
    GLuint                 bufSize;
    std::vector<auxTexPar> textures;

private:
    GLuint                   tfo{};
    GLuint                  *buffers{};
    GLuint                  *bufferSizes{};
    GLuint                  *tbos{};
    GLuint                   resVAO{};
    GLuint                   resElementBuffer{};
    GLuint                   resElementBufferNrIndices{};
    GLuint                   resElementBufferSize{};
    GLfloat                 *statColor{};
    int                      recNrVert{};
    int                      recVertOffs{};
    uint32_t                 nrPar{};
    std::vector<std::string> parNames;
    std::vector<std::string> varyingsNames;
    std::vector<uint32_t>    fragSizes;
    std::vector<uint32_t>    locations;

    std::vector<std::pair<uint32_t, uint32_t> > objOffset;

    bool         depthTest{};
    GLenum       srcMode{};
    GLenum       dstMode{};
    glm::vec4   *recColors{};
    std::string *recProtoName{};
    GLenum       lastMode{};
};
}  // namespace ara