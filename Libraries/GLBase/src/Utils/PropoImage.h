//
//  PropoImage.h
//
//  Created by Sven Hahne on 4/5/15.
//

#pragma once

#include <glb_common/glb_common.h>

#include "GeoPrimitives/Quad.h"
#include "Utils/FBO.h"
#include "Utils/Texture.h"

namespace ara {
class PropoImage {
public:
    enum propoImagePos { CENTER = 0, UPPER_LEFT = 1, UPPER_RIGHT = 2, LOWER_LEFT = 3, LOWER_RIGHT = 4 };

    explicit PropoImage(GLBase* glbase);
    PropoImage(const std::string& fileName, int _screenW, int _screenH, float _logoWidth = 0.5f,
               propoImagePos _pos = CENTER, float _border = 0.f);

    void setupQuad();
    void draw() const;
    void setWidth(float _newWidth);

    [[nodiscard]] float getImgHeight() const { return imgHeight; }
    [[nodiscard]] float getImgAspectRatio() const { return imgAspectRatio; }
    [[nodiscard]] GLint getTexId() const { return static_cast<GLint>(imgTex->getId()); }

private:
    std::unique_ptr<Texture> imgTex;
    std::unique_ptr<Quad>    imgQuad;
    propoImagePos            pos{};
    bool                     inited = false;

    float imgWidth{};
    float imgHeight{};
    float oldImgWidth{};
    float oldImgHeight{};
    float screenW{};
    float screenH{};
    float border{};
    float imgAspectRatio{};
    float screenAspectRatio{};
};
}  // namespace ara
