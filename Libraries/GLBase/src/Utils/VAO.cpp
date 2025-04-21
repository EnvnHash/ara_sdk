//
//  VAO.cpp
//
//  Created by Sven Hahne, last edit on 19.08.17.
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
//  in vec4 model_matrix; where the later consumes 4 locations (5-8)

#include "Utils/VAO.h"

#include <string_utils.h>

#include "Meshes/Mesh.h"

#ifndef __EMSCRIPTEN__
#include "Utils/TFO.h"
#endif

using namespace std;

namespace ara {

VAO::VAO() : m_storeMode(GL_DYNAMIC_DRAW), m_maxNrInstances(1), m_createBuffers(true) {}

VAO::VAO(std::string &&format, GLenum storeMode, vector<CoordType> *instAttribs, int maxNrInstances, bool createBuffers)
    : m_storeMode(storeMode), m_instAttribs(instAttribs), m_maxNrInstances(maxNrInstances),
      m_createBuffers(createBuffers) {
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
        for (int i = 0; i < toType(CoordType::Count); i++) m_statCoords.emplace_back(1.f, 1.f, 1.f, 1.f);

        m_usesStaticCoords.resize(toType(CoordType::Count));
        for (int i = 0; i < toType(CoordType::Count); i++) m_usesStaticCoords[i] = false;
    }

    // interpret the format string
    auto   sepFormat       = split(format, ',');
    size_t totVertAttrSize = 0;

    // separate the declarations by semicolon,
    for (auto &it : sepFormat) {
        auto sep = split(it, ':');

        m_attributes.emplace_back();
        m_attributes.back().setName(sep[0]);
        m_attributes.back().setType(sep[1][1]);
        if (sep[0] == "modMatr") m_attributes.back().nrConsecLocs = 4;
        if (sep[0] == "position") m_nrCoordsPerVert = m_attributes.back().size;
        m_attributes.back().location = static_cast<GLint>(
            find(getStdAttribNames().begin(), getStdAttribNames().end(), sep[0]) - getStdAttribNames().begin());
        m_attributes.back().size = atoi(sep[1].c_str());

        if (m_interleaved) m_attributes.back().pointer = (void *)totVertAttrSize;

        totVertAttrSize += m_attributes.back().getByteSize();
    }

    if (m_interleaved)
        for (auto &a : m_attributes) a.stride = (GLsizei)totVertAttrSize;

    if (m_createBuffers) m_buffers.resize(m_interleaved ? 1 : toType(CoordType::Count), 0);

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
                        m_attributes.emplace_back();
                        m_attributes.back().setName(getStdAttribNames()[toType(CoordType::ModMatr)]);
                        m_attributes.back().location     = toType(CoordType::ModMatr) + i;
                        m_attributes.back().size         = 4;
                        m_attributes.back().nrConsecLocs = 4;
                        m_attributes.back().stride       = sizeof(glm::mat4);
                        m_attributes.back().pointer      = (void *)(sizeof(glm::vec4) * i);
                        m_attributes.back().instDiv      = 1;  // könnte auch alles andere sein..
                    }
                } else {
                    m_attributes.emplace_back();
                    m_attributes.back().setName(getStdAttribNames()[toType(it)]);
                    m_attributes.back().location = toType(it);
                    m_attributes.back().size     = getCoTypeStdSize()[toType(it)];
                    m_attributes.back().instDiv  = 1;  // könnte auch alles andere sein..
                }
            }
        }
    }
#endif

    //-- init buffers
    //-------------------------------------------------------------------------------------

#ifndef STRICT_WEBGL_1
    glGenVertexArrays(1, &m_VAOId);  // returns 1 unused names for use as VAO in the array VAO
    glBindVertexArray(m_VAOId);      // create VAO, assign the name and bind that array
#endif

    for (int i = 0; i < static_cast<int>(m_attributes.size()); i++) {
        int attrIndex = (int)m_attributes[i].location;
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

                for (int j = 0; j < 4; j++) m_attributes[i + j].enable();
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);  // create VAO, assign the name and bind that array
#endif

    m_inited = true;
}

void VAO::initData(int nrVert, GLfloat *_data) {
    if (m_createBuffers) {
        for (auto &m_attribute : m_attributes) {
            CoordType uploadType = CoordType::Position;

            for (int j = 0; j < (int)getStdAttribNames().size(); j++)
                if (std::strcmp(getStdAttribNames()[j].c_str(), m_attribute.getName()) == 0) uploadType = (CoordType)j;

            if (_data) {
                upload(uploadType, _data, nrVert);
            } else {
                auto *vals = new GLfloat[nrVert * m_attribute.size];
                for (int j = 0; j < nrVert * m_attribute.size; j++) vals[j] = 0.f;
                upload(uploadType, vals, nrVert);
            }
        }
    } else {
        LOGE << "VAO Error: Vao was build without buffers!!!";
    }
}

void VAO::upload(CoordType type, GLfloat *entries, uint32_t nrVertices) {
    // check if CoordType exists
    bool   exists = false;
    int    size   = 0;
    GLuint buffer = 0;

    for (auto &m_attribute : m_attributes) {
        if (std::strcmp(getStdAttribNames()[(int)type].c_str(), m_attribute.getName()) == 0) {
            exists = true;
            size   = m_attribute.size;
            if (m_buffers.size() > m_attribute.location) buffer = m_buffers[m_attribute.location];
        }
    }

    if (exists) {
        if (m_nrVertices == 0)
            m_nrVertices = nrVertices;
        else {
            if ((uint32_t)m_nrVertices < nrVertices)
                LOGE << "VAO Warning: nr of Vertices to upload is greater than "
                        "the nr of vertices of the existing buffers";
        }

#ifndef STRICT_WEBGL_1
        glBindVertexArray(m_VAOId);  // bind that array
#endif
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(GLfloat) * nrVertices * size), entries, m_storeMode);
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

void VAO::setElemIndices(uint32_t count, GLuint *indices) {
    m_nrElements = count;

    if (!m_elementBuffer) {
        glGenBuffers(1, &m_elementBuffer);
        if (!m_elementBuffer)
            LOGE << "VAO::setElemIndices Error!!! couldn´t generate "
                    "ElementBuffer";
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(GLuint) * m_nrElements), indices, m_storeMode);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VAO::setExtElemIndices(uint32_t count, GLuint buffer) {
    if (buffer != 0) {
        m_elementBuffer = buffer;
        m_nrElements    = count;
    } else {
        LOGE << "VAO::setElemIndices Error!!! couldn´t set external "
                "ElementBuffer";
    }
}

void VAO::remove() {
#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);
#else
    for (int i = 0; i < static_cast<int>(m_attributes.size()); i++) {
        glDeleteBuffers(1, &buffers[m_attributes[i].location]);
    }
#endif
    for (auto &m_attribute : m_attributes)
        if (!m_attribute.isStatic && m_buffers.size() > m_attribute.location)
            glDeleteBuffers(1, &m_buffers[m_attribute.location]);

    m_nrVertices = 0;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_VAOId);
#endif

    if (m_elementBuffer) glDeleteBuffers(1, &m_elementBuffer);

    m_inited = false;
}

void VAO::resize(GLuint newNrVertices) {
#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);
#else
    for (int i = 0; i < static_cast<int>(m_attributes.size()); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[m_attributes[i].location]);
        m_attributes[i].enable();
    }
#endif
    int i = 0;
    for (auto &attr : m_attributes) {
        if (!attr.isStatic && m_buffers.size() > attr.location) {
            glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attr.location]);
            glBufferData(
                GL_ARRAY_BUFFER,
                (GLsizeiptr)(m_interleaved ? (newNrVertices * attr.stride) : (newNrVertices * attr.getByteSize())),
                nullptr, m_storeMode);
        }

        i++;
    }

    m_nrVertices = newNrVertices;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef STRICT_WEBGL_1
    glBindVertexArray(0);
#endif
}

void VAO::bindBuffer(GLuint _inBuffer, GLenum _type) const {
#ifndef STRICT_WEBGL_1
    glBindVertexArray(m_VAOId);  // bind that array
#endif
    glBindBuffer(GL_ARRAY_BUFFER, _inBuffer);
}

GLuint VAO::addBuffer(CoordType _type) {
    string attribName = getStdAttribNames()[int(_type)];

    m_attributes.emplace_back();
    // attributes.back().setName((GLchar*) attribName.c_str());

    vector<string> v             = getStdAttribNames();
    m_attributes.back().location = static_cast<GLint>(find(v.begin(), v.end(), v[int(_type)]) - v.begin());
    m_attributes.back().size     = getCoTypeStdSize()[int(_type)];
    m_attributes.back().stride   = (GLsizei)(sizeof(float) * getCoTypeStdSize()[int(_type)]);
    m_attributes.back().setType('f');

    if (attribName == "CoordType::ModMatr") m_attributes.back().nrConsecLocs = 4;

    if (attribName == "position") m_nrCoordsPerVert = m_attributes.back().size;

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

void VAO::addExtBuffer(CoordType _type, GLuint buffer) {
    VertexAttribute *vAttrib;

    string attribName    = getStdAttribNames()[int(_type)];
    auto  *attribNameChr = (GLchar *)attribName.c_str();
    bool   found         = false;
    for (auto &it : m_attributes)
        if (strcmp(it.getName(), attribName.c_str()) == 0) {
            found   = true;
            vAttrib = &it;
        }

    // check if the the component already exists
    if (!found) {
        m_attributes.emplace_back();

        vector<string> v             = getStdAttribNames();
        m_attributes.back().location = static_cast<GLint>(find(v.begin(), v.end(), v[int(_type)]) - v.begin());
        m_attributes.back().size     = getCoTypeStdSize()[int(_type)];
        m_attributes.back().stride   = (GLsizei)(sizeof(float) * getCoTypeStdSize()[int(_type)]);
        m_attributes.back().setType('f');

        if (attribName == "CoordType::ModMatr") m_attributes.back().nrConsecLocs = 4;
        if (attribName == "position") m_nrCoordsPerVert = m_attributes.back().size;
    }

    if (m_buffers.empty()) m_buffers.resize(toType(CoordType::Count), 0);

    m_buffers[int(_type)] = buffer;

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
    m_attributes.push_back(new VertexAttribute());
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
    for (int i = 0; i < static_cast<int>(m_attributes.size()); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[m_attributes[i].location]);
        m_attributes[i].enable();
    }
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
    bind(tfo, recMode);
    glDrawArrays(mode, 0, (GLsizei)m_nrVertices);
    unbind(tfo, m_nrVertices);
}

void VAO::draw(GLenum mode, GLint offset, GLsizei count, TFO *tfo) {
    bind(tfo);
    glDrawArrays(mode, offset, count);
    unbind(tfo, count);
}

void VAO::draw(GLenum mode, GLint offset, GLsizei count, TFO *tfo, GLenum recMode) {
    bind(tfo, recMode);
    glDrawArrays(mode, offset, count);
    unbind(tfo, count);
}

void VAO::drawInstanced(GLenum mode, GLuint nrInstances, TFO *tfo, float nrVert) {
    bind(tfo);
    m_drawNrVertices = static_cast<int>(static_cast<float>(m_nrVertices) * nrVert);
    glDrawArraysInstanced(mode, 0, m_drawNrVertices, nrInstances);
    unbind(tfo, m_nrVertices * nrInstances);
}

void VAO::drawElements(GLenum mode, TFO *tfo, GLenum recMode) {
    bind(tfo, recMode);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElements(mode, m_nrElements, m_elementBufferType, nullptr);
    unbind(tfo, m_nrElements);
}

void VAO::drawElementsInst(GLenum mode, GLuint nrInstances, TFO *tfo, GLenum recMode) {
    bind(tfo, recMode);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElementsInstanced(mode, m_nrElements, m_elementBufferType, nullptr, nrInstances);
    unbind(tfo, m_nrElements * nrInstances);
}

void VAO::drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements, int offset) {
    bind(tfo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    glDrawElements(mode, nrElements, m_elementBufferType, (const GLvoid *)(offset * sizeof(GLuint)));
    unbind(tfo, nrElements);
}

void VAO::drawElements(GLenum mode, TFO *tfo, GLenum recMode, int nrElements) {
    bind(tfo);
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
        if (mesh->usesIntrl())
            m_nrVertices = mesh->getNrVertIntrl() / m_nrCoordsPerVert;
        else
            m_nrVertices = mesh->getNrPositions();

#ifndef STRICT_WEBGL_1
        glBindVertexArray(m_VAOId);
#endif
        // if there are indices, allocate space and upload them
        if (mesh->usesIndices()) {
            m_nrElements = mesh->getNrIndices();

            if (!m_elementBuffer)
                glGenBuffers(1, &m_elementBuffer);  // generate one element
                                                    // buffer for later use

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

        // Allocate space for the buffer and upload the data
        // upload data for mod matr later, will usually only be used for
        // instancing
        for (auto &attr : m_attributes) {
            int attrIndex = attr.location;

            if ((attrIndex <= toType(CoordType::ModMatr) || attrIndex > (toType(CoordType::ModMatr) + 3)) &&
                !attr.isStatic && m_buffers.size() > attrIndex) {
                glBindBuffer(GL_ARRAY_BUFFER, m_buffers[attrIndex]);

                // if it´s a regular attribute
                if (attr.instDiv == 0) {
                    glBufferData(GL_ARRAY_BUFFER, mesh->getByteSize(attrIndex), mesh->getPtr(attrIndex), m_storeMode);
                } else {
                    // if it´s an instanced attribute, make space for the
                    // maxNrInstances since for each attrib there´s an extra
                    // buffer pointer
                    glBufferData(GL_ARRAY_BUFFER, attr.getByteSize() * m_maxNrInstances, nullptr, m_storeMode);
                    // glBufferData(GL_ARRAY_BUFFER, m_attribute.getByteSize() *
                    // m_maxNrInstances, &m_instData[attrIndex][0],
                    // m_storeMode);
                }

                //                glBufferData(GL_ARRAY_BUFFER,
                //                mesh->getTotalByteSize(),
                //                mesh->getPtrInterleaved(), m_storeMode);
            }
        }
#ifndef STRICT_WEBGL_1
        glBindVertexArray(0);
#endif

        if (mesh->usesStaticColor()) setStaticColor(mesh->getStaticColor());

        if (mesh->usesStaticNormal()) setStaticNormal(mesh->getStaticNormal());
    } else {
        LOGE << "VAO::uploadMesh Error: VertexBufferObject or mesh was not "
                "generated";
    }
}

/// if a VertexAttribute is not enabled a default value is taken
/// instead of reading from a VBO, this default value is set with
/// glVertexAttrib{1234}{fds}
void VAO::setStatic(float r, float g, float b, float a, char *name, char *format) {
    int statCoordInd = 0;

    if (std::strcmp(name, "color") == 0)
        statCoordInd = toType(CoordType::Color);
    else if (std::strcmp(name, "normal") == 0)
        statCoordInd = toType(CoordType::Normal);

    GLint  size                = getCoTypeStdSize()[statCoordInd];
    GLuint index               = static_cast<int>(statCoordInd);
    m_statCoords[statCoordInd] = glm::vec4{r, g, b, a};

    // look if there is already an attribute for color
    bool found = false;
    for (auto &it : m_attributes)
        if (!std::strcmp(it.getName(), name)) {
            found = true;
            index = it.location;
        }

    if (!m_interleaved) {
        if (!found) {
            m_attributes.emplace_back();
            m_attributes.back().setName(name);
            m_attributes.back().location = index;
            m_attributes.back().size     = index;
            m_attributes.back().setType('f');
            m_attributes.back().isStatic = true;
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

void *VAO::getMapBuffer(CoordType attrIndex) {
    if (m_buffers.size() <= toType(attrIndex)) {
        LOGE << "VAO::getMapBuffer Error, trying to access non-existing buffer";
        return nullptr;
    }

    if (m_buffers[toType(attrIndex)] == 0) {
        LOGE << " VAO::getMapBuffer Error: trying to bind 0 ";
        return nullptr;
    }

    if (m_nrVertices == 0) {
        LOGE << "VAO::getMapBuffer Error: trying to map empty buffer";
        return nullptr;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[toType(attrIndex)]);
    return glMapBufferRange(GL_ARRAY_BUFFER, 0, m_nrVertices, GL_MAP_WRITE_BIT);
}

void *VAO::mapElementBuffer() {
    if (m_elementBuffer == 0) {
        LOGE << "VAO::mapElementBuffer Error, m_elementBuffer does not exist";
        return nullptr;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
    return glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, m_nrElements, GL_MAP_WRITE_BIT);
}

#endif

void VAO::enableVertexAttribs() {
    for (int i = 0; i < static_cast<int>(m_attributes.size()); i++) {
        if (m_buffers.size() > m_attributes[i].location)
            glBindBuffer(GL_ARRAY_BUFFER, m_buffers[m_attributes[i].location]);

        if (!m_attributes[i].isStatic)
            m_attributes[i].enable();
        else
            glVertexAttrib4fv(i, &m_statCoords[i][0]);
    }
}

void VAO::disableVertexAttribs() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (auto &m_attribute : m_attributes) m_attribute.disable();
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
