//
// Created by user on 10.07.2020.
//

#include "Utils/MaskStack.h"

#include <Meshes/Mesh.h>
#include <string_utils.h>

using namespace glm;
using namespace std;
namespace fs = std::filesystem;

namespace ara {

MaskLayer *MaskStack::addLayer() {
    m_layers.push_back(std::make_unique<MaskLayer>());
    checkLayers();
    return m_layers.back().get();
}

MaskLayer *MaskStack::addLayer(maskType mType) {
    MaskLayer *outPtr = nullptr;

    if (m_shCol) {
        m_layers.push_back(make_unique<MaskLayer>(mType, m_glbase));
        outPtr = m_layers.back().get();
    } else {
        m_layers.push_back(make_unique<MaskLayer>(mType, m_glbase));
        outPtr = m_layers.back().get();
    }

    checkLayers();
    return outPtr;
}

MaskLayer *MaskStack::addLayer(const std::filesystem::path &inPath,
                               maskType                     mType /*, uint32_t width, uint32_t height*/) {
    if (m_shCol) {
        // be sure we are on the same context as during initialization
        m_layers.push_back(make_unique<MaskLayer>(inPath, mType, m_glbase));
        checkLayers();
        return m_layers.back().get();
    } else
        return nullptr;
}

void MaskStack::update() {
    for (auto &it : m_layers) it->tesselate();

    checkLayers();
    glFinish();  // when rendered on a different gl context, then displayed,
                 // this is needed on order to avoid tearing effects
    callUptCbs();
}

void MaskStack::clear() {
    m_layers.clear();
    callUptCbs();
}

void MaskStack::removeLayer(size_t idx) {
    if (idx < m_layers.size()) m_layers.erase(m_layers.begin() + (int)idx);

    checkLayers();
    callUptCbs();
}

void MaskStack::checkLayers() {
    m_hasBitmap  = false;
    m_hasPolygon = false;
    uint32_t idx = 0;

    for (auto &it : m_layers) {
        if (it->getType() == maskType::Bitmap && it->isVisible())
            m_hasBitmap = true;
        else if (it->getType() == maskType::Vector && it->isVisible())
            m_hasPolygon = true;

        it->setLayerIdx(idx);
        idx++;
    }
}

void MaskStack::createBitmapLayer(uint8_t *data, int width, int height, int bpp, bool inverted, bool hFlip) {
    auto layer = addLayer(maskType::Bitmap);
    layer->loadBitmapFromMem(data, width, height, bpp, inverted, hFlip);
    layer->setIsValid(true);
    checkLayers();
    update();
}

void MaskStack::serializeToXml(pugi::xml_node &parent) {
    auto node = parent.append_child("Layers");
    node.append_attribute("size").set_value(m_layers.size());

    parent.append_attribute("fc_r").set_value(m_fillColor.r);
    parent.append_attribute("fc_g").set_value(m_fillColor.g);
    parent.append_attribute("fc_b").set_value(m_fillColor.b);
    parent.append_attribute("fc_a").set_value(m_fillColor.a);

    parent.append_attribute("fci_r").set_value(m_fillColorInv.r);
    parent.append_attribute("fci_g").set_value(m_fillColorInv.g);
    parent.append_attribute("fci_b").set_value(m_fillColorInv.b);
    parent.append_attribute("fci_a").set_value(m_fillColorInv.a);

    // check if there are any obsolete-exported bitmaps with the name of this
    // export
    if (!m_stackSaveFileName.empty()) {
        auto sp = filesystem::path(m_stackSaveFileName);
        for (const auto &entry : fs::directory_iterator(sp.parent_path())) {
            if (!entry.is_directory()) {
                auto splt = split(entry.path().stem().string(), "_");

                if (splt.size() >= 2) {
                    std::string fileNameWithoutNr;
                    for (int i = 0; i < (int)splt.size() - 1; i++) {
                        fileNameWithoutNr += splt[i];
                        if (i < (int)splt.size() - 2) fileNameWithoutNr += "_";
                    }

                    if (fileNameWithoutNr == sp.stem().string()) fs::remove(entry.path());
                }
            }
        }
    }

    for (auto &it : m_layers) {
        auto layer = node.append_child("Layer");
        layer.append_attribute("type").set_value((int)it->getType());
        it->serializeToXml(layer);
    }
}

void MaskStack::parseFromXml(pugi::xml_node &node) {
    pugi::xml_attribute attr;
    size_t              nrChildren = 0;

    if ((attr = node.attribute("size"))) nrChildren = (size_t)attr.as_uint();

    if ((attr = node.attribute("fc_r"))) m_fillColor.r = attr.as_float();
    if ((attr = node.attribute("fc_g"))) m_fillColor.g = attr.as_float();
    if ((attr = node.attribute("fc_b"))) m_fillColor.b = attr.as_float();
    if ((attr = node.attribute("fc_a"))) m_fillColor.a = attr.as_float();

    if ((attr = node.attribute("fci_r"))) m_fillColorInv.r = attr.as_float();
    if ((attr = node.attribute("fci_g"))) m_fillColorInv.g = attr.as_float();
    if ((attr = node.attribute("fci_b"))) m_fillColorInv.b = attr.as_float();
    if ((attr = node.attribute("fci_a"))) m_fillColorInv.a = attr.as_float();

    if (m_layers.size() != nrChildren) m_layers.resize(nrChildren);

    // iterate through layers
    if (m_layers.empty()) return;

    auto layerIt = m_layers.begin();
    for (auto xmlLayer = node.begin(); xmlLayer != node.end(); ++xmlLayer, ++layerIt) {
        auto mType = (maskType)0;
        if ((attr = xmlLayer->attribute("type"))) mType = (maskType)attr.as_int();

        if (!(*layerIt)) {
            *layerIt = make_unique<MaskLayer>(mType, m_glbase);
        } else {
            (*layerIt)->setType(mType);
        }

        (*layerIt)->setLoadFileName(m_loadFileName);
        (*layerIt)->parseFromXml(*xmlLayer);
    }

    checkLayers();
    callUptCbs();
}

}  // namespace ara