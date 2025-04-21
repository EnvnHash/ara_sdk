#include "ImgSection.h"

#include "ResInstance.h"

// #include <filesystem>

using namespace std;

namespace ara {

std::array<int, 2> ImageBase::getVerPos(int ver) {
    int vidx = getVer(ver);

    std::array<int, 2> pos{};
    pos[0] = getSectionPos()[0];
    pos[1] = getSectionPos()[1];

    if (m_Dist == Dist::vert) {
        pos[1] += m_DistPixOffset * vidx;
    } else if (m_Dist == Dist::horz) {
        pos[0] += m_DistPixOffset * vidx;
    }

    return pos;
}

bool ImgSrc::OnProcess() {
    auto pvsize = splitNodeValue("grid");

    m_GridSize[0] = pvsize.getIntPar(0, 1);
    m_GridSize[1] = pvsize.getIntPar(1, m_GridSize[0]);

    if (m_GridSize[0] <= 0 || m_GridSize[1] <= 0) {
        return error((char *)"imgsrc: Invalid grid size (%dx%d)", m_GridSize[0], m_GridSize[1]);
    }

    m_ImgPath = getValue("imgsrc");

    if (m_ImgPath.empty()) {
        m_ImgPath = getValue("src");
    }

    if (m_ImgPath.empty()){
        return error((char *)"imgsrc: No path");
    }

    m_MipMaps = value1i("mipmaps", 1);
    m_Type = ImageBase::Type::simple;
    return true;
}

bool ImgSrc::LoadImg(int mimMapLevel) {
    if (m_Instance == nullptr) {
        return error((char *)"Instance is nullptr");
    }

    if (m_ImgPath.empty()) {
        return error((char *)"ImgPath(%s) not found", m_ImgPath.c_str());
    }

    std::vector<uint8_t> resdata;
    if (m_Instance->loadResource(this, resdata, m_ImgPath) <= 0) {
        return error((char *)"Cannot load resource (%s)", m_ImgPath.c_str());
    }

    m_texMan->keepBitmap(true);
    m_texMan->setFileName(m_ImgPath);
    m_texMan->loadFromMemPtr(&resdata[0], resdata.size(), GL_TEXTURE_2D, mimMapLevel);

    m_texSize.x = static_cast<int>(m_texMan->getWidth());
    m_texSize.y = static_cast<int>(m_texMan->getHeight());

    return true;
}

bool ImgSection::OnProcess() {
    ResNode *src = getParent();

    if (!getValue("img").empty()) {
        if ((src = findNodeFromRoot(getValue("img"))) == nullptr) {
            return error((char *)"img: imsrc parent defined in src: not found");
        }

        if (typeid(src[0]) != typeid(ImgSrc)) {
            return error((char *)"img: imsrc parent defined in src: is not of imgsrc type");
        }
    }

    if (getByName("src")) {
        if ((src = findNodeFromRoot(getValue("src"))) == nullptr) {
            return error((char *)"img: imsrc parent defined in src: not found");
        }

        if (typeid(src[0]) != typeid(ImgSrc)) {
            return error((char *)"img: imsrc parent defined in src: is not of imgsrc type");
        }
    }

    if ((m_ImgSrc = dynamic_cast<ImgSrc *>(src)) == nullptr) {
        return error((char *)"imgsrc is null");
    }

    ParVec pv_pos  = splitNodeValue("pos");
    ParVec pv_size = splitNodeValue("size");
    ParVec pv_sep  = splitNodeValue("sep");

    m_Pos[0] = pv_pos.getIntPar(0, 0);
    m_Pos[1] = pv_pos.getIntPar(1, 0);

    if (m_Pos[0] < 0 || m_Pos[1] < 0) {
        return error((char *)"Invalid position (%d,%d), position values should be positive", m_Pos[0], m_Pos[1]);
    }

    m_PixSize[0] = pv_size.getIntPar(0, 0);
    m_PixSize[1] = pv_size.getIntPar(1, m_PixSize[0]);

    if (m_PixSize[0] <= 0 || m_PixSize[1] <= 0) {
        return error((char *)"img: Invalid size (%dx%d)", m_PixSize[0], m_PixSize[1]);
    }

    m_PixSep[0] = pv_sep.getIntPar(0, 0);
    m_PixSep[1] = pv_sep.getIntPar(1, m_PixSep[0]);

    m_PixPos[0] = m_Pos[0] * m_ImgSrc->m_GridSize[0];
    m_PixPos[1] = m_Pos[1] * m_ImgSrc->m_GridSize[1];

    if (hasFlag("frame")) {
        m_Type = ImageBase::Type::frame;
    }

    if (m_Type == ImageBase::Type::none) {
        m_Type = ImageBase::Type::simple;
    }

    if (hasFlag("vdist")) {
        m_Dist          = ImageBase::Dist::vert;
        m_DistPixOffset = value1i("vdist", 0);
    }

    if (hasFlag("hdist")) {
        m_Dist          = ImageBase::Dist::horz;
        m_DistPixOffset = value1i("hdist", 0);
    }

    if (hasFlag("ver")) {
        ParVec pv_ver = splitNodeValue("ver");
        for (int i = 0; i < pv_ver.getParCount(); i++) {
            m_Ver[i] = pv_ver.getIntPar(i, i);
        }
    }

    m_VerDefault = value1i("default", 0);

    return true;
}

}  // namespace ara