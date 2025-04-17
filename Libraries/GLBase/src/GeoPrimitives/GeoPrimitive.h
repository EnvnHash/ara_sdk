//
//  Author Sven Hahne
//

#pragma once

#include <Meshes/Mesh.h>
#include <glb_common/glb_common.h>

namespace ara {

class TFO;
class VAO;

class GeoPrimitive {
public:
    virtual void init() = 0;

#ifndef __EMSCRIPTEN__
    virtual void draw(TFO *_tfo = nullptr);
    virtual void draw(GLenum _type, TFO *_tfo = nullptr, GLenum _recMode = GL_TRIANGLES);
    virtual void draw(GLenum _mode, uint32_t offset, uint32_t count, TFO *_tfo, GLenum _recMode);
    virtual void drawInstanced(int nrInstances, TFO *_tfo, float nrVert);
    virtual void drawInstanced(GLenum _type, int nrInstances, TFO *_tfo, float nrVert = 1.f);
    virtual void drawElements(GLenum _type, TFO *_tfo = nullptr, GLenum _recMode = GL_TRIANGLES);
#else
    virtual void draw();
    virtual void draw(GLenum _type, GLenum _recMode = GL_TRIANGLES);
    virtual void draw(GLenum _mode, uint32_t offset, uint32_t count, GLenum _recMode);
    virtual void drawInstanced(int nrInstances, float nrVert = 1.f);
    virtual void drawInstanced(GLenum _type, int nrInstances, float nrVert = 1.f);
    virtual void drawElements(GLenum _type, GLenum _recMode = GL_TRIANGLES);
#endif
    virtual void drawElements();
    virtual void setColor(float _r, float _g, float _b, float _a);
    virtual void setAlpha(float _a);
    virtual void scale(float scaleX, float scaleY, float scaleZ);
    virtual void rotate(float angle, float rotX, float rotY, float rotZ);
    virtual void translate(float _x, float _y, float _z);
    virtual void invertNormals();
#ifndef __EMSCRIPTEN__
    virtual void *getMapBuffer(CoordType _attrIndex);
    virtual void  unMapBuffer();
#endif
    virtual int getNrVert();

    std::unique_ptr<Mesh> m_mesh;
    std::unique_ptr<VAO>  m_vao;
    GLfloat               m_r      = 0.f;
    GLfloat               m_g      = 0.f;
    GLfloat               m_b      = 0.f;
    GLfloat               m_a      = 0.f;
    const char           *m_format = nullptr;
};
}  // namespace ara
