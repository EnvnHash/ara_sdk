//
// stride has to be set after initialisation!!!
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {
class VertexAttribute {
public:
    VertexAttribute() = default;

    ~VertexAttribute() = default;

    void enable() {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, size, type, normalized, stride, pointer);

        // glVertexAttribDivisor is never used up to now ???
#ifndef STRICT_WEBGL_1
        glVertexAttribDivisor(location, instDiv);
#endif
    }

    void setType(char _ctype) {
        switch (_ctype) {
            case 'b': type = GL_BYTE; break;
            case 'B': type = GL_UNSIGNED_BYTE; break;
            case 's': type = GL_SHORT; break;
            case 'S': type = GL_UNSIGNED_SHORT; break;
            case 'i': type = GL_INT; break;
            case 'I': type = GL_UNSIGNED_INT; break;
            case 'f': type = GL_FLOAT; break;
            default: type = 0; break;
        }
    }

    void        disable() { glDisableVertexAttribArray(location); }
    void        setName(const std::string& inName) { name = inName; }
    const char *getName() { return name.c_str(); }
    GLsizeiptr  getByteSize() { return size * sizeof(type) * nrConsecLocs; }

    void printInfo() {
        printf(
            "VertexAttribute with name: %s at location %d, size: %d, type: %d, "
            "normalized: %d, stride: %d\n",
            name.c_str(), location, size, type, normalized, stride);
    }

    //----------------------------------------------------------------------------
    GLuint location = 0;  // location of the generic vertex attribute to be modified.
    GLint  size     = 0;  // Number of components per generic vertex attribute.
    // Must be 1, 2, 3, or 4. The initial value is 4.
    GLenum type = GL_FLOAT;  // data type of each component in the array.
    // Symbolic constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
    // GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_FLOAT, or GL_DOUBLE are
    // accepted. The initial value is GL_FLOAT.
    GLboolean normalized = GL_FALSE;  // whether fixed-point data values should
                                      // be normalized (GL_TRUE) or
    // converted directly as fixed-point values (GL_FALSE) when they are
    // accessed.
    GLsizei stride = 0;  // byte offset between consecutive generic vertex attributes.
    //  If stride is 0, the generic vertex attributes are understood to be
    // tightly packed in the array. The initial value is 0.
    GLvoid *pointer = nullptr;     // pointer to the first component of the first
                                   // attribute element in the array.
    GLint     instDiv  = 0;        // attribdivisor, 0 means per vertex, >1 means per instance
    GLboolean isStatic = false;    // defines if there is one value for all
                                   // vertices or if there are values by vertice
    GLuint      nrConsecLocs = 1;  // if the attribute uses more than one location
    std::string shdrName;

private:
    std::string name;  // atribute name
};
}  // namespace ara
