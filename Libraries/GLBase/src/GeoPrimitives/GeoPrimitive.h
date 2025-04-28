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

#include <Meshes/Mesh.h>
#include <glb_common/glb_common.h>

namespace ara {

class TFO;
class VAO;

class GeoPrimitive {
public:
    virtual void init() = 0;

#ifndef __EMSCRIPTEN__
    virtual void draw(TFO *tfo = nullptr);
    virtual void draw(GLenum type, TFO *tfo = nullptr, GLenum recMode = GL_TRIANGLES);
    virtual void draw(GLenum mode, uint32_t offset, uint32_t count, TFO *tfo, GLenum _recMode);
    virtual void drawInstanced(int nrInstances, TFO *tfo, float nrVert);
    virtual void drawInstanced(GLenum type, int nrInstances, TFO *tfo, float nrVert = 1.f);
    virtual void drawElements(GLenum type, TFO *tfo = nullptr, GLenum recMode = GL_TRIANGLES);
#else
    virtual void draw();
    virtual void draw(GLenum _type, GLenum _recMode = GL_TRIANGLES);
    virtual void draw(GLenum _mode, uint32_t offset, uint32_t count, GLenum _recMode);
    virtual void drawInstanced(int nrInstances, float nrVert = 1.f);
    virtual void drawInstanced(GLenum _type, int nrInstances, float nrVert = 1.f);
    virtual void drawElements(GLenum _type, GLenum _recMode = GL_TRIANGLES);
#endif
    virtual void drawElements();
    virtual void setColor(glm::vec4 col);
    virtual void setAlpha(float a);
    virtual void scale(glm::vec3 scale);
    virtual void rotate(float angle, float rotX, float rotY, float rotZ);
    virtual void translate(glm::vec3 trans);
    virtual void invertNormals();
#ifndef __EMSCRIPTEN__
    virtual void *getMapBuffer(CoordType _attrIndex);
    virtual void  unMapBuffer();
#endif
    virtual int getNrVert();

    std::unique_ptr<Mesh> m_mesh;
    std::unique_ptr<VAO>  m_vao;
    glm::vec4             m_color{};
    const char           *m_format = nullptr;
};
}  // namespace ara
