//
// Created by sven on 11-03-25.
//

#pragma once

#include <DataModel/Node.h>

namespace ara::node {

class DataPath {
public:
    static void setDataPath(const std::string& p) {
        m_dataPath = p;
    }

protected:
    static inline std::string   m_dataPath;
};

}