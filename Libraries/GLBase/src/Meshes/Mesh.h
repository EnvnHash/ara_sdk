#pragma once

#include "glb_common/glb_common.h"

namespace ara {

class Mesh {
public:
    Mesh();
    explicit Mesh(const char *format);
    virtual ~Mesh() = default;

    void init(const char *format);
    void scale(float scaleX, float scaleY, float scaleZ);
    void rotate(float angle, float rotX, float rotY, float rotZ);
    void translate(float x, float y, float z);
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

    int getNrIndices() {
        if (m_useIndices) {
            return static_cast<int>(m_indices.size());
        } else if (m_useIndicesUint) {
            return static_cast<int>(m_indicesUint.size());
        }
        return 0;
    }

    int getByteSizeInd() {
        return m_useIndices ? static_cast<int>(m_indices.size() * sizeof(GLushort))
                            : m_useIndicesUint ? static_cast<int>(m_indicesUint.size() * sizeof(GLuint))
                                               : 0;
    }

    int getNrPositions() { return static_cast<int>(m_positions.size()) / m_coordTypeSize[toType(CoordType::Position)]; }
    int getNrVertIntrl() { return static_cast<int>(m_interleaved.size()); }
    int getByteSizeVert() { return static_cast<int>(m_positions.size()) * sizeof(GLfloat); }
    GLenum getType(CoordType t) { return GL_FLOAT; }
    int    getByteSize(int t) { return static_cast<int>(m_allCoords[t]->size() * sizeof(GL_FLOAT)); }
    int    getByteSize(CoordType t) { return static_cast<int>(m_allCoords[toType(t)]->size() * sizeof(GL_FLOAT)); }
    int    getNrAttributes() { return static_cast<int>(m_usedCoordTypes.size()); }
    int    getSize(CoordType t) { return static_cast<int>(m_allCoords[toType(t)]->size()); }

    void *getPtr(int t) { return !m_allCoords[t]->empty() ? &m_allCoords[t]->front() : nullptr; }
    void *getPtr(CoordType t) { return !m_allCoords[toType(t)]->empty() ? &m_allCoords[toType(t)]->front() : nullptr; }

    GLfloat  *getPositionPtr() { return &m_positions.front(); }
    GLfloat  *getNormalPtr() { return &m_normals.front(); }
    GLfloat  *getTexCoordPtr() { return &m_texCoords.front(); }
    GLushort *getIndicePtr() { return &m_indices.front(); }
    GLuint   *getIndiceUintPtr() { return &m_indicesUint.front(); }

    GLuint getIndexUint(int ind) {
        GLuint out = 0;
        if (int(m_indices.size()) > ind) {
            out = m_indices[ind];
        }
        return out;
    }

    GLushort getIndex(int ind) {
        if (int(m_indices.size()) > ind) {
            return m_indices[ind];
        }
        return 0;
    }

    glm::vec3 getVec3(CoordType t, int ind);

    void resize(CoordType t, uint32_t size);

    std::vector<GLfloat> *getStaticColor() { return &m_statColor; }
    std::vector<GLfloat> *getStaticNormal() { return &m_statNormal; }

    void setStaticColor(float r, float g, float b, float a);
    void setStaticNormal(float x, float y, float z);
    bool usesStaticColor() { return !m_statColor.empty(); }
    bool usesStaticNormal() { return !m_statNormal.empty(); }
    void dumpInterleaved();

    [[nodiscard]] bool usesIntrl() const { return m_useIntrl; }
    [[nodiscard]] bool usesIndices() const { return m_useIndices || m_useIndicesUint; }
    [[nodiscard]] bool usesUintIndices() const { return m_useIndicesUint; }
    bool               hasTexCoords() { return static_cast<int>(!m_texCoords.empty()); }
    void               setVec3(CoordType t, uint32_t ind, glm::vec3 _vec);
    void               setName(std::string name) { m_name = std::move(name); }
    void               setMaterialId(int id) { m_materialId = id; }
    const char        *getFormat() { return m_format.c_str(); }

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
