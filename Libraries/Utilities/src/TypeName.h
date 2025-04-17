// Created by user on 18.03.2021.
//

#pragma once

#include <nameof.hpp>

namespace ara {

template <class T>
std::string getTypeName() {
    return std::string(NAMEOF_SHORT_TYPE(T));
}

}  // namespace ara