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

void GeoPrimitive::setColor(float _r, float _g, float _b, float _a) {
    m_r = _r;
    m_g = _g;
    m_b = _b;
    m_a = _a;

    m_vao->setStaticColor(m_r, m_g, m_b, m_a);
}

void GeoPrimitive::setAlpha(float _a) { m_vao->setStaticColor(m_r, m_g, m_b, _a); }

#ifndef __EMSCRIPTEN__

void GeoPrimitive::draw(TFO *_tfo) { m_vao->draw(GL_TRIANGLES, _tfo, GL_TRIANGLES); }

void GeoPrimitive::draw(GLenum _type, TFO *_tfo, GLenum _recMode) { m_vao->draw(_type, _tfo, _recMode); }

void GeoPrimitive::draw(GLenum _mode, uint offset, uint count, TFO *_tfo, GLenum _recMode) {
    m_vao->draw(_mode, offset, count, _tfo, _recMode);
}

void GeoPrimitive::drawInstanced(int nrInstances, TFO *_tfo, float nrVert) {
    m_vao->drawInstanced(GL_TRIANGLES, nrInstances, _tfo, nrVert);
}

void GeoPrimitive::drawInstanced(GLenum _type, int nrInstances, TFO *_tfo, float nrVert) {
    m_vao->drawInstanced(_type, nrInstances, _tfo, nrVert);
}

void GeoPrimitive::drawElements(GLenum _type, TFO *_tfo, GLenum _recMode) {
    m_vao->drawElements(_type, _tfo, _recMode);
}

#else
void GeoPrimitive::draw() { m_vao->draw(GL_TRIANGLES); }

void GeoPrimitive::draw(GLenum _type, GLenum _recMode) { m_vao->draw(_type); }

void GeoPrimitive::draw(GLenum _mode, uint offset, uint count, GLenum _recMode) {
    m_vao->draw(_mode, offset, count, _recMode);
}

void GeoPrimitive::drawInstanced(int nrInstances, float nrVert) {
    m_vao->drawInstanced(GL_TRIANGLES, nrInstances, nrVert);
}

void GeoPrimitive::drawInstanced(GLenum _type, int nrInstances, float nrVert) {
    m_vao->drawInstanced(_type, nrInstances, nrVert);
}

void GeoPrimitive::drawElements(GLenum _type, GLenum _recMode) { m_vao->drawElements(_type, _recMode); }
#endif

void GeoPrimitive::drawElements() {
    if (m_vao) m_vao->drawElements(GL_TRIANGLES);
}

void GeoPrimitive::scale(float scaleX, float scaleY, float scaleZ) {
    if (m_mesh && m_vao) {
        m_mesh->scale(scaleX, scaleY, scaleZ);
        m_vao->uploadMesh(m_mesh.get());
    }
}

void GeoPrimitive::rotate(float angle, float rotX, float rotY, float rotZ) {
    if (m_mesh && m_vao) {
        m_mesh->rotate(angle, rotX, rotY, rotZ);
        m_vao->uploadMesh(m_mesh.get());
    }
}

void GeoPrimitive::translate(float _x, float _y, float _z) {
    if (m_mesh && m_vao) {
        m_mesh->translate(_x, _y, _z);
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

void *GeoPrimitive::getMapBuffer(CoordType _attrIndex) { return (m_vao) ? m_vao->getMapBuffer(_attrIndex) : nullptr; }

void GeoPrimitive::unMapBuffer() {
    if (m_vao) VAO::unMapBuffer();
}

#endif

int GeoPrimitive::getNrVert() { return m_vao ? m_vao->getNrVertices() : 0; }
}  // namespace ara
