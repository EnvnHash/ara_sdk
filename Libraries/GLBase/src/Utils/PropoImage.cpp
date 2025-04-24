//
//  PropoImage.cpp
//
//  Created by Sven Hahne on 4/5/15.
//

#include "PropoImage.h"

#include <Meshes/Mesh.h>

namespace ara {

PropoImage::PropoImage(GLBase* glbase) : imgTex(std::make_unique<Texture>(glbase)) {
}

PropoImage::PropoImage(const std::string& fileName, int _screenW, int _screenH, float _logoWidth, propoImagePos _pos,
                       float _border)
    : imgWidth(_logoWidth), border(_border), pos(_pos) {
    screenW = float(_screenW);
    screenH = float(_screenH);

    imgTex->loadTexture2D(fileName, 1);

    setupQuad();
}

void PropoImage::setupQuad() {
    imgAspectRatio    = static_cast<float>(imgTex->getHeight()) / static_cast<float>(imgTex->getWidth());
    screenAspectRatio = static_cast<float>(screenH) / static_cast<float>(screenW);

    imgHeight = imgWidth * imgAspectRatio / screenAspectRatio;
    glm::vec2 imgLowerLeftCorner;

    switch (pos) {
        case CENTER:
            imgLowerLeftCorner = glm::vec2(0.f - imgWidth * 0.5f - border, 0.f - imgHeight * 0.5f - border);
            break;
        case UPPER_LEFT: imgLowerLeftCorner = glm::vec2(1.f - border, 1.f - imgHeight - border); break;
        case UPPER_RIGHT: imgLowerLeftCorner = glm::vec2(1.f - imgWidth - border, 1.f - imgHeight - border); break;
        case LOWER_LEFT: imgLowerLeftCorner = glm::vec2(1.f - border, 1.f - border); break;
        case LOWER_RIGHT: imgLowerLeftCorner = glm::vec2(1.f - imgWidth - border, 1.f - border); break;
        default: imgLowerLeftCorner = glm::vec2(1.f - imgWidth - border, 1.f - imgHeight - border); break;
    }

    // position to the right upper corner for fullscreen
    if (imgQuad) {
        imgQuad->scale(imgWidth / oldImgWidth, imgHeight / oldImgHeight, 1.f);
    } else {
        imgQuad = std::make_unique<Quad>(QuadInitParams{imgLowerLeftCorner.x, imgLowerLeftCorner.y, imgWidth, imgHeight,
                                                        glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 0.f});
    }
}

void PropoImage::draw() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, imgTex->getId());
    imgQuad->draw();
}

void PropoImage::setWidth(float _newWidth) {
    oldImgWidth  = imgWidth;
    oldImgHeight = imgHeight;

    imgWidth = _newWidth;
    setupQuad();
}

}  // namespace ara
