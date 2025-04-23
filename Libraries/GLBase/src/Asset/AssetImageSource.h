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

#include <Asset/AssetImageBase.h>

namespace ara {

class AssetImageSource : public AssetImageBase {
public:
    explicit AssetImageSource(const std::string& name, GLBase *glbase);
    ~AssetImageSource() override = default;

    void onProcess() override;
    bool onLoad() override ;
    bool onResourceChange(bool deleted, const std::string &res_fpath) override;
    bool isOK() override;
    void loadImg(int mimMapLevel = 1);

    static bool isClass(ResNode *snode);
    Texture*    getTexture() override;
    GLuint      getTexID() override;
    int*        getSrcPixSize() override;
    int*        getSectionSize() override;
    int*        getSectionPos() override { return m_nullIntPtr; }

    std::string              m_imgPath;
    int                      m_gridSize[2] = {1, 1};
    int                      m_mipMaps     = 1;
    std::unique_ptr<Texture> m_texture;
};

}