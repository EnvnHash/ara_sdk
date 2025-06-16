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

#include <Asset/AssetImageSource.h>
#include <Asset/AssetManager.h>
#include <Utils/Texture.h>

using namespace std;

namespace ara {

AssetImageSource::AssetImageSource(const std::string& name, GLBase *glbase) : AssetImageBase(name, glbase) {
    m_texture = std::make_unique<Texture>(glbase);
}

void AssetImageSource::onProcess() {
    auto pvsize = splitNodeValue("grid");

    m_gridSize[0] = pvsize.getIntPar(0, 1);
    m_gridSize[1] = pvsize.getIntPar(1, m_gridSize[0]);

    if (m_gridSize[0] <= 0 || m_gridSize[1] <= 0) {
        throw runtime_error("imgsrc: Invalid grid size (" + std::to_string(m_gridSize[0]) + "x" + std::to_string(m_gridSize[1]) + ")");
    }

    m_imgPath = getValue("imgsrc");

    if (m_imgPath.empty()) {
        m_imgPath = getValue("src");
    }

    if (m_imgPath.empty()){
        throw runtime_error("imgsrc: No path");
    }

    m_mipMaps = value<int32_t>("mipmaps", 1);
    m_type = AssetImageBase::Type::simple;
}

void AssetImageSource::loadImg(int mimMapLevel) {
    if (m_assetManager == nullptr) {
        throw runtime_error("Instance is nullptr");
    }

    if (m_imgPath.empty()) {
        throw runtime_error("ImgPath(" + m_imgPath + ") not found");
    }

#ifdef ARA_USE_CMRC
    auto ret = m_assetManager->loadResource(this, m_imgPath);
    if (std::get<size_t>(ret) <= 0) {
        throw runtime_error("Cannot load resource " + m_imgPath);
    }
    m_texture->loadFromMemPtr((void*)std::get<const char*>(ret), std::get<size_t>(ret), GL_TEXTURE_2D, mimMapLevel);
#else
    std::vector<uint8_t> resdata;
    if (m_assetManager->loadResource(this, resdata, m_imgPath) <= 0) {
        throw runtime_error("Cannot load resource " + m_imgPath);
    }
    
    m_texture->keepBitmap(true);
    m_texture->setFileName(m_imgPath);
    m_texture->loadFromMemPtr(&resdata[0], resdata.size(), GL_TEXTURE_2D, mimMapLevel);
#endif

    m_texSize.x = static_cast<int>(m_texture->getWidth());
    m_texSize.y = static_cast<int>(m_texture->getHeight());
}

bool AssetImageSource::onLoad() {
    try {
        loadImg(m_mipMaps);
        return true;
    } catch (std::runtime_error &err) {
        LOGE << err.what() << endl;
        return false;
    }
}

bool AssetImageSource::onResourceChange(bool deleted, const std::string &res_fpath) {
    if (deleted) {
        return false;
    }

    try {
        loadImg(m_mipMaps);
        return true;
    } catch (std::runtime_error &err) {
        LOGE << err.what() << endl;
        return false;
    }
}

bool AssetImageSource::isOK() {
    return m_texture->getWidth() > 0 && m_texture->getHeight() > 0 && m_texture->getBpp() > 0;
}

bool AssetImageSource::isClass(ResNode *snode) {
    return snode->getFlag("imgsrc") != nullptr;
}

Texture* AssetImageSource::getTexture() {
    return m_texture.get();
}

GLuint AssetImageSource::getTexID()  {
    return m_texture->getId();
}

int *AssetImageSource::getSrcPixSize()  {
    return m_texture->getSize();
}

int *AssetImageSource::getSectionSize()  {
    return m_texture->getSize();
}

}