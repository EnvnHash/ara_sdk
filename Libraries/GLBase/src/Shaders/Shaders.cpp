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

#include "Shaders.h"

using namespace std;

namespace ara {

Shaders::Shaders(const string &comp_str, bool readFromFile) : m_comp(readFromFile ? textFileRead(comp_str) : comp_str) {
    create();
}

Shaders::Shaders(const string &vert, const string &frag, bool readFromFile) :
    m_vs(readFromFile ? textFileRead(vert) : vert),
    m_fs(readFromFile ? textFileRead(frag) : frag)
{
    create();
}

Shaders::Shaders(const string &vert, const string &geom, const string &frag, bool readFromFile) :
    m_vs(readFromFile ? textFileRead(vert) : vert),
    m_gs(readFromFile ? textFileRead(geom) : geom),
    m_fs(readFromFile ? textFileRead(frag) : frag) {
    create();
}

Shaders::Shaders(const string &vert, const string &tessCtr, const string &tessEv, const string &geom,
                 const string &frag, bool readFromFile) :
    m_vs(readFromFile ? textFileRead(vert) : vert),
    m_cs(readFromFile ? textFileRead(tessCtr) : tessCtr),
    m_es(readFromFile ? textFileRead(tessEv) : tessEv),
    m_gs(readFromFile ? textFileRead(geom) : geom),
    m_fs(readFromFile ? textFileRead(frag) : frag) {
    create();
}

// create program
GLuint Shaders::create() {
    if (m_comp.empty()) {
        m_program = glCreateProgram();
    }

    if (!m_vs.empty()) {
        attachShader(m_program, GL_VERTEX_SHADER, m_vs);
    }

#ifndef __EMSCRIPTEN__
    if (!m_comp.empty()) {
        const GLchar *fullSrc[1] = {m_comp.c_str()};
        m_program                = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1,
                                                          fullSrc);  // with this command GL_PROGRAM_SEPARABLE is set to true

        GLint logLength;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::string log;
            log.reserve(logLength);
            glGetProgramInfoLog(m_program, logLength, nullptr, log.data());
            LOGE << "Shaders: ComputeShader Program didn't compile: " << log;
        }

        glGenProgramPipelines(1, &m_progPipe);
        glBindProgramPipeline(m_progPipe);
        glUseProgramStages(m_progPipe, GL_COMPUTE_SHADER_BIT, m_program);

        // check prog Pipeline
        GLint status;
        glValidateProgramPipeline(m_progPipe);
        glGetProgramPipelineiv(m_progPipe, GL_VALIDATE_STATUS, &status);

        if (status != GL_TRUE) {
            glGetProgramPipelineiv(m_progPipe, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 0) {
                std::string log;
                log.reserve(logLength);
                glGetProgramPipelineInfoLog(m_progPipe, logLength, nullptr, log.data());
                LOGE << "Shader pipeline not valid:\n" << log;
            }
        }

        glBindProgramPipeline(0);
    }

#ifndef ARA_USE_GLES31
    if (!m_cs.empty()) {
        attachShader(m_program, GL_TESS_CONTROL_SHADER, m_cs);
    }
    if (!m_es.empty()) {
        attachShader(m_program, GL_TESS_EVALUATION_SHADER, m_es);
    }
#endif
    if (!m_gs.empty()) {
        attachShader(m_program, GL_GEOMETRY_SHADER, m_gs);
    }
#endif
    if (!m_fs.empty()) {
        attachShader(m_program, GL_FRAGMENT_SHADER, m_fs);
    }

    bLoaded    = true;
    m_identMat = glm::mat4(1.f);

    return m_program;
}

void Shaders::checkStatusLink(GLuint obj) {
    GLint status = GL_FALSE, len = 10;
    std::string log;

    if (glIsProgram(obj)) {
        glGetProgramiv(obj, GL_LINK_STATUS, &status);
    }

    if (status != GL_TRUE) {
        LOGE << "Shaders Error: shader didn´t link!!!";

        if (glIsShader(obj)) {
            glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
            log.resize(len);
            glGetShaderInfoLog(obj, len, nullptr, &log[0]);
            LOGE << log;
        }

        if (glIsProgram(obj)) {
            glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
            log.resize(len);
            glGetProgramInfoLog(obj, len, nullptr, &log[0]);
            LOGE << log;
        }

        LOG << "shader geometry code: log: " + (!m_gs.empty() ? m_gs : "");
    } else {
        bLoaded = true;

        // get uniform locations
        int numUniforms;
        glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &numUniforms);

        GLenum  type;
        GLint   size;
        GLsizei length;
        for (int i = 0; i < numUniforms; i++) {
            std::string name;
            name.resize(m_uniNameBufSize);
            glGetActiveUniform(m_program, static_cast<GLuint>(i), m_uniNameBufSize, &length, &size, &type, &name[0]);
            m_locations[name] = glGetUniformLocation(m_program, &name[0]);
        }
    }
}

void Shaders::checkStatusCompile(GLuint obj) const {
    GLint status = GL_FALSE, len = 10;
    if (glIsShader(obj)) glGetShaderiv(obj, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        LOGE << "shader didn´t compile!!!";

        if (glIsShader(obj)) {
            glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
        }
        if (glIsProgram(obj)) {
            glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
        }

        vector<GLchar> log(len);
        if (glIsShader(obj)) {
            glGetShaderInfoLog(obj, len, nullptr, &log[0]);
        }
        if (glIsProgram(obj)) {
            glGetProgramInfoLog(obj, len, nullptr, &log[0]);
        }

        std::string logStr(log.begin(), log.end());
        LOG << "shader log: \n" << logStr;
        LOG << "shader vertex code: \n" << m_vs.c_str();
        LOG << "shader fragment code: \n" << m_fs.c_str();
    }
}

void Shaders::attachShader(GLuint program, GLenum type, const string &src) const {
    if (debug) {
        LOG << "attach \n";
        switch (type) {
            case GL_COMPUTE_SHADER: LOG << "GL_COMPUTE_SHADER"; break;
            case GL_VERTEX_SHADER: LOG << "GL_VERTEX_SHADER"; break;
#ifndef ARA_USE_GLES31
            case GL_TESS_CONTROL_SHADER: LOG << "GL_TESS_CONTROL_SHADER"; break;
            case GL_TESS_EVALUATION_SHADER: LOG << "GL_TESS_EVALUATION_SHADER"; break;
            case GL_GEOMETRY_SHADER: LOG << "GL_GEOMETRY_SHADER"; break;
#endif
            case GL_FRAGMENT_SHADER: LOG << "GL_FRAGMENT_SHADER"; break;
            default: break;
        }
    }

    GLuint shader = glCreateShader(type);

    const char *cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);

    checkStatusCompile(shader);

    glAttachShader(program, shader);
    glDeleteShader(shader);
}

std::string Shaders::textFileRead(const std::string &filename) {
    if (std::filesystem::exists(filename)) {
        std::ifstream t(filename);
        return {std::istreambuf_iterator<char>(t), std::istreambuf_iterator<char>()};
    } else {
        LOGE << "pd3d::Shaders Error: file " << filename << " not found ";
        return "";
    }
}

void Shaders::getGlVersions() {
    LOG << "OpenGL Version: " << glGetString(GL_VERSION);
    LOG << "Glsl Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
}

inline GLint Shaders::getUniformLocation(const std::string &name) {
    if (m_locations.find(name) == m_locations.end()) {
        m_locations[name] = glGetUniformLocation(m_program, name.c_str());
    }
    return m_locations[name];
}

inline void Shaders::pathToResources() {
#ifdef __APPLE__
    CFBundleRef mainBundle   = CFBundleGetMainBundle();
    CFURLRef    resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char        path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
        LOGE << "error setting path to resources";
    CFRelease(resourcesURL);
    chdir(path);
#endif
}

Shaders::~Shaders() {
    glUseProgram(0);
    if (m_program) {
        glDeleteProgram(m_program);
    }

    if (m_progPipe) {
        glDeleteProgramPipelines(1, &m_progPipe);
    }
}

}  // namespace ara
