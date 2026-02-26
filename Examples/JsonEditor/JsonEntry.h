//
// Created by sven on 25-02-26.
//

#pragma once
#include "util_common.h"
#include "json/json.hpp"

namespace ara {

template<typename T>
concept JsonEntryType = std::is_same_v<T, nlohmann::json> || std::is_same_v<T, std::string> || std::is_integral_v<T>
    || std::is_floating_point_v<T>;


template <typename T>
requires JsonEntryType<T> || (std::is_array_v<T> && JsonEntryType<std::remove_extent_t<T>>)
class JsonEntry {
public:
    JsonEntry();
};

}
