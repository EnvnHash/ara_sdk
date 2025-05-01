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

#include <Asset/AssetImageSection.h>
#include <Asset/AssetManager.h>

#include "AssetImageSource.h"

using namespace std;

namespace ara {

void AssetImageSection::onProcess() {
    parseNode(getParent());

    auto pv_pos  = splitNodeValue("pos");
    auto pv_size = splitNodeValue("size");
    auto pv_sep  = splitNodeValue("sep");

    for (int i = 0; i<2; i++) {
        m_pos[i] = pv_pos.getIntPar(i, 0);
    }

    if (m_pos[0] < 0 || m_pos[1] < 0) {
        throw runtime_error("AssetImageSection Error: Invalid position (" + std::to_string(m_pos[0]) + "," + std::to_string(m_pos[1]) + "), position values should be positive");
    }

    m_pixSize[0] = pv_size.getIntPar(0, 0);
    m_pixSize[1] = pv_size.getIntPar(1, m_pixSize[0]);

    if (m_pixSize[0] <= 0 || m_pixSize[1] <= 0) {
        throw runtime_error("AssetImageSection Error: Invalid size (" + std::to_string(m_pixSize[0]) + "x" + std::to_string(m_pixSize[1]) + ")");
    }

    m_pixSep[0] = pv_sep.getIntPar(0, 0);
    m_pixSep[1] = pv_sep.getIntPar(1, m_pixSep[0]);

    for (int i = 0; i<2; i++) {
        m_pixPos[i] = m_pos[i] * m_imgSrc->m_gridSize[i];
    }

    procFlags();

    m_verDefault = value<int>("default", 0);
}

void AssetImageSection::parseNode(ResNode* src) {
    if (!getValue("img").empty()) {
        if ((src = findNodeFromRoot(getValue("img"))) == nullptr) {
            throw runtime_error("img: imsrc parent defined in src: not found");
        }

        if (typeid(src[0]) != typeid(AssetImageSource)) {
            throw runtime_error("img: imsrc parent defined in src: is not of imgsrc type");
        }
    }

    if (getByName("src")) {
        if ((src = findNodeFromRoot(getValue("src"))) == nullptr) {
            throw runtime_error("img: imsrc parent defined in src: not found");
        }

        if (typeid(src[0]) != typeid(AssetImageSource)) {
            throw runtime_error("img: imsrc parent defined in src: is not of imgsrc type");
        }
    }

    if ((m_imgSrc = dynamic_cast<AssetImageSource *>(src)) == nullptr) {
        throw runtime_error("imgsrc is null");
    }
}

void AssetImageSection::procFlags() {
    if (hasFlag("frame")) {
        m_type = Type::frame;
    }

    if (m_type == Type::none) {
        m_type = Type::simple;
    }

    if (hasFlag("vdist")) {
        m_dist          = Dist::vert;
        m_distPixOffset = value<int32_t>("vdist", 0);
    }

    if (hasFlag("hdist")) {
        m_dist          = Dist::horz;
        m_distPixOffset = value<int32_t>("hdist", 0);
    }

    if (hasFlag("ver")) {
        ParVec pv_ver = splitNodeValue("ver");
        for (int i = 0; i < pv_ver.getParCount(); i++) {
            m_ver[i] = pv_ver.getIntPar(i, i);
        }
    }
}

Texture* AssetImageSection::getTexture() {
    return m_imgSrc != nullptr ? m_imgSrc->getTexture() : nullptr;
}

GLuint AssetImageSection::getTexID() {
    return m_imgSrc != nullptr ? m_imgSrc->getTexID() : 0;
}

int* AssetImageSection::getSrcPixSize() {
    return m_imgSrc ? m_imgSrc->getSrcPixSize() : nullptr;
}


}  // namespace ara