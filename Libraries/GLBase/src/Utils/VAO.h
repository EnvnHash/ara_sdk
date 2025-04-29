//
//  VAO.h
//
//  Created by Sven Hahne, last edit 19.08.17
//
//  the data is supposed to be organized e.g. (e.g. vncvncvncvnc for 4 vertices,
//  4 normals, 4 colors) shader locations are supposed to be layout(location =
//  0) in glm::vec4 vertex; layout(location = 1) in glm::vec4 normal;
//  layout(location = 2) in glm::vec4 color;
//  layout(location = 3) in glm::vec4 texCoords;
//  layout(location = 4) in glm::vec4 model_matrix;
//  where the later consumes 4 locations (5-8)
//

#pragma once

#include <glb_common/glb_common.h>

#include "VertexAttribute.h"

namespace ara {

class TFO;
class Mesh;

class VAO {
public:
    // takes a std::string for defining the VAO
    // can be vertex, normal, color, texCoord
    // datatypes are b(GL_BYTE) B(GL_UNSIGNED_BYTE), s(GL_SHORT) ,
    // S(GL_UNSIGNED_SHORT), i(GL_INT), I(GL_UNSIGNED_INT),
    // f(GL_FLOAT), d(GL_DOUBLE)

    VAO();
    VAO(std::string &&format, GLenum storeMode, bool createBuffers = true);
    VAO(std::string &&format, GLenum storeMode, std::vector<CoordType> *instAttribs, int maxNrInstances,
        bool createBuffers = true);
    void init(std::string &&format, bool createBuffers = true, bool interleaved = false);
    virtual ~VAO();
    void   initData(int nrVert, GLfloat *data = nullptr);
    void   upload(CoordType type, GLfloat *entries, size_t nrVertices);
    void   setElemIndices(size_t count, GLuint *indices);
    void   setExtElemIndices(size_t count, GLuint buffer);
    void   resize(GLuint newNrVertices);
    void   remove();
    void   bindBuffer(GLuint buffer, GLenum type = GL_ARRAY_BUFFER) const;
    GLuint addBuffer(CoordType type);
    void   addExtBuffer(CoordType type, GLuint buffer);

#ifdef EMSCRIPTEN
    void bind();
#else
    void bind(TFO *tfo = nullptr, GLenum recMode = GL_TRIANGLES);
#endif
#ifdef EMSCRIPTEN
    void unbind();
#else
    static void unbind(TFO *tfo = nullptr, int incCntr = 0);
#endif
    void        bindElementBuffer() const;
    static void unbindElementBuffer() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
    void        draw(GLenum mode = GL_TRIANGLES);
    void        drawElements(GLenum mode);
#ifndef __EMSCRIPTEN__
    void        draw(GLenum mode, TFO *tfo, GLenum recMode);
    void        draw(GLenum mode, GLint offset, GLsizei count, TFO *tfo);
    void        draw(GLenum mode, GLint offset, GLsizei count, TFO *tfo, GLenum recMode);
    void        drawInstanced(GLenum mode, GLuint nrInstances, TFO *tfo = nullptr, float nrVert = 1.f);
    void        drawElements(GLenum mode, TFO *tfo, GLenum recMode);
    void        drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements);
    void        drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements, int offset);
    void        drawElementsInst(GLenum mode, GLuint nrInstances, TFO *tfo, GLenum recMode);
    void       *getMapBuffer(CoordType attrIndex);
    static void unMapBuffer() {
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void       *mapElementBuffer() const;
    static void unMapElementBuffer() {
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
#else
    GLint addBufferFloat(uint32_t location, uint32_t size, const char *name);
    void  uploadFloat(uint32_t location, uint32_t size, GLfloat *_entries, int _nrVertices);
    void  draw(GLenum _mode, uint32_t offset, uint32_t count);
    void  draw(GLenum _mode, uint32_t offset, uint32_t count, GLenum _recMode);
    void  drawInstanced(GLenum _mode, int nrInstances, float nrVert = 1.f);
    void  drawElements(GLenum _mode, GLenum _recMode);
    void  drawElements(GLenum _mode, GLenum _recMode, int _nrElements);

#endif
    void uploadMesh(Mesh *mesh);
    void setStaticNormal(float x, float y, float z) { setStatic({x, y, z, 0}, "normal", "normal:3f"); }
    void setStaticNormal(std::vector<GLfloat> *norm) {
        setStatic({norm->at(0), norm->at(1), norm->at(2), 0}, "normal", "normal:3f");
    }
    void setStaticColor(glm::vec4 color) {
        setStatic(color, (char *)"color", (char *)"color:4f");
    }
    void setStaticColor(std::vector<GLfloat> *col) {
        setStatic({col->at(0), col->at(1), col->at(2), col->at(3)}, (char *)"color", (char *)"color:4f");
    }
    void setStatic(glm::vec4 color, const std::string& name, const std::string& format);
    void enableVertexAttribs();
    void disableVertexAttribs();

    [[nodiscard]] GLuint          getVAOId() const { return m_VAOId; }
    GLuint                        getVBO(CoordType attrIndex) { return m_buffers[toType(attrIndex)]; }
    std::vector<VertexAttribute> *getAttributes() { return &m_attributes; }
    [[nodiscard]] uint32_t        getNrVertices() const { return m_nrVertices; }
    uint32_t                      getNrBuffers() { return (uint32_t)m_buffers.size(); }
    [[nodiscard]] uint32_t        getNrIndices() const { return m_nrElements; }
    GLuint                        getElementBuffer() const { return m_elementBuffer; }
    bool                          isInited() const { return m_inited; }
    void                          setStoreMode(GLenum mode) { m_storeMode = mode; }

private:
    std::vector<GLuint> m_buffers;
    GLuint              m_elementBuffer     = 0;
    GLuint              m_VAOId             = 0;
    GLenum              m_storeMode         = GL_STATIC_DRAW;
    GLenum              m_elementBufferType = GL_UNSIGNED_INT;

    bool m_createBuffers = true;
    bool m_inited        = false;
    bool m_interleaved   = false;

    GLuint m_nrElements      = 0;
    GLuint m_nrVertices      = 0;
    int    m_drawNrVertices  = 0;
    int    m_nrCoordsPerVert = 3;
    int    m_maxNrInstances  = 0;

    std::vector<VertexAttribute> m_attributes;  //  Array of attributes.
    std::vector<CoordType>      *m_instAttribs = nullptr;
    std::vector<bool>            m_usesStaticCoords;
    std::vector<GLushort>        m_indices;
    std::vector<glm::vec4>       m_statCoords;
};
}  // namespace ara
