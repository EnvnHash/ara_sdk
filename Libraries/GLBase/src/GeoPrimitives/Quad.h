//
//  Quad.h
//
//  Created by Sven Hahne last edit on 19.08.17
//
//

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"
#include "Utils/VAO.h"

namespace ara {

class Quad : public GeoPrimitive {
public:
    Quad();

    Quad(float x, float y, float w, float h, glm::vec3 inNormal = glm::vec3(0.f, 0.f, 1.f), float _r = 1.f,
         float _g = 1.f, float _b = 1.f, float _a = 1.f, std::vector<CoordType> *_instAttribs = nullptr,
         int _nrInstances = 1, bool _flipHori = false);

    virtual ~Quad() {}

    void init();

    void drawAsShared() {
        if (!m_vao) return;
        m_vao->enableVertexAttribs();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_vao->disableVertexAttribs();
    }

    std::vector<glm::vec3> *getPositions();
    std::vector<glm::vec3> *getNormals();
    std::vector<glm::vec2> *getTexCoords();

private:
    float                  width;
    float                  height;
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texCoords;

    std::vector<CoordType> *instAttribs;
    int                     maxNrInstances;
};
}  // namespace ara
