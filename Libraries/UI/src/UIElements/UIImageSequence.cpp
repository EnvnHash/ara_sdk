//
// Created by sven on 25-08-25.
//

#include "UIImageSequence.h"

using namespace std;

namespace ara {

UIImageSequence::UIImageSequence() : ara::Image() {
    setName(ara::getTypeName<UIImageSequence>());
}

void UIImageSequence::init() {
    if (!m_filepath.empty()) {
        m_imgSeq.loadFromAsset(m_filepath, m_glbase);
        setTexId(m_imgSeq.getTexId(0), m_imgSeq.getWidth(), m_imgSeq.getHeight(), m_imgSeq.getBitCount());
    } else {
        LOGE << "UIImageSequence::init failed m_filepath empty";
    }
}

void UIImageSequence::update() {
    auto currFrame = m_imgSeq.update();
    setTexId(m_imgSeq.getTexId(currFrame), m_imgSeq.getWidth(), m_imgSeq.getHeight(), m_imgSeq.getBitCount());
}

}