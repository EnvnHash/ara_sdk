//
//  TFO.cpp
//
//  Created by Sven Hahne on 25.08.14.
//

#include "TFO.h"

namespace ara {

TFO::TFO(int bufSize, std::vector<std::string> &parNames) : m_bufSize(bufSize), m_parNames(parNames) {
    init();
}

TFO::TFO() : m_bufSize(524288) {
    for (auto i = 0; i < MAX_SEP_REC_BUFS; i++) {
        m_parNames.emplace_back(getStdRecAttribNames()[i]);
    }
    init();
}

void TFO::init() {
#ifndef ARA_USE_GLES31
    m_primitivesWritten         = 0;
    m_totalPrimsWritten         = 0;
    m_resElementBuffer          = 0;
    m_depthTest                 = true;
    m_recVertOffs               = 0;

    m_nrPar = static_cast<uint32_t>(m_parNames.size());

    if (m_nrPar > MAX_SEP_REC_BUFS) {
        LOGE << "TFO Error: More than 4 separate Buffer requested!!!";
    }

    glGenTransformFeedbacks(1, &m_tfo);  // id 0 is the default TFO
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo);

    m_buffers.resize(m_nrPar);
    m_bufferSizes.resize(m_nrPar);
    std::fill(m_bufferSizes.begin(), m_bufferSizes.end(), m_bufSize);
    m_tbos.resize(m_nrPar);

    glGenBuffers(m_nrPar, &m_buffers[0]);  // generate m_buffers for all attributes

    uint32_t i = 0;
    for (auto &parName : m_parNames) {
        std::vector<std::string> v        = getStdRecAttribNames();
        uint32_t                 fragSize = static_cast<uint32_t>(std::find(v.begin(), v.end(), parName) - v.begin());

        m_locations.push_back(fragSize);
        fragSize = getRecCoTypeFragSize()[fragSize];

        // Bind it to the TRANSFORM_FEEDBACK binding to create it
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffers[i]);

        // Call glBufferData to allocate 1MB of space
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,        // target
                     m_bufSize * fragSize * sizeof(float),  // size
                     nullptr,                             // no initial data
                     GL_DYNAMIC_DRAW);                    // usage

        // Now we can bind it to indexed buffer binding points.
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,  // target
                          i,                             // index 0
                          m_buffers[i],                    // buffer name
                          0,                             // start of range
                          m_bufSize * fragSize * sizeof(float));

        m_fragSizes.emplace_back(fragSize);
        ++i;
    }

    // generate texture buffer for later access
    glGenTextures(m_nrPar, m_tbos.data());

    for (uint32_t j = 0; j < m_nrPar; ++j) {
        glBindTexture(GL_TEXTURE_BUFFER, m_tbos[j]);
        switch (m_fragSizes[j]) {
            case 1: glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_buffers[j]); break;
            case 2: glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, m_buffers[j]); break;
            case 3: glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_buffers[j]); break;
            case 4: glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_buffers[j]); break;
        }
    }

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    // generate VAO for replay
    glGenVertexArrays(1, &m_resVAO);
    glBindVertexArray(m_resVAO);

    for (uint32_t j = 0; j < m_nrPar; ++j) {
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers[j]);
        glVertexAttribPointer(m_locations[j], m_fragSizes[j], GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(m_locations[j]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_statColor[0] = 0.f;
    m_statColor[1] = 0.f;
    m_statColor[2] = 0.f;
    m_statColor[3] = 1.f;

    m_recColors.resize(MAX_NUM_COL_SCENE);
    std::fill(m_recColors.begin(), m_recColors.end(), glm::vec4{0.f, 0.f, 0.f, 1.f});

    m_srcMode = GL_SRC_ALPHA;
    m_dstMode = GL_ONE_MINUS_SRC_ALPHA;
#endif
}

// tfo doesn't exist until this is called
void TFO::bind(bool resetCounters) {
    if (resetCounters) {
        m_recVertOffs       = 0;
        m_primitivesWritten = 0;
        m_totalPrimsWritten = 0;
        m_textures.clear();
        resetObjOffset();
    }

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo);
}

void TFO::unbind() { glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0); }

void TFO::setVaryingsToRecord(std::vector<std::string> *names, GLuint prog) {
    glTransformFeedbackVaryings(prog, static_cast<int>(names->size()),
                                reinterpret_cast<const GLchar *const *>(names->data()), GL_SEPARATE_ATTRIBS);
}

void TFO::setVaryingsToRecordInternNames(GLuint prog) {
    glTransformFeedbackVaryings(prog, (int)m_varyingsNames.size(),
                                reinterpret_cast<const GLchar *const *>(m_varyingsNames.data()), GL_SEPARATE_ATTRIBS);
}

void TFO::begin(GLenum mode) {
    m_lastMode = mode;
    glBeginTransformFeedback(mode);
}

void TFO::pauseAndOffsetBuf(GLenum _mode) {
    int offs = m_recVertOffs;

    end();

    for (uint32_t i = 0; i < m_nrPar; i++) {
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,         // target
                          i,                                    // index 0
                          m_buffers[i],                           // buffer name
                          offs * m_fragSizes[i] * sizeof(float),  // start of range
                          m_bufSize * m_fragSizes[i] * sizeof(float));
    }

    begin(_mode);
}

void TFO::offsetBuf() {
    for (uint32_t i = 0; i < m_nrPar; i++)
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,                // target
                          i,                                           // index 0
                          m_buffers[i],                                  // buffer name
                          m_recVertOffs * m_fragSizes[i] * sizeof(float),  // start of range
                          m_bufSize * sizeof(float));
}

void TFO::incCounters(int nrVertices) {
    m_recVertOffs += nrVertices;
    m_totalPrimsWritten += nrVertices;
    setObjOffset();
}

void TFO::decCounters(int nrVertices) {
    m_recVertOffs -= nrVertices;
    m_totalPrimsWritten -= nrVertices;
}

void TFO::draw(GLenum mode, TFO *tfo, GLenum recToMode, int geoAmpAmt) const {
    if (tfo) {
        tfo->pauseAndOffsetBuf(recToMode);
    }

    glBindVertexArray(m_resVAO);
    glDrawArrays(mode, 0, m_totalPrimsWritten);
    glBindVertexArray(0);

    if (tfo) {
        tfo->incCounters(m_totalPrimsWritten * geoAmpAmt);
    }
}

void TFO::draw(GLenum mode, GLuint offset, GLuint count, TFO *tfo, GLenum recToMode, int geoAmpAmt) const {
    if (tfo) {
        tfo->pauseAndOffsetBuf(recToMode);
    }

    glBindVertexArray(m_resVAO);
    glDrawArrays(mode, offset, count);
    glBindVertexArray(0);

    if (tfo) {
        tfo->incCounters(count * geoAmpAmt);
    }
}

void TFO::setIndices(GLuint count, GLenum type, GLvoid *indices) {
    if (m_resElementBuffer == 0) {
        glGenBuffers(1, &m_resElementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resElementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(type), indices, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        m_resElementBufferSize = count * sizeof(type);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resElementBuffer);

    if (m_resElementBufferSize != count * sizeof(type)) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(type), indices, GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count * sizeof(type), indices);
    }
}

void TFO::drawElements(GLenum mode, GLuint count, GLenum type, TFO *tfo, GLenum _recToMode, int geoAmpAmt) {
    if (tfo) {
        tfo->pauseAndOffsetBuf(_recToMode);
    }

    if (m_resElementBuffer) {
        glBindVertexArray(m_resVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resElementBuffer);
        glDrawElements(mode, count, type, nullptr);
        glBindVertexArray(0);
    } else {
        LOGE << "TFO::drawElements Error: no GL_ELEMENT_ARRAY_BUFFER created!!! ";
    }

    if (tfo) {
        tfo->incCounters(count * geoAmpAmt);
    }
}

void TFO::drawElementsBaseVertex(GLenum mode, GLuint count, GLenum type, GLint basevertex, TFO *tfo,
                                 GLenum _recToMode, int geoAmpAmt) {
#ifndef ARA_USE_GLES31
    if (tfo) {
        tfo->pauseAndOffsetBuf(_recToMode);
    }

    if (m_resElementBuffer) {
        glBindVertexArray(m_resVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resElementBuffer);
        glDrawElementsBaseVertex(mode, count, type, nullptr, basevertex);
        glBindVertexArray(0);
    } else {
        LOGE << "TFO::drawElementsBaseVertex Error: no GL_ELEMENT_ARRAY_BUFFER created!!! ";
    }

    if (tfo) {
        tfo->incCounters(count * geoAmpAmt);
    }
#endif
}

void TFO::resizeTFOBuf(CoordType _nr, uint32_t size) {
    auto it = std::find(m_parNames.begin(), m_parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != m_parNames.end()) {
        uint32_t fragSize = getRecCoTypeFragSize()[toType(_nr)];
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers[it - m_parNames.begin()]);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,       // target
                     size * fragSize * sizeof(GLfloat),  // size
                     nullptr,                            // no initial data
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_bufferSizes[it - m_parNames.begin()] = size;
    } else {
        LOGE << "TFO::resizeTFOBuf Error: index out of range";
    }
}

GLuint TFO::getTFOBufSize(CoordType _nr) {
    GLuint out = 0;
    auto   it  = std::find(m_parNames.begin(), m_parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != m_parNames.end()) {
        out = m_bufferSizes[it - m_parNames.begin()];
    } else {
        LOGE << "TFO::getTFOBufSize Error: index out of range";
    }

    return out;
}

GLuint TFO::getTFOBuf(CoordType _nr) {
    GLuint out = 0;
    auto   it  = std::find(m_parNames.begin(), m_parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != m_parNames.end()) {
        out = m_buffers[it - m_parNames.begin()];
    } else {
        printf("TFO::getTFOBuf Error: index out of range \n");
    }

    return out;
}

uint32_t TFO::getTFOBufFragSize(CoordType nr) {
    uint32_t out = 0;
    auto     it  = std::find(m_parNames.begin(), m_parNames.end(), getStdRecAttribNames()[toType(nr)]);
    if (it != m_parNames.end()) {
        out = getRecCoTypeFragSize()[toType(nr)];
    } else {
        LOGE << "TFO::resizeTFOBuf Error: index out of range";
    }

    return out;
}

void TFO::setBlendMode(GLenum srcMode, GLenum dstMode) {
    m_srcMode = srcMode;
    m_dstMode = dstMode;
}

void TFO::setSceneNodeColors(glm::vec4 *cols) {
    for (int i = 0; i < MAX_NUM_COL_SCENE; i++) {
        m_recColors[i] = cols[i];
    }
}

void TFO::setVaryingsNames(std::vector<std::string> &names) {
    for (auto &name : names) {
        m_varyingsNames.emplace_back(name);
    }
}

void TFO::setObjOffset() {
    if (static_cast<int>(m_objOffset.size()) == 0) {
        m_objOffset.emplace_back(0, m_totalPrimsWritten);
    } else {
        uint32_t lastEnd = m_objOffset.back().first + m_objOffset.back().second;
        m_objOffset.emplace_back(lastEnd, m_totalPrimsWritten - lastEnd);
    }
}

void TFO::addTexture(int unit, int texInd, GLenum target, const std::string &name) {
    bool found = false;
    for (auto &texture : m_textures)
        if (texture.unitNr == unit) {
            found = true;
            texture.target = target;
            texture.texNr  = texInd;
            texture.unitNr = unit;
            texture.name   = name;
        }

    if (!found) {
        m_textures.emplace_back();
        m_textures.back().target = target;
        m_textures.back().texNr  = texInd;
        m_textures.back().unitNr = unit;
        m_textures.back().name   = name;
    }
}

void TFO::recallDepthTestState() const {
    if (!m_depthTest) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }
}


TFO::~TFO() {
    glDeleteTransformFeedbacks(1, &m_tfo);
    glDeleteBuffers(m_nrPar, m_tbos.data());
    glDeleteBuffers(m_nrPar, m_buffers.data());
    glDeleteVertexArrays(1, &m_resVAO);

}
}  // namespace ara
