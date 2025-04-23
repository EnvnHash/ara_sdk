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

#include <Utils/Texture.h>
#include <Asset/AssetImageBase.h>

namespace ara {

class Texture;
class AssetImageSource;

class AssetImageSection : public AssetImageBase {
public:

    explicit AssetImageSection(const std::string& name, GLBase *glbase) : AssetImageBase(name, glbase) {}
    void onProcess() override;
    void parseNode(ResNode* src);
    void procFlags();

    static bool         isClass(ResNode *snode) { return snode->getFlag("img") != nullptr; }
    bool                isOK() override { return m_imgSrc != nullptr; }
    bool                onSourceResUpdate(bool deleted, ResNode *unode) override { return true; };
    Texture*            getTexture() override;
    GLuint              getTexID() override;
    int                 *getSrcPixSize() override;
    int                 *getSectionSize() override { return m_pixSize; };
    int                 *getSectionPos() override { return m_pixPos; }

    AssetImageSource *m_imgSrc = nullptr;

    int m_pos[2]     = {0, 0};  // position of the element in GRID units from imgsrc
    int m_pixPos[2]  = {0, 0};  // m_Pos * m_imgSrc->m_GridSize
    int m_pixSize[2] = {0, 0};  // size of each component, in pixels
};

}  // namespace ara
