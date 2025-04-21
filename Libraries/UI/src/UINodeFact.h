#pragma once

#include <glsg_common/glsg_common.h>

namespace ara {

class UINode;

class UINodeFact {
public:
    std::unique_ptr<UINode> Create(const std::string &className);
};

}  // namespace ara
