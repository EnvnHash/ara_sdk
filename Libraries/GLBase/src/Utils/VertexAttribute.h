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

#include <GlbCommon/GlbCommon.h>

namespace ara {
struct VertexAttribute {
    void enable() const {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, static_cast<GLint>(size), type, normalized, stride, pointer);
#ifndef STRICT_WEBGL_1
        glVertexAttribDivisor(location, instDiv);
#endif
    }

    void setType(char ctype) {
        type = typeMap.contains(ctype) ? typeMap.at(ctype) : GL_FLOAT;
    }

    void            disable() const { glDisableVertexAttribArray(location); }
    void            setName(const std::string& inName) { name = inName; }
    std::string&    getName() { return name; }

    [[nodiscard]] GLsizeiptr getByteSize() const { return static_cast<GLsizeiptr>(size * sizeof(type) * nrConsecLocs ); }

    void printInfo() {
        LOGE << "VertexAttribute with name: " << name << " at location " << location <<", size: " << size << ", type: " << type << ", normalized: " << normalized << ", stride: " << stride;
    }

    GLsizei location = 0;               // location of the generic vertex attribute to be modified.
    GLsizeiptr size = 0;                // Number of components per generic vertex attribute.
                                        // Must be 1, 2, 3, or 4. The initial value is 4.
    GLenum type = GL_FLOAT;             // data type of each component in the array.
                                        // Symbolic constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT,
                                        // GL_INT, GL_UNSIGNED_INT, GL_FLOAT, or GL_DOUBLE are  accepted. The initial
                                        // value is GL_FLOAT.
    GLboolean normalized = GL_FALSE;    // whether fixed-point data values should be normalized (GL_TRUE) or converted
                                        // directly as fixed-point values (GL_FALSE) when they are accessed.
    GLsizei stride = 0;                 // byte offset between consecutive generic vertex attributes.
                                        // If stride is 0, the generic vertex attributes are understood to be
                                        // tightly packed in the array. The initial value is 0.
    GLvoid * pointer = nullptr;         // pointer to the first component of the first attribute element in the array.
    GLint     instDiv  = 0;             // attribdivisor, 0 means per vertex, >1 means per instance
    GLboolean isStatic = false;         // defines if there is one value for all vertices or if there are values by vertices
    GLuint      nrConsecLocs = 1;       // if the attribute uses more than one location
    std::string shdrName;
    std::string name;                   // attribute name

    static inline const std::unordered_map<char, GLenum> typeMap = {
        {'b', GL_BYTE},
        {'B', GL_UNSIGNED_BYTE},
        {'s', GL_SHORT},
        {'S', GL_UNSIGNED_SHORT},
        {'i', GL_INT},
        {'I', GL_UNSIGNED_INT},
        {'f', GL_FLOAT}
    };
};

}  // namespace ara
