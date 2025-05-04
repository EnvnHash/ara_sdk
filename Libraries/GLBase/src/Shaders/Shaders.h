#pragma once

#include "GlbCommon/GlbCommon.h"

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

// #define USE_SHADER_STATE_CACHING

namespace ara {

class Shaders {
public:
    Shaders() = default;
    Shaders(const std::string &comp, bool readFromFile);
    Shaders(const std::string &vert, const std::string &frag, bool readFromFile);
    Shaders(const std::string &vert, const std::string &geom, const std::string &frag, bool readFromFile);
    Shaders(const std::string &vert, const std::string &tessCtr, const std::string &tessEv, const std::string &geom,
            const std::string &frag, bool readFromFile = false);

    virtual ~Shaders();

    void addVertSrc(const std::string &vert, bool readFromFile = false) {
        pathToResources();
        m_vs = readFromFile ? textFileRead(vert) : vert;
    }

    void addContrSrc(const std::string &contr, bool readFromFile = false) {
        pathToResources();
        m_cs = readFromFile ? textFileRead(contr) : contr;
    }

    void addEvalSrc(const std::string &eval, bool readFromFile = false) {
        pathToResources();
        m_es = readFromFile ? textFileRead(eval) : eval;
    }

    void addGeomSrc(const std::string &geom, bool readFromFile = false) {
        pathToResources();
        m_gs = readFromFile ? textFileRead(geom) : geom;
    }

    void addFragSrc(const std::string &frag, bool readFromFile = false) {
        pathToResources();
        m_fs = readFromFile ? textFileRead(frag) : frag;
    }

    GLuint      create();
    void        begin() const { glUseProgram(m_program); }
    static void end() { glUseProgram(0); }

    void link() {
        glLinkProgram(m_program);
        checkStatusLink(m_program);
    }

    void        bindProgramPipeline() const { glBindProgramPipeline(m_progPipe); }
    static void unbindProgramPipeline() { glBindProgramPipeline(0); }
    GLuint     &getProgram() { return m_program; }

    // GLES 2.0
    void bindAttribLocation(unsigned int pos, const std::string &name) {
#ifdef USE_WEBGL
        glBindAttribLocation(program, pos, name.c_str());
#endif
    }

    GLint getUniformLocation(const std::string &name);

#ifdef USE_SHADER_STATE_CACHING
    template <typename T>
    bool cacheExits(GLint loc) {
        if (typeid(T) == typeid(int))
            return m_intValCache.find(loc) != m_intValCache.end();
        else if (typeid(T) == typeid(unsigned int))
            return m_uintValCache.find(loc) != m_uintValCache.end();
        else if (typeid(T) == typeid(float))
            return m_fValCache.find(loc) != m_fValCache.end();
        else
            return false;
    }

    template <typename T>
    void createCache(GLint loc, std::list<T> &&args) {
        if (typeid(T) == typeid(int)) {
            m_intValCache[loc] = std::list<int>(args.size());
            std::copy(args.begin(), args.end(), m_intValCache[loc].begin());
        } else if (typeid(T) == typeid(unsigned int)) {
            m_uintValCache[loc] = std::list<unsigned int>(args.size());
            std::copy(args.begin(), args.end(), m_uintValCache[loc].begin());
        } else if (typeid(T) == typeid(float)) {
            m_fValCache[loc] = std::list<float>(args.size());
            std::copy(args.begin(), args.end(), m_fValCache[loc].begin());
        }
    }

    template <typename T>
    std::list<T> *getCache(GLint loc) {
        if (typeid(T) == typeid(int))
            return reinterpret_cast<std::list<T> *>(&m_intValCache[loc]);
        else if (typeid(T) == typeid(unsigned int))
            return reinterpret_cast<std::list<T> *>(&m_uintValCache[loc]);
        else if (typeid(T) == typeid(float))
            return reinterpret_cast<std::list<T> *>(&m_fValCache[loc]);
        else
            return nullptr;
    }

    template <typename T>
    bool checkState(GLint loc, std::list<T> &&args) {
        m_watch.setStart();

        m_needsUpdate = false;

        if (!cacheExits<T>(loc)) {
            createCache<T>(loc, std::move(args));
            m_needsUpdate = true;
        } else {
            m_needsUpdate = *getCache<T>(loc) == args;
            /*
            if (auto c = getCache<T>(loc)) {
                auto cIt = c->begin();
                for (auto it = args.begin(); it != args.end(); ++it, ++cIt)
                    if (*it != *cIt) {
                        *cIt = *it;
                        m_needsUpdate = true;
                        break;
                    }
            }*/
        }

        m_watch.setEnd();
        m_watch.print("checkState ");

        return bLoaded && m_needsUpdate;
    }
#endif

    int getLoc(const std::string &name) {
        if (m_locations.empty() || !m_locations.contains(name))
            m_locations[name] = glGetUniformLocation(m_program, name.c_str());
        return m_locations[name];
    }

    bool checkStateV(const std::string &name, void *v) const { return bLoaded; }

#ifdef USE_SHADER_STATE_CACHING
    void setUniform1ui(const std::string &name, unsigned int v1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<unsigned int>(m_loc, {v1})) glUniform1ui(m_loc, v1);
    }
    void setUniform2ui(const std::string &name, unsigned int v1, unsigned int v2) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<unsigned int>(m_loc, {v1, v2})) glUniform2ui(m_loc, v1, v2);
    }
    void setUniform3ui(const std::string &name, unsigned int v1, unsigned int v2, unsigned int v3) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<unsigned int>(m_loc, {v1, v2, v3})) glUniform3ui(m_loc, v1, v2, v3);
    }
    void setUniform4ui(const std::string &name, unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<unsigned int>(m_loc, {v1, v2, v3, v4})) glUniform4ui(m_loc, v1, v2, v3, v4);
    }

    void setUniform1i(const std::string &name, int v1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<int>(m_loc, {v1})) glUniform1i(m_loc, v1);
    }
    void setUniform2i(const std::string &name, int v1, int v2) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<int>(m_loc, {v1, v2})) glUniform2i(m_loc, v1, v2);
    }
    void setUniform3i(const std::string &name, int v1, int v2, int v3) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<int>(m_loc, {v1, v2, v3})) glUniform3i(m_loc, v1, v2, v3);
    }
    void setUniform4i(const std::string &name, int v1, int v2, int v3, int v4) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<int>(m_loc, {v1, v2, v3, v4})) glUniform4i(m_loc, v1, v2, v3, v4);
    }

    void setUniform1f(const std::string &name, float v1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<float>(m_loc, {v1})) glUniform1f(m_loc, v1);
    }
    void setUniform2f(const std::string &name, float v1, float v2) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<float>(m_loc, {v1, v2})) glUniform2f(m_loc, v1, v2);
    }
    void setUniform3f(const std::string &name, float v1, float v2, float v3) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<float>(m_loc, {v1, v2, v3})) glUniform3f(m_loc, v1, v2, v3);
    }
    void setUniform4f(const std::string &name, float v1, float v2, float v3, float v4) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkState<float>(m_loc, {v1, v2, v3, v4})) glUniform4f(m_loc, v1, v2, v3, v4);
    }
#else

    void setUniform1ui(const std::string &name, unsigned int v1) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform1ui(m_loc, v1);
    }

    void setUniform2ui(const std::string &name, unsigned int v1, unsigned int v2) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform2ui(m_loc, v1, v2);
    }

    void setUniform3ui(const std::string &name, unsigned int v1, unsigned int v2, unsigned int v3) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform3ui(m_loc, v1, v2, v3);
    }

    void setUniform4ui(const std::string &name, unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform4ui(m_loc, v1, v2, v3, v4);
    }

    void setUniform1i(const std::string &name, int v1) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform1i(m_loc, v1);
    }

    void setUniform2i(const std::string &name, int v1, int v2) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform2i(m_loc, v1, v2);
    }

    void setUniform3i(const std::string &name, int v1, int v2, int v3) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform3i(m_loc, v1, v2, v3);
    }

    void setUniform4i(const std::string &name, int v1, int v2, int v3, int v4) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform4i(m_loc, v1, v2, v3, v4);
    }

    void setUniform1f(const std::string &name, float v1) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform1f(m_loc, v1);
    }

    void setUniform2f(const std::string &name, float v1, float v2) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform2f(m_loc, v1, v2);
    }

    void setUniform3f(const std::string &name, float v1, float v2, float v3) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform3f(m_loc, v1, v2, v3);
    }

    void setUniform4f(const std::string &name, float v1, float v2, float v3, float v4) {
        m_loc = getLoc(name);
        if (m_loc != -1) glUniform4f(m_loc, v1, v2, v3, v4);
    }

#endif

    // set an array of uniform values
    void setUniform1iv(const std::string &name, int *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform1iv(m_loc, count, v);
        }
    }

    void setUniform2iv(const std::string &name, int *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform2iv(m_loc, count, v);
        }
    }

    void setUniform3iv(const std::string &name, int *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform3iv(m_loc, count, v);
        }
    }

    void setUniform4iv(const std::string &name, int *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform4iv(m_loc, count, v);
        }
    }

    void setUniform1fv(const std::string &name, float *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform1fv(m_loc, count, v);
        }
    }

    void setUniform2fv(const std::string &name, float *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform2fv(m_loc, count, v);
        }
    }

    void setUniform3fv(const std::string &name, float *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform3fv(m_loc, count, v);
        }
    }

    void setUniform4fv(const std::string &name, float *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniform4fv(m_loc, count, v);
        }
    }

    void setUniformMatrix3fv(const std::string &name, float *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniformMatrix3fv(m_loc, count, GL_FALSE, v);
        }
    }

    void setUniformMatrix4fv(const std::string &name, float *v, int count = 1) {
        m_loc = getLoc(name);
        if (m_loc != -1 && checkStateV(name, v)) {
            glUniformMatrix4fv(m_loc, count, GL_FALSE, v);
        }
    }

    void setIdentMatrix4fv(const std::string &name) { setUniformMatrix4fv(name, &m_identMat[0][0]); }

    bool debug = false;

private:
    void               attachShader(GLuint program, GLenum type, const std::string &src) const;
    void               checkStatusLink(GLuint obj);
    void               checkStatusCompile(GLuint obj) const;
    static std::string textFileRead(const std::string &filename);
    static void        getGlVersions();
    void               pathToResources();

    GLuint      m_program  = 0;
    GLuint      m_progPipe = 0;
    std::string m_comp;
    std::string m_vs;
    std::string m_cs;
    std::string m_es;
    std::string m_gs;
    std::string m_fs;
    bool        bLoaded       = false;
    bool        m_needsUpdate = false;
    glm::mat4   m_identMat    = glm::mat4(1.f);

    static constexpr GLsizei               m_uniNameBufSize = 128;
    GLint                                  m_loc            = -1;
    std::unordered_map<std::string, GLint> m_locations;

#ifdef USE_SHADER_STATE_CACHING
    std::unordered_map<GLint, std::list<int>>          m_intValCache;
    std::unordered_map<GLint, std::list<unsigned int>> m_uintValCache;
    std::unordered_map<GLint, std::list<float>>        m_fValCache;
#endif
};

}  // namespace ara
