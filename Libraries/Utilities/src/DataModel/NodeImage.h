//
// Created by sven on 04-03-25.
//

#pragma once

#include "DataModel/Node.h"
#include "DataModel/NodeDataPath.h"

namespace ara::node {

class Image : public Node, public DataPath {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(m_imgPath, m_mipMaps)

    Image();

    void load() override;
    void setWatch(bool val) override;

    const std::string&  imgPath() { return m_imgPath; }
    int                 getMipMap() const { return m_mipMaps; }

protected:
    std::string m_imgPath;
    int         m_mipMaps = 1;
};

}