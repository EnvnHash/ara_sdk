//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
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

#include <GlbCommon/GlbCommon.h>

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
    void   initData(int nrVert, const GLfloat *data = nullptr);
    void   upload(CoordType type, const GLfloat *entries, size_t nrVertices);
    void   setElemIndices(size_t count, const GLuint *indices);
    void   setExtElemIndices(size_t count, GLuint buffer);
    void   resize(size_t newNrVertices);
    void   remove();
    void   bindBuffer(GLuint buffer, GLenum type = GL_ARRAY_BUFFER) const;
    GLuint addBuffer(CoordType type);
    void   addExtBuffer(CoordType type, GLuint buffer);

#ifdef STRICT_WEBGL_1
    void enableAttributes();
#endif

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
    void        drawInstanced(GLenum mode, GLsizei nrInstances, TFO *tfo = nullptr, float nrVert = 1.f);
    void        drawElements(GLenum mode, TFO *tfo, GLenum recMode);
    void        drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements);
    void        drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements, int offset);
    void        drawElementsInst(GLenum mode, GLsizei nrInstances, TFO *tfo, GLenum recMode);
    void       *getMapBuffer(CoordType attrIndex) const;
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
    void disableVertexAttribs() const;

    [[nodiscard]] GLuint          getVAOId() const { return m_VAOId; }
    GLuint                        getVBO(CoordType attrIndex) { return m_buffers[toType(attrIndex)]; }
    std::vector<VertexAttribute> *getAttributes() { return &m_attributes; }
    [[nodiscard]] GLsizei        getNrVertices() const { return m_nrVertices; }
    uint32_t                      getNrBuffers() { return static_cast<uint32_t>(m_buffers.size()); }
    [[nodiscard]] uint32_t        getNrIndices() const { return m_nrElements; }
    [[nodiscard]] GLuint          getElementBuffer() const { return m_elementBuffer; }
    [[nodiscard]] bool            isInited() const { return m_inited; }
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

    GLsizei m_nrElements      = 0;
    GLsizei  m_nrVertices     = 0;
    int     m_drawNrVertices  = 0;
    int     m_nrCoordsPerVert = 3;
    int     m_maxNrInstances  = 0;

    std::vector<VertexAttribute> m_attributes;  //  Array of attributes.
    std::vector<CoordType>      *m_instAttribs = nullptr;
    std::vector<bool>            m_usesStaticCoords;
    std::vector<GLushort>        m_indices;
    std::vector<glm::vec4>       m_statCoords;
};
}  // namespace ara
