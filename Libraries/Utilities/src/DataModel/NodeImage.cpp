//
// Created by sven on 04-03-25.
//

#include "DataModel/NodeImage.h"

using fs = std::filesystem::path;

namespace ara::node {

Image::Image() : Node() {
    ara::Node::setTypeName<Image>();
}

void Image::load() { // called when the image was changed on disk
    for (auto& cb : m_changeCb[cbType::postChange]) {
        cb.second();
    }
}

void Image::setWatch(bool val) {
    if (!m_imgPath.empty()) {
        auto comp_path = (fs(m_dataPath) / fs(m_imgPath)).string();
        checkAndAddWatchPath(comp_path);
        if (val && !m_watchThreadRunning) {
            m_watchThreadRunning = true;
            Node::startWatchThread();
        }
    }
}

}