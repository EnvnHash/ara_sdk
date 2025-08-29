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


#include "PropoImage.h"

#include <Meshes/Mesh.h>

namespace ara {

PropoImage::PropoImage(GLBase* glbase) : m_imgTex(std::make_unique<Texture>(glbase)) {
}

PropoImage::PropoImage(GLBase* glbase, const std::string& fileName, int screenW, int screenH, float logoWidth, propoImagePos pos,
                       float border)
    : m_pos(pos), m_imgWidth(logoWidth), m_border(border) {
    m_screenW = static_cast<float>(screenW);
    m_screenH = static_cast<float>(screenH);

    m_imgTex = std::make_unique<Texture>(glbase);
    m_imgTex->loadTexture2D(fileName, 1);

    setupQuad();
}

void PropoImage::setupQuad() {
    m_imgAspectRatio    = static_cast<float>(m_imgTex->getHeight()) / static_cast<float>(m_imgTex->getWidth());
    m_screenAspectRatio = m_screenH / m_screenW;

    m_imgHeight = m_imgWidth * m_imgAspectRatio / m_screenAspectRatio;
    glm::vec2 imgLowerLeftCorner;

    switch (m_pos) {
        case CENTER:
            imgLowerLeftCorner = glm::vec2(0.f - m_imgWidth * 0.5f - m_border, 0.f - m_imgHeight * 0.5f - m_border);
            break;
        case UPPER_LEFT: imgLowerLeftCorner = glm::vec2(1.f - m_border, 1.f - m_imgHeight - m_border); break;
        case UPPER_RIGHT: imgLowerLeftCorner = glm::vec2(1.f - m_imgWidth - m_border, 1.f - m_imgHeight - m_border); break;
        case LOWER_LEFT: imgLowerLeftCorner = glm::vec2(1.f - m_border, 1.f - m_border); break;
        case LOWER_RIGHT: imgLowerLeftCorner = glm::vec2(1.f - m_imgWidth - m_border, 1.f - m_border); break;
        default: imgLowerLeftCorner = glm::vec2(1.f - m_imgWidth - m_border, 1.f - m_imgHeight - m_border); break;
    }

    // position to the right upper corner for fullscreen
    if (m_imgQuad) {
        m_imgQuad->scale({m_imgWidth / m_oldImgWidth, m_imgHeight / m_oldImgHeight, 1.f});
    } else {
        m_imgQuad = std::make_unique<Quad>(QuadInitParams{.pos = imgLowerLeftCorner,
                                                        .size = {m_imgWidth, m_imgHeight},
                                                        .color = {0.f, 0.f, 0.f, 0.f} });
    }
}

void PropoImage::draw() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_imgTex->getId());
    m_imgQuad->draw();
}

void PropoImage::setWidth(float newWidth) {
    m_oldImgWidth     = m_imgWidth;
    m_oldImgHeight    = m_imgHeight;
    m_imgWidth        = newWidth;
    setupQuad();
}

}  // namespace ara
