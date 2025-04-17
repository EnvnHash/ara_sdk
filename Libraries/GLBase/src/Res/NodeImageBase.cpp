//
// Created by sven on 04-03-25.
//

#include "Res/NodeImageBase.h"

namespace ara::node {

ImageBase::ImageBase() : Node() {
    ara::Node::setTypeName<node::ImageBase>();
}

int *ImageBase::getVerPos(int *pos, int ver) {
    int vidx = getVer(ver);

    pos[0] = getSectionPos()[0];
    pos[1] = getSectionPos()[1];

    if (m_Dist == Dist::vert) {
        pos[1] += m_DistPixOffset * vidx;
    } else if (m_Dist == Dist::horz) {
        pos[0] += m_DistPixOffset * vidx;
    }

    return pos;
}

}
