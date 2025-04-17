//
//  TFO.cpp
//
//  Created by Sven Hahne on 25.08.14.
//

#include "TFO.h"

namespace ara {

// on amd radeon 6490M osx 4 seems to be max with separate buffers...

TFO::TFO(int _bufSize, std::vector<std::string> &_parNames) : bufSize(_bufSize), parNames(_parNames) { init(); }

TFO::TFO() : bufSize(524288) {
    for (auto i = 0; i < MAX_SEP_REC_BUFS; i++) parNames.push_back(getStdRecAttribNames()[i]);
    init();
}

void TFO::init() {
#ifndef ARA_USE_GLES31
    primitivesWritten         = 0;
    totalPrimsWritten         = 0;
    resElementBuffer          = 0;
    resElementBufferNrIndices = 0;
    depthTest                 = true;
    recNrVert                 = 0;
    recVertOffs               = 0;

    nrPar = static_cast<uint32_t>(parNames.size());

    if (nrPar > MAX_SEP_REC_BUFS)
        printf(
            "TFO Error: More than 4 separate Buffer requested!!! Actually only "
            "4 supported by Hardware (radeon 6490m)\n");

    glGenTransformFeedbacks(1, &tfo);  // id 0 is the default TFO
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo);

    buffers     = new GLuint[nrPar];
    bufferSizes = new GLuint[nrPar];
    for (uint32_t i = 0; i < nrPar; i++) bufferSizes[i] = bufSize;
    tbos = new GLuint[nrPar];

    glGenBuffers(nrPar, &buffers[0]);  // generate buffers for all attributes

    uint32_t i = 0;
    for (auto &parName : parNames) {
        std::vector<std::string> v        = getStdRecAttribNames();
        uint32_t                 fragSize = static_cast<uint32_t>(std::find(v.begin(), v.end(), parName) - v.begin());

        locations.push_back(fragSize);
        fragSize = getRecCoTypeFragSize()[fragSize];

        // Bind it to the TRANSFORM_FEEDBACK binding to create it
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffers[i]);

        // Call glBufferData to allocate 1MB of space
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,        // target
                     bufSize * fragSize * sizeof(float),  // size
                     nullptr,                             // no initial data
                     GL_DYNAMIC_DRAW);                    // usage

        // Now we can bind it to indexed buffer binding points.
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,  // target
                          i,                             // index 0
                          buffers[i],                    // buffer name
                          0,                             // start of range
                          bufSize * fragSize * sizeof(float));

        // fragSizes need below
        fragSizes.push_back(fragSize);
        i++;
    }

    // generate texture buffer for later access
    glGenTextures(nrPar, &tbos[0]);

    for (uint32_t j = 0; j < nrPar; j++) {
        glBindTexture(GL_TEXTURE_BUFFER, tbos[j]);
        switch (fragSizes[j]) {
            case 1: glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffers[j]); break;
            case 2: glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, buffers[j]); break;
            case 3: glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, buffers[j]); break;
            case 4: glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers[j]); break;
        }
    }

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    // generate VAO for replay
    glGenVertexArrays(1, &resVAO);
    glBindVertexArray(resVAO);

    for (uint32_t j = 0; j < nrPar; j++) {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[j]);
        glVertexAttribPointer(locations[j], fragSizes[j], GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(locations[j]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    statColor    = new GLfloat[4];
    statColor[0] = 0.f;
    statColor[1] = 0.f;
    statColor[2] = 0.f;
    statColor[3] = 1.f;

    recColors = new glm::vec4[MAX_NUM_COL_SCENE];
    for (int j = 0; j < MAX_NUM_COL_SCENE; j++) recColors[j] = glm::vec4{0.f, 0.f, 0.f, 1.f};

    srcMode = GL_SRC_ALPHA;
    dstMode = GL_ONE_MINUS_SRC_ALPHA;
#endif
}

// tfo doesn´t exist until this is called
void TFO::bind(bool _resetCounters) {
    if (_resetCounters) {
        recVertOffs       = 0;
        primitivesWritten = 0;
        totalPrimsWritten = 0;
        textures.clear();
        resetObjOffset();
    }

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo);
}

void TFO::unbind() { glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0); }

void TFO::setVaryingsToRecord(std::vector<std::string> *names, GLuint _prog) {
    const char **vars = new const char *[names->size()];

    int ind = 0;
    for (auto &name : *names) {
        vars[ind] = name.c_str();
        ind++;
    };

    glTransformFeedbackVaryings(_prog, (int)names->size(), (const char **)vars, GL_SEPARATE_ATTRIBS);
}

void TFO::setVaryingsToRecordInternNames(GLuint _prog) {
    const char **vars = new const char *[varyingsNames.size()];

    int ind = 0;
    for (auto &varyingsName : varyingsNames) {
        vars[ind] = varyingsName.c_str(), ind++;
    };

    glTransformFeedbackVaryings(_prog, (int)varyingsNames.size(), (const char **)vars, GL_SEPARATE_ATTRIBS);
}

void TFO::begin(GLenum _mode) {
    lastMode = _mode;
    glBeginTransformFeedback(_mode);
}

// tfo will still be active!!!! may cause gl errors
void TFO::pause() { glPauseTransformFeedback(); }

void TFO::resume() { glResumeTransformFeedback(); }

void TFO::end() {
    glEndTransformFeedback();

    // read values from recorded buffer
    //            glm::vec4* pos = (glm::vec4*)
    //            glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY); for
    //            (int i=0;i<20;i++)  LOG << (glm::to_string(pos[i]) <<
    //            std::endl; glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
}

void TFO::pauseAndOffsetBuf(GLenum _mode) {
    int offs = recVertOffs;

    end();

    for (uint32_t i = 0; i < nrPar; i++) {
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,         // target
                          i,                                    // index 0
                          buffers[i],                           // buffer name
                          offs * fragSizes[i] * sizeof(float),  // start of range
                          bufSize * fragSizes[i] * sizeof(float));

        // if ((bufSize * sizeof(float)) < (offs * coTypeFragSize[i] *
        // sizeof(float))) printf("der \n");
    }

    begin(_mode);
}

void TFO::offsetBuf() {
    for (uint32_t i = 0; i < nrPar; i++)
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,                // target
                          i,                                           // index 0
                          buffers[i],                                  // buffer name
                          recVertOffs * fragSizes[i] * sizeof(float),  // start of range
                          bufSize * sizeof(float));
}

void TFO::incCounters(int nrVertices) {
    recVertOffs += nrVertices;
    totalPrimsWritten += nrVertices;
    setObjOffset();
}

void TFO::decCounters(int nrVertices) {
    recVertOffs -= nrVertices;
    totalPrimsWritten -= nrVertices;
}

// verschiedene draw and recToMode, z.B. fuer Geo-Amplification
void TFO::draw(GLenum _mode, TFO *_tfo, GLenum _recToMode, int geoAmpAmt) const {
    if (_tfo) _tfo->pauseAndOffsetBuf(_recToMode);

    // printf("drawing %d \n", totalPrimsWritten);

    glBindVertexArray(resVAO);
    glDrawArrays(_mode, 0, totalPrimsWritten);
    glBindVertexArray(0);

    // wenn für transform feedback gerendert wird
    if (_tfo) _tfo->incCounters(totalPrimsWritten * geoAmpAmt);
}

// verschiedene draw and recToMode, z.B. fuer Geo-Amplification
void TFO::draw(GLenum _mode, GLuint offset, GLuint count, TFO *_tfo, GLenum _recToMode, int geoAmpAmt) const {
    if (_tfo) _tfo->pauseAndOffsetBuf(_recToMode);

    glBindVertexArray(resVAO);
    glDrawArrays(_mode, offset, count);
    glBindVertexArray(0);

    // wenn für transform feedback gerendert wird
    if (_tfo) _tfo->incCounters(count * geoAmpAmt);
}

void TFO::setIndices(GLuint _count, GLenum _type, GLvoid *_indices) {
    if (resElementBuffer == 0) {
        glGenBuffers(1, &resElementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _count * sizeof(_type), _indices, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        resElementBufferSize = _count * sizeof(_type);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);

    // wenn die groesse sich geändert hat, erweitere den buffer
    if (resElementBufferSize != _count * sizeof(_type)) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _count * sizeof(_type), _indices, GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _count * sizeof(_type), _indices);
    }
}

// verschiedene draw and recToMode, z.B. fuer Geo-Amplification
void TFO::drawElements(GLenum _mode, GLuint _count, GLenum _type, TFO *_tfo, GLenum _recToMode, int geoAmpAmt) {
    if (_tfo) _tfo->pauseAndOffsetBuf(_recToMode);

    if (resElementBuffer) {
        glBindVertexArray(resVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);
        glDrawElements(_mode, _count, _type, nullptr);
        glBindVertexArray(0);
    } else {
        printf(
            "TFO::drawElements Error: no GL_ELEMENT_ARRAY_BUFFER created!!! "
            "\n");
    }

    // wenn für transform feedback gerendert wird
    if (_tfo) _tfo->incCounters(_count * geoAmpAmt);
}

void TFO::drawElementsBaseVertex(GLenum _mode, GLuint _count, GLenum _type, GLint basevertex, TFO *_tfo,
                                 GLenum _recToMode, int geoAmpAmt) {
#ifndef ARA_USE_GLES31
    if (_tfo) _tfo->pauseAndOffsetBuf(_recToMode);

    if (resElementBuffer) {
        glBindVertexArray(resVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resElementBuffer);
        glDrawElementsBaseVertex(_mode, _count, _type, nullptr, basevertex);
        glBindVertexArray(0);
    } else {
        printf(
            "TFO::drawElementsBaseVertex Error: no GL_ELEMENT_ARRAY_BUFFER "
            "created!!! \n");
    }

    // wenn für transform feedback gerendert wird
    if (_tfo) _tfo->incCounters(_count * geoAmpAmt);
#endif
}

void TFO::resizeTFOBuf(CoordType _nr, uint32_t size) {
    auto it = std::find(parNames.begin(), parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != parNames.end()) {
        uint32_t fragSize = getRecCoTypeFragSize()[toType(_nr)];
        glBindBuffer(GL_ARRAY_BUFFER, buffers[it - parNames.begin()]);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,       // target
                     size * fragSize * sizeof(GLfloat),  // size
                     nullptr,                            // no initial data
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        bufferSizes[it - parNames.begin()] = size;
    } else
        printf("TFO::resizeTFOBuf Error: index out of range \n");
}

GLuint TFO::getTFOBufSize(CoordType _nr) {
    GLuint out = 0;
    auto   it  = std::find(parNames.begin(), parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != parNames.end())
        out = bufferSizes[it - parNames.begin()];
    else
        printf("TFO::getTFOBufSize Error: index out of range \n");

    return out;
}

GLuint TFO::getTFOBuf(CoordType _nr) {
    GLuint out = 0;
    auto   it  = std::find(parNames.begin(), parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != parNames.end())
        out = buffers[it - parNames.begin()];
    else
        printf("TFO::getTFOBuf Error: index out of range \n");

    return out;
}

uint32_t TFO::getTFOBufFragSize(CoordType _nr) {
    uint32_t out = 0;
    auto     it  = std::find(parNames.begin(), parNames.end(), getStdRecAttribNames()[toType(_nr)]);
    if (it != parNames.end()) {
        out = getRecCoTypeFragSize()[toType(_nr)];
    } else
        printf("TFO::resizeTFOBuf Error: index out of range \n");

    return out;
}

GLuint TFO::getId() const { return tfo; }

GLuint TFO::getTbo(CoordType _coord) { return tbos[toType(_coord)]; }

GLuint TFO::getRecVertOffs() const { return recVertOffs; }

GLuint TFO::getTotalPrimWritten() const { return totalPrimsWritten; }

GLuint TFO::getBufSize() const { return bufSize; }

GLuint TFO::getVao() const { return resVAO; }

glm::vec4 TFO::getColor(int ind) { return recColors[ind]; }

GLenum TFO::getLastMode() const { return lastMode; }

void TFO::setBlendMode(GLenum _srcMode, GLenum _dstMode) {
    srcMode = _srcMode;
    dstMode = _dstMode;
}

void TFO::setSceneNodeColors(glm::vec4 *_cols) {
    for (int i = 0; i < MAX_NUM_COL_SCENE; i++) recColors[i] = _cols[i];
}

void TFO::setSceneProtoName(std::string *_protoName) { recProtoName = _protoName; }

void TFO::setVaryingsNames(std::vector<std::string> &_names) {
    for (auto &_name : _names) varyingsNames.push_back(_name);
}

void TFO::setRecNrVert(int _nr) { recNrVert = _nr; }

void TFO::setRecVertOffs(int _nr) { recVertOffs = _nr; }

void TFO::setObjOffset() {
    if (static_cast<int>(objOffset.size()) == 0) {
        objOffset.emplace_back(0, totalPrimsWritten);
    } else {
        uint32_t lastEnd = objOffset.back().first + objOffset.back().second;
        objOffset.emplace_back(lastEnd, totalPrimsWritten - lastEnd);
    }
}

std::vector<std::pair<uint32_t, uint32_t> > *TFO::getObjOffsets() { return &objOffset; }

void TFO::resetObjOffset() { objOffset.clear(); }

void TFO::incRecVertOffs(int _nr) { recVertOffs += _nr; }

void TFO::addTexture(int _unit, int _texInd, GLenum _target, const std::string &_name) {
    // printf("TFO::AddTexture \n");
    bool found = false;
    for (auto &texture : textures)
        if (texture.unitNr == _unit) {
            found = true;
            // printf("TFO::AddTexture overwriting unit %d \n", _unit);
            texture.target = _target;
            texture.texNr  = _texInd;
            texture.unitNr = _unit;
            texture.name   = _name;
        }

    if (!found) {
        //  printf("TFO::AddTexture adding unit %d name %s \n", _unit,
        //  _name.c_str());

        textures.emplace_back();
        textures.back().target = _target;
        textures.back().texNr  = _texInd;
        textures.back().unitNr = _unit;
        textures.back().name   = _name;
    }
}

void TFO::enableDepthTest() { depthTest = true; }

void TFO::disableDepthTest() { depthTest = false; }

void TFO::recallDepthTestState() const {
    if (!depthTest) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }
}

void TFO::recallBlendMode() const { glBlendFunc(srcMode, dstMode); }

TFO::~TFO() {
    glDeleteTransformFeedbacks(1, &tfo);
    glDeleteBuffers(nrPar, tbos);
    glDeleteBuffers(nrPar, buffers);
    glDeleteVertexArrays(1, &resVAO);

    delete[] statColor;
    delete[] recColors;
    delete[] buffers;
    delete[] bufferSizes;
    delete[] tbos;
}
}  // namespace ara
