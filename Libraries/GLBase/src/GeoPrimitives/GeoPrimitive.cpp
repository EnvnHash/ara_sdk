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

#include "GeoPrimitives/GeoPrimitive.h"

#include <Meshes/Mesh.h>

#include "Utils/TFO.h"
#include "Utils/VAO.h"

namespace ara {

void GeoPrimitive::setColor(glm::vec4 color) {
    m_color = color;
    m_vao->setStaticColor(color);
}

void GeoPrimitive::setAlpha(float a) {
    m_vao->setStaticColor({m_color.r, m_color.g, m_color.b, a});
}

#ifndef __EMSCRIPTEN__

void GeoPrimitive::draw(TFO *tfo) {
    m_vao->draw(GL_TRIANGLES, tfo, GL_TRIANGLES);
}

void GeoPrimitive::draw(GLenum type, TFO *tfo, GLenum recMode) {
    m_vao->draw(type, tfo, recMode);
}

void GeoPrimitive::draw(GLenum mode, uint offset, uint count, TFO *tfo, GLenum recMode) {
    m_vao->draw(mode, offset, count, tfo, recMode);
}

void GeoPrimitive::drawInstanced(int nrInstances, TFO *tfo, float nrVert) {
    m_vao->drawInstanced(GL_TRIANGLES, nrInstances, tfo, nrVert);
}

void GeoPrimitive::drawInstanced(GLenum type, int nrInstances, TFO *tfo, float nrVert) {
    m_vao->drawInstanced(type, nrInstances, tfo, nrVert);
}

void GeoPrimitive::drawElements(GLenum type, TFO *tfo, GLenum recMode) {
    m_vao->drawElements(type, tfo, recMode);
}
#else
void GeoPrimitive::draw() {
    m_vao->draw(GL_TRIANGLES);
}

void GeoPrimitive::draw(GLenum type, GLenum recMode) {
    m_vao->draw(type);
}

void GeoPrimitive::draw(GLenum mode, uint offset, uint count, GLenum recMode) {
    m_vao->draw(mode, offset, count, recMode);
}

void GeoPrimitive::drawInstanced(int nrInstances, float nrVert) {
    m_vao->drawInstanced(GL_TRIANGLES, nrInstances, nrVert);
}

void GeoPrimitive::drawInstanced(GLenum type, int nrInstances, float nrVert) {
    m_vao->drawInstanced(type, nrInstances, nrVert);
}

void GeoPrimitive::drawElements(GLenum type, GLenum recMode) {
    m_vao->drawElements(type, recMode);
}
#endif

void GeoPrimitive::drawElements() {
    if (m_vao) m_vao->drawElements(GL_TRIANGLES);
}

void GeoPrimitive::scale(glm::vec3 scale) {
    if (m_mesh && m_vao) {
        m_mesh->scale(scale);
        m_vao->uploadMesh(m_mesh.get());
    }
}

void GeoPrimitive::rotate(float angle, float rotX, float rotY, float rotZ) {
    if (m_mesh && m_vao) {
        m_mesh->rotate(angle, rotX, rotY, rotZ);
        m_vao->uploadMesh(m_mesh.get());
    }
}

void GeoPrimitive::translate(glm::vec3 trans) {
    if (m_mesh && m_vao) {
        m_mesh->translate(trans);
        m_vao->uploadMesh(m_mesh.get());
    }
}

void GeoPrimitive::invertNormals() {
    if (m_mesh && m_vao) {
        m_mesh->invertNormals();
        m_vao->uploadMesh(m_mesh.get());
    }
}

#ifndef __EMSCRIPTEN__
void *GeoPrimitive::getMapBuffer(CoordType _attrIndex) {
    return m_vao ?  m_vao->getMapBuffer(_attrIndex) : nullptr;
}

void GeoPrimitive::unMapBuffer() {
    if (m_vao) {
        VAO::unMapBuffer();
    }
}
#endif

int GeoPrimitive::getNrVert() { return m_vao ? m_vao->getNrVertices() : 0; }
}  // namespace ara
