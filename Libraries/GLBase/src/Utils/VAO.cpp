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
//  aims the attribute indices
//  VERTEX = 0
//  NORMAL = 1
//  TEXCOORD = 2
//  COLOR = 3
//  TEXCORMOD = 4,
//  MOD_MATR = 5
//  AUX0 = 6
//  AUX1 = 7
//  AUX2 = 8
//  AUX3 = 9
//
//
//  the data is supposed to be organized e.g. (e.g. vncvncvncvnc for 4 vertices,
//  4 normals, 4 colors) shader locations are supposed to be layout(location =
//  0) in vec4 vertex; layout(location = 1) in vec4 normal; layout(location = 2)
//  in vec2 texCoord; layout(location = 3) in vec4 color; layout(location = 4)
//  in vec4 model_matrix; where the latter consumes 4 locations (5-8)

#include "Utils/VAO.h"

#include <string_utils.h>

#include "Meshes/Mesh.h"

#ifndef __EMSCRIPTEN__
#include "Utils/TFO.h"
#endif

using namespace std;

namespace ara {

VAO::VAO() : m_storeMode(GL_DYNAMIC_DRAW), m_maxNrInstances(1) {}

VAO::VAO(std::string &&format, GLenum storeMode, vector<CoordType> *instAttribs, int maxNrInstances, bool createBuffers)
    : m_storeMode(storeMode), m_createBuffers(createBuffers), m_maxNrInstances(maxNrInstances),
      m_instAttribs(instAttribs) {
    init(std::move(format));
}

VAO::VAO(std::string &&format, GLenum storeMode, bool createBuffers)
    : m_storeMode(storeMode), m_createBuffers(createBuffers) {
    init(std::move(format));
}

void VAO::init(std::string &&format, bool createBuffers, bool interleaved) {
    m_createBuffers = createBuffers;
    m_interleaved   = interleaved;

    if (!m_interleaved) {
        for (int i = 0; i < toType(CoordType::Count); i++) {
            m_statCoords.emplace_back(1.f, 1.f, 1.f, 1.f);
        }

        m_usesStaticCoords.resize(toType(CoordType::Count));
        for (int i = 0; i < toType(CoordType::Count); i++) {
            m_usesStaticCoords[i] = false;
        }
    }

    // interpret the format string
    auto   sepFormat       = split(format, ',');
    size_t totVertAttrSize = 0;

    // separate the declarations by semicolon,
    for (auto &it : sepFormat) {
        auto sep = split(it, ':');

        m_attributes.emplace_back(VertexAttribute{
            .type = VertexAttribute::typeMap.at(sep[1][1]),
            .name = sep[0]
        });

        if (sep[0] == "modMatr") {
            m_attributes.back().nrConsecLocs = 4;
        }

        if (sep[0] == "position") {
            m_nrCoordsPerVert = static_cast<int>(m_attributes.back().size);
        }

        m_attributes.back().location = static_cast<GLint>(ranges::find(getStdAttribNames(), sep[0]) - getStdAttribNames().begin());
        try {
            m_attributes.back().size = std::stoi(sep[1]);
        } catch (...) {

        }

        if (m_interleaved) {
            m_attributes.back().pointer = reinterpret_cast<void *>(totVertAttrSize);
        }

        totVertAttrSize += m_attributes.back().getByteSize();
    }

    if (m_interleaved) {
        for (auto &a : m_attributes) {
            a.stride = static_cast<GLsizei>(totVertAttrSize);
        }
    }

    if (m_createBuffers) {
        m_buffers.resize(m_interleaved ? 1 : toType(CoordType::Count), 0);
    }

// instanced drawing is only possible via extensions in webGL
#ifndef __EMSCRIPTEN__
    if (m_instAttribs) {
        for (auto &it : *m_instAttribs) {
            bool found = false;
            for (auto &pIt : m_attributes) {
                if (pIt.type == toType(it)) {
                    pIt.instDiv = 1;
                    found       = true;
                }
            }

            if (!found) {
                // special case, MODMATR needs 4 attributes
                if (it == CoordType::ModMatr) {
                    for (int i = 0; i < 4; i++) {
                        m_attributes.emplace_back(VertexAttribute{
                            .location     = toType(CoordType::ModMatr) + i,
                            .size         = 4,
                            .stride       = sizeof(glm::mat4),
                            .pointer      = reinterpret_cast<void *>(sizeof(glm::vec4) * i),
                            .instDiv      = 1,
                            .nrConsecLocs = 4,
                            .name = (getStdAttribNames()[toType(CoordType::ModMatr)]),
                        });

                    }
                } else {
                    m_attributes.emplace_back();
                    m_attributes.back().setName(getStdAttribNames()[toType(it)]);
                    m_attributes.back().location = toType(it);
                    m_attributes.back().size     = getCoTypeStdSize()[toType(it)];
                    m_attributes.back().instDiv  = 1;
                }
            }
        }
    }
#endif

    //-- init buffers

#ifndef STRICT_WEBGL_1
    glGenVertexArrays(1, &m_VAOId);  // returns 1 unused names for use as VAO in the array VAO
    glBindVertexArray(m_VAOId);      // create VAO, assign the name and bind that array
#endif

    for (auto i = 0; i < m_attributes.size(); i++) {
        auto attrIndex = m_attributes[i].location;
        if (attrIndex < toType(CoordType::ModMatr) || attrIndex > toType(CoordType::ModMatr) + 3) {
            if (m_createBuffers && m_buffers.size() > attrIndex) {
                glGenBuffers(1, &m_buffers[attrIndex]);
                glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attrIndex]);
            }
            m_attributes[i].enable();
        } else {
            // nur ein buffer für alle teile der mod matr
            if (attrIndex == toType(CoordType::ModMatr)) {
                if (m_createBuffers && m_buffers.size() > attrIndex) {
                    glGenBuffers(1, &m_buffers[attrIndex]);
                    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attrIndex]);
                }

                for (int j = 0; j < 4; j++) {
                    m_attributes[i + j].enable();
                }
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);  // create VAO, assign the name and bind that array
#endif

    m_inited = true;
}

void VAO::initData(int nrVert, const GLfloat *data) {
    if (m_createBuffers) {
        for (auto &m_attribute : m_attributes) {
            CoordType uploadType = CoordType::Position;

            for (auto j = 0; j < getStdAttribNames().size(); j++)
                if (getStdAttribNames()[j] == m_attribute.getName()) {
                    uploadType = static_cast<CoordType>(j);
                }

            if (data) {
                upload(uploadType, data, nrVert);
            } else {
                auto *vals = new GLfloat[nrVert * m_attribute.size];
                for (int j = 0; j < nrVert * m_attribute.size; j++) {
                    vals[j] = 0.f;
                }
                upload(uploadType, vals, nrVert);
            }
        }
    } else {
        LOGE << "VAO Error: Vao was build without buffers!!!";
    }
}

void VAO::upload(CoordType type, const GLfloat *entries, size_t nrVertices) {
    // check if CoordType exists
    bool   exists = false;
    int    size   = 0;
    GLuint buffer = 0;

    for (auto &m_attribute : m_attributes) {
        if (getStdAttribNames()[toType(type)] == m_attribute.getName()) {
            exists = true;
            size   = static_cast<int>(m_attribute.size);
            if (m_buffers.size() > m_attribute.location) {
                buffer = m_buffers[m_attribute.location];
            }
        }
    }

    if (exists) {
        if (m_nrVertices == 0) {
            m_nrVertices = static_cast<GLsizei>(nrVertices);
        } else {
            if (m_nrVertices < nrVertices) {
                LOGE << "VAO Warning: nr of Vertices to upload is greater than the nr of vertices of the existing buffers";
            }
        }

#ifndef STRICT_WEBGL_1
        glBindVertexArray(m_VAOId);  // bind that array
#endif
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(GLfloat) * nrVertices * size), entries, m_storeMode);
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifndef STRICT_WEBGL_1
        glBindVertexArray(0);
#endif
    } else {
        LOGE << ("VAO Error: CoordType tried to upload doesn´t exist!");
    }
}

#ifdef __EMSCRIPTEN__
void VAO::uploadFloat(uint32_t location, uint32_t size, GLfloat *_entries, int _nrVertices) {
    if (m_nrVertices == 0)
        m_nrVertices = _nrVertices;
    else {
        if (m_nrVertices != _nrVertices)
            LOGE << "VAO Warning: nr of Vertices to upload is not equivalent "
                    "to the nr of vertices of the existing buffers";
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffers[location]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _nrVertices * size, _entries, m_storeMode);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#endif

void VAO::setElemIndices(size_t count, const GLuint *indices) {
    m_nrElements = static_cast<GLsizei>(count);

    if (!m_elementBuffer) {
        glGenBuffers(1, &m_elementBuffer);
        if (!m_elementBuffer) {
            LOGE << "VAO::setElemIndices Error!!! couldn´t generate ElementBuffer";
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(GLuint) * m_nrElements), indices, m_storeMode);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VAO::setExtElemIndices(size_t count, GLuint buffer) {
    if (buffer != 0) {
        m_elementBuffer = buffer;
        m_nrElements    = static_cast<GLsizei>(count);
    } else {
        LOGE << "VAO::setElemIndices Error!!! couldn´t set external ElementBuffer";
    }
}

void VAO::remove() {
#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);
#else
    for (auto &attr : m_attributes) {
        glDeleteBuffers(1, &buffers[attr.location]);
    }
#endif
    for (auto &attr : m_attributes) {
        if (!attr.isStatic && m_buffers.size() > attr.location) {
            glDeleteBuffers(1, &m_buffers[attr.location]);
        }
    }

    m_nrVertices = 0;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_VAOId);
#endif

    if (m_elementBuffer) {
        glDeleteBuffers(1, &m_elementBuffer);
    }

    m_inited = false;
}

void VAO::resize(size_t newNrVertices) {
#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);
#else
    enableAttributes();
#endif
    int i = 0;
    for (auto &attr : m_attributes) {
        if (!attr.isStatic && m_buffers.size() > attr.location) {
            glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attr.location]);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_interleaved ? (newNrVertices * attr.stride) : (newNrVertices * attr.getByteSize())),
                nullptr,
                m_storeMode);
        }
        ++i;
    }

    m_nrVertices = static_cast<GLsizei>(newNrVertices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);
#endif
}

void VAO::bindBuffer(GLuint inBuffer, GLenum type) const {
#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);  // bind that array
#endif
    glBindBuffer(GL_ARRAY_BUFFER, inBuffer);
}

GLuint VAO::addBuffer(CoordType type) {
    auto v = getStdAttribNames();
    m_attributes.emplace_back(VertexAttribute{
        .location = static_cast<GLint>(ranges::find(v, v[toType(type)]) - v.begin()),
        .size     = getCoTypeStdSize()[toType(type)],
        .stride   = static_cast<GLsizei>(sizeof(float) * getCoTypeStdSize()[toType(type)]),
    });

    string attribName = v[toType(type)];
    if (attribName == "CoordType::ModMatr") {
        m_attributes.back().nrConsecLocs = 4;
    }

    if (attribName == "position") {
        m_nrCoordsPerVert = static_cast<int>(m_attributes.back().size);
    }

#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);  // create VAO, assign the name and bind that array
#endif
    glGenBuffers(1, &m_buffers[m_attributes.back().location]);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[m_attributes.back().location]);
    m_attributes.back().enable();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);  // create VAO, assign the name and bind that array
#endif

    return m_buffers[m_attributes.back().location];
}

void VAO::addExtBuffer(CoordType type, GLuint buffer) {
    VertexAttribute *vAttrib = nullptr;
    auto v          = getStdAttribNames();
    auto attribName     = getStdAttribNames()[toType(type)];
    bool   found                = false;
    for (auto &it : m_attributes)
        if (it.getName() == attribName) {
            found   = true;
            vAttrib = &it;
        }

    // check if the component already exists
    if (!found) {
        m_attributes.emplace_back(VertexAttribute{
            .location = static_cast<GLint>(ranges::find(v, v[toType(type)]) - v.begin()),
            .size     = getCoTypeStdSize()[toType(type)],
            .stride   = static_cast<GLsizei>(sizeof(float) * getCoTypeStdSize()[toType(type)]),
        });

        if (attribName == "CoordType::ModMatr") {
            m_attributes.back().nrConsecLocs = 4;
        }

        if (attribName == "position") {
            m_nrCoordsPerVert = static_cast<int>(m_attributes.back().size);
        }
    }

    if (m_buffers.empty()) {
        m_buffers.resize(toType(CoordType::Count), 0);
    }

    m_buffers[toType(type)] = buffer;

#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);  // create VAO, assign the name and bind that array
#endif

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    vAttrib->enable();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);  // create VAO, assign the name and bind that array
#endif
}

#ifdef __EMSCRIPTEN__
GLint VAO::addBufferFloat(uint32_t location, uint32_t size, const char *name) {
    m_attributes.emplace_back(new VertexAttribute());
    m_attributes.back().setName((GLchar *)name);
    m_attributes.back().location = location;
    m_attributes.back().size     = size;
    m_attributes.back().stride   = sizeof(float) * size;
    m_attributes.back().setType('f');

    if (location >= CoordType::Count) m_buffers.resize(location + 1);

    glGenBuffers(1, &m_buffers[location]);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[location]);
    m_attributes.back().enable();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return m_buffers[location];
}

#endif

#ifdef STRICT_WEBGL_1
void VAO::enableAttributes() {
    for (auto &attr : m_attributes) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[attr.location]);
        attr.enable();
    }
}
#endif

#ifndef EMSCRIPTEN
void VAO::bind(TFO *tfo, GLenum recMode) {
    if (tfo) {
        tfo->pauseAndOffsetBuf(recMode);
    }
#else
void VAO::bind() {
#endif

#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);
#else
    enableAttributes();
#endif

    if (!m_interleaved) {
        for (int i = 0; i < toType(CoordType::Count); i++) {
            if (m_usesStaticCoords.size() > i && m_usesStaticCoords[i]) {
                glVertexAttrib4fv(i, &m_statCoords[i][0]);
            }
        }
    }
}

void VAO::bindElementBuffer() const {
    if (m_elementBuffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    } else {
        LOGE << " VAO::bindElementBuffer Error: could not bind buffer";
    }
}

#ifndef EMSCRIPTEN

void VAO::unbind(TFO *tfo, int incCntr)
#else
void VAO::unbind()
#endif
{
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);
#endif
    if (tfo) {
        tfo->incCounters(incCntr);
    }
}

void VAO::draw(GLenum mode) {
    bind();
    glDrawArrays(mode, 0, static_cast<GLsizei>(m_nrVertices));
    unbind();
}

void VAO::drawElements(GLenum mode) {
    bind();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElements(mode, m_nrElements, m_elementBufferType, nullptr);
    unbind();
}

#ifndef __EMSCRIPTEN__

void VAO::draw(GLenum mode, TFO *tfo, GLenum recMode) {
    VAO::bind(tfo, recMode);
    glDrawArrays(mode, 0, m_nrVertices);
    unbind(tfo, m_nrVertices);
}

void VAO::draw(GLenum mode, GLint offset, GLsizei count, TFO *tfo) {
    VAO::bind(tfo);
    glDrawArrays(mode, offset, count);
    unbind(tfo, count);
}

void VAO::draw(GLenum mode, GLint offset, GLsizei count, TFO *tfo, GLenum recMode) {
    VAO::bind(tfo, recMode);
    glDrawArrays(mode, offset, count);
    unbind(tfo, count);
}

void VAO::drawInstanced(GLenum mode, GLsizei nrInstances, TFO *tfo, float nrVert) {
    VAO::bind(tfo);
    m_drawNrVertices = static_cast<int>(static_cast<float>(m_nrVertices) * nrVert);
    glDrawArraysInstanced(mode, 0, m_drawNrVertices, nrInstances);
    unbind(tfo, m_nrVertices * nrInstances);
}

void VAO::drawElements(GLenum mode, TFO *tfo, GLenum recMode) {
    VAO::bind(tfo, recMode);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElements(mode, m_nrElements, m_elementBufferType, nullptr);
    unbind(tfo, m_nrElements);
}

void VAO::drawElementsInst(GLenum mode, GLsizei nrInstances, TFO *tfo, GLenum recMode) {
    VAO::bind(tfo, recMode);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElementsInstanced(mode, m_nrElements, m_elementBufferType, nullptr, nrInstances);
    unbind(tfo, m_nrElements * nrInstances);
}

void VAO::drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements, int offset) {
    VAO::bind(tfo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElements(mode, nrElements, m_elementBufferType, reinterpret_cast<const GLvoid *>(offset * sizeof(GLuint)));
    unbind(tfo, nrElements);
}

void VAO::drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements) {
    VAO::bind(tfo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElements(mode, nrElements, m_elementBufferType, nullptr);
    unbind(tfo, nrElements);
}

#else

void VAO::draw(GLenum _mode, unsigned int offset, unsigned int count) {
    bind();
    if (!m_interleaved)
        for (int i = 0; i < ARA_COORDTYPE_COUNT; i++)
            if (m_usesStaticCoords[i]) glVertexAttrib4fv(i, &m_statCoords[i][0]);

    glDrawArrays(_mode, offset, count);
}

void VAO::draw(GLenum _mode, unsigned int offset, unsigned int count, GLenum _recMode) {
    bind();
    if (!m_interleaved)
        for (int i = 0; i < ARA_COORDTYPE_COUNT; i++)
            if (m_usesStaticCoords[i]) glVertexAttrib4fv(i, &m_statCoords[i][0]);

    glDrawArrays(_mode, offset, count);
}

void VAO::drawInstanced(GLenum _mode, int nrInstances, float nrVert) {
    bind();
#ifndef STRICT_WEBGL_1
    if (!m_interleaved)
        for (int i = 0; i < ARA_COORDTYPE_COUNT; i++)
            if (m_usesStaticCoords[i]) glVertexAttrib4fv(i, &m_statCoords[i][0]);

    m_drawNrVertices = static_cast<int>(static_cast<float>(m_nrVertices) * nrVert);

    glDrawArraysInstanced(_mode, 0, m_drawNrVertices, nrInstances);
#endif
}

void VAO::drawElements(GLenum _mode, GLenum _recMode) {
    bind();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);

    if (!m_interleaved)
        for (int i = 0; i < ARA_COORDTYPE_COUNT; i++)
            if (m_usesStaticCoords[i]) glVertexAttrib4fv(i, &m_statCoords[i][0]);

    glDrawElements(_mode, m_nrElements, m_elementBufferType, nullptr);
}

void VAO::drawElements(GLenum _mode, GLenum _recMode, int _nrElements) {
    bind();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);

    if (!m_interleaved)
        for (int i = 0; i < ARA_COORDTYPE_COUNT; i++)
            if (m_usesStaticCoords[i]) glVertexAttrib4fv(i, &m_statCoords[i][0]);

    glDrawElements(_mode, _nrElements, m_elementBufferType, nullptr);
}

#endif

void VAO::uploadMesh(Mesh *mesh) {
    if (!m_buffers.empty() && mesh != nullptr) {
        m_nrVertices = mesh->usesIntrl() ? mesh->getNrVertIntrl() / m_nrCoordsPerVert : m_nrVertices = mesh->getNrPositions();

#ifndef STRICT_WEBGL_1
        glBindVertexArray(m_VAOId);
#endif
        // if there are indices, allocate space and upload them
        if (mesh->usesIndices()) {
            m_nrElements = mesh->getNrIndices();

            if (!m_elementBuffer) {
                glGenBuffers(1, &m_elementBuffer);  // generate one element buffer for later use
            }

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
            if (mesh->usesUintIndices()) {
                m_elementBufferType = GL_UNSIGNED_INT;
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->getByteSizeInd(), mesh->getIndiceUintPtr(), m_storeMode);
            } else {
                m_elementBufferType = GL_UNSIGNED_SHORT;
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->getByteSizeInd(), mesh->getIndicePtr(), m_storeMode);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        // Allocate space for the buffer and upload the data, upload data for mod matr later, will usually only be used for instancing
        for (auto &attr : m_attributes) {
            int attrIndex = static_cast<int>(attr.location);

            if ((attrIndex <= toType(CoordType::ModMatr) || attrIndex > (toType(CoordType::ModMatr) + 3)) &&
                !attr.isStatic && m_buffers.size() > attrIndex) {
                glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attrIndex]);

                // if it´s a regular attribute
                if (attr.instDiv == 0) {
                    glBufferData(GL_ARRAY_BUFFER, mesh->getByteSize(attrIndex), mesh->getPtr(attrIndex), m_storeMode);
                } else {
                    // if it's an instanced attribute, make space for the maxNrInstances since for each attrib there´s
                    // an extra buffer pointer
                    glBufferData(GL_ARRAY_BUFFER, attr.getByteSize() * m_maxNrInstances, nullptr, m_storeMode);
                }
            }
        }
#ifndef STRICT_WEBGL_1
        glBindVertexArray(0);
#endif
        if (mesh->usesStaticColor()) {
            setStaticColor(mesh->getStaticColor());
        }

        if (mesh->usesStaticNormal()) {
            setStaticNormal(mesh->getStaticNormal());
        }
    } else {
        LOGE << "VAO::uploadMesh Error: VertexBufferObject or mesh was not generated";
    }
}

/// if a VertexAttribute is not enabled a default value is taken instead of reading from a VBO, this default value is set with
/// glVertexAttrib{1234}{fds}
void VAO::setStatic(glm::vec4 col, const std::string& name, const std::string& format) {
    GLsizei statCoordInd = 0;

    if (name == "color") {
        statCoordInd = toType(CoordType::Color);
    } else if (name == "normal") {
        statCoordInd = toType(CoordType::Normal);
    }

    GLsizei index               = statCoordInd;
    m_statCoords[statCoordInd] = col;

    // look if there is already an attribute for color
    bool found = false;
    for (auto &it : m_attributes)
        if (it.getName() == name) {
            found = true;
            index = it.location;
        }

    if (!m_interleaved) {
        if (!found) {
            m_attributes.emplace_back(VertexAttribute{
                .location = 0,
                .size     = index,
                .type     = VertexAttribute::typeMap.at('f'),
                .isStatic = true,
                .name = name,
            });
        } else if (!m_usesStaticCoords[statCoordInd]) {
#ifndef STRICT_WEBGL_1
            glBindVertexArray(m_VAOId);
            glDisableVertexAttribArray(index);  // disable the attribute
            glBindVertexArray(0);               // causes QT to stop drawing
#endif
        }

        m_usesStaticCoords[statCoordInd] = true;
    }
}

#ifndef __EMSCRIPTEN__

void *VAO::getMapBuffer(CoordType attrIndex) const {
    if (m_buffers.size() <= toType(attrIndex)) {
        throw std::runtime_error("VAO::getMapBuffer Error, trying to access non-existing buffer");
    }

    if (m_buffers[toType(attrIndex)] == 0) {
        throw std::runtime_error("VAO::getMapBuffer Error: trying to bind 0");
    }

    if (m_nrVertices == 0) {
        throw std::runtime_error("VAO::getMapBuffer Error: trying to map empty buffer");
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[toType(attrIndex)]);
    return glMapBufferRange(GL_ARRAY_BUFFER, 0, m_nrVertices, GL_MAP_WRITE_BIT);
}

void *VAO::mapElementBuffer() const {
    if (m_elementBuffer == 0) {
        throw std::runtime_error("VAO::mapElementBuffer Error, m_elementBuffer does not exist");
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    return glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, m_nrElements, GL_MAP_WRITE_BIT);
}

#endif

void VAO::enableVertexAttribs() {
    int i=0;
    for (auto &attr : m_attributes) {
        if (m_buffers.size() > attr.location)
            glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attr.location]);

        if (!attr.isStatic) {
            attr.enable();
        } else {
            glVertexAttrib4fv(i, &m_statCoords[i][0]);
        }
        ++i;
    }
}

void VAO::disableVertexAttribs() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (auto &attr : m_attributes) {
        attr.disable();
    }
}

VAO::~VAO() {
    if (m_inited) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        for (auto &it : m_attributes) {
            if (m_buffers.size() > it.location) {
                glDeleteBuffers(1, &m_buffers[it.location]);
                m_buffers[it.location] = 0;
            }
        }

#ifndef STRICT_WEBGL_1
        glDeleteVertexArrays(1, &m_VAOId);
        glBindVertexArray(0);
#endif
        m_inited = false;
    }
}

}  // namespace ara
