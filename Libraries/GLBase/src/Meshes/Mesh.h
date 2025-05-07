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

#pragma once

#include "GlbCommon/GlbCommon.h"

namespace ara {

class Mesh {
public:
    Mesh();
    explicit Mesh(const char *format);
    virtual ~Mesh() = default;

    void init(const char *format);
    void scale(glm::vec3 scale);
    void rotate(float angle, float rotX, float rotY, float rotZ);
    void translate(glm::vec3 trans);
    void doTransform(bool transfNormals);
    void invertNormals();
    void calcNormals();
    void calcSmoothNormals();
    void genTexCoord(TexCordGenType type);
    void push_back(GLfloat *coords, int count);
    void push_back_any(CoordType t, GLfloat *vertices, int count);

    void push_back_indices(GLushort *indices, int count) {
        for (int i = 0; i < count; i++) {
            m_indices.emplace_back(indices[i]);
        }
        m_useIndices = true;
    }

    void push_back_indices(GLuint *indices, int count) {
        for (int i = 0; i < count; i++) {
            m_indicesUint.emplace_back(indices[i]);
        }
        m_useIndicesUint = true;
    }

    void push_back_positions(GLfloat *positions, int count) { push_back_any(CoordType::Position, positions, count); }
    void push_back_normals(GLfloat *normals, int count) { push_back_any(CoordType::Normal, normals, count); }
    void push_back_texCoords(GLfloat *texCoords, int count) { push_back_any(CoordType::TexCoord, texCoords, count); }
    void push_back_colors(GLfloat *colors, int count) { push_back_any(CoordType::Color, colors, count); }

    void clear_indices() { m_indices.clear(); m_indicesUint.clear(); }

    void                  clear_positions() { m_positions.clear(); }
    void                  clear_normals() { m_normals.clear(); }
    void                  clear_texCoords() { m_texCoords.clear(); }
    void                  clear_colors() { m_colors.clear(); }
    std::vector<GLfloat> *getPositions() { return &m_positions; }
    std::vector<GLfloat> *getNormals() { return &m_normals; }
    std::vector<GLfloat> *getTexCoords() { return &m_texCoords; }
    std::vector<GLfloat> *getColors() { return &m_texCoords; }

    int getTotalByteSize();

    void *getPtrInterleaved();

    [[nodiscard]] int getNrIndices() const {
        if (m_useIndices) {
            return static_cast<int>(m_indices.size());
        } else if (m_useIndicesUint) {
            return static_cast<int>(m_indicesUint.size());
        }
        return 0;
    }

    [[nodiscard]] int getByteSizeInd() const {
        return m_useIndices ? static_cast<int>(m_indices.size() * sizeof(GLushort))
                            : m_useIndicesUint ? static_cast<int>(m_indicesUint.size() * sizeof(GLuint))
                                               : 0;
    }

    [[nodiscard]] int getNrPositions() const { return static_cast<int>(m_positions.size()) / m_coordTypeSize[toType(CoordType::Position)]; }
    [[nodiscard]] int getNrVertIntrl() const { return static_cast<int>(m_interleaved.size()); }
    [[nodiscard]] int getByteSizeVert() const { return static_cast<int>(m_positions.size()) * sizeof(GLfloat); }
    static GLenum getType(CoordType t) { return GL_FLOAT; }
    [[nodiscard]] int getByteSize(int t) const { return static_cast<int>(m_allCoords[t]->size() * sizeof(GL_FLOAT)); }
    [[nodiscard]] int getByteSize(CoordType t) const { return static_cast<int>(m_allCoords[toType(t)]->size() * sizeof(GL_FLOAT)); }
    [[nodiscard]] int getNrAttributes() const { return static_cast<int>(m_usedCoordTypes.size()); }
    [[nodiscard]] int getSize(CoordType t) const { return static_cast<int>(m_allCoords[toType(t)]->size()); }

    [[nodiscard]] void *getPtr(int t) const { return !m_allCoords[t]->empty() ? &m_allCoords[t]->front() : nullptr; }
    [[nodiscard]] void *getPtr(CoordType t) const { return !m_allCoords[toType(t)]->empty() ? &m_allCoords[toType(t)]->front() : nullptr; }

    GLfloat  *getPositionPtr() { return &m_positions.front(); }
    GLfloat  *getNormalPtr() { return &m_normals.front(); }
    GLfloat  *getTexCoordPtr() { return &m_texCoords.front(); }
    GLushort *getIndicePtr() { return &m_indices.front(); }
    GLuint   *getIndiceUintPtr() { return &m_indicesUint.front(); }

    [[nodiscard]] GLuint getIndexUint(int ind) const {
        GLuint out = 0;
        if (static_cast<int>(m_indices.size()) > ind) {
            out = m_indices[ind];
        }
        return out;
    }

    [[nodiscard]] GLushort getIndex(int ind) const {
        if (static_cast<int>(m_indices.size()) > ind) {
            return m_indices[ind];
        }
        return 0;
    }

    glm::vec3 getVec3(CoordType t, int ind);

    void resize(CoordType t, uint32_t size);

    std::vector<GLfloat> *getStaticColor() { return &m_statColor; }
    std::vector<GLfloat> *getStaticNormal() { return &m_statNormal; }

    void setStaticColor(float r, float g, float b, float a);
    void setStaticNormal(glm::vec3 norm);
    [[nodiscard]] bool usesStaticColor() const { return !m_statColor.empty(); }
    [[nodiscard]] bool usesStaticNormal() const { return !m_statNormal.empty(); }
    void dumpInterleaved();

    [[nodiscard]] bool usesIntrl() const { return m_useIntrl; }
    [[nodiscard]] bool usesIndices() const { return m_useIndices || m_useIndicesUint; }
    [[nodiscard]] bool usesUintIndices() const { return m_useIndicesUint; }
    [[nodiscard]] bool               hasTexCoords() const { return static_cast<int>(!m_texCoords.empty()); }
    void               setVec3(CoordType t, uint32_t ind, glm::vec3 _vec);
    void               setName(std::string name) { m_name = std::move(name); }
    void               setMaterialId(int id) { m_materialId = id; }
    [[nodiscard]] const char        *getFormat() const { return m_format.c_str(); }

protected:
    std::string              m_name;
    std::vector<std::string> m_entr_types;  // Format of the vertex buffer.
    std::string              m_format;

    std::vector<bool>      m_bUsing;
    bool                   m_useIntrl       = false;
    bool                   m_useIndices     = false;
    bool                   m_useIndicesUint = false;
    std::vector<CoordType> m_usedCoordTypes;
    std::vector<int>       m_coordTypeSize;

    std::vector<GLfloat> m_statColor;
    std::vector<GLfloat> m_statNormal;

    std::vector<GLushort> m_indices;
    std::vector<GLuint>   m_indicesUint;

    std::vector<GLfloat>                m_positions;
    std::vector<GLfloat>                m_normals;
    std::vector<GLfloat>                m_texCoords;
    std::vector<GLfloat>                m_colors;
    std::vector<std::vector<GLfloat> *> m_allCoords;

    std::vector<GLfloat> m_interleaved;

    GLfloat m_r = 1.f;
    GLfloat m_g = 1.f;
    GLfloat m_b = 1.f;
    GLfloat m_a = 1.f;

    glm::mat4 m_transfMatr     = glm::mat4(1.f);
    glm::mat4 m_normTransfMatr = glm::mat4(1.f);

    int m_materialId = 0;
};

}  // namespace ara
