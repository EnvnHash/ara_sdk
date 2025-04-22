
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <util_common.h>
#include <string_utils.h>
#include <json/json.hpp>

/// convenience Macro definition for generating a serialization function just by passing member variables as arguments
#define ARA_NODE_ADD_SERIALIZE_FUNCTIONS(...)                           \
void serializeValues(nlohmann::json& j) override {                      \
    Node::serializeValues(j);                                           \
    std::string in_arg_names = std::string(#__VA_ARGS__);               \
    std::vector<std::string> names = ara::split(in_arg_names, ", ");    \
    for (auto &it : names) {                                            \
        it = it.substr(2, it.size());                                   \
    }                                                                   \
    ara::node::serializeSingleValue(j, names.begin(), __VA_ARGS__);     \
}                                                                       \
                                                                        \
void deserializeValues(const nlohmann::json& j) override {              \
    for (auto & it: m_changeCb[cbType::preChange]) {                    \
        it.second();                                                    \
    }                                                                   \
    Node::deserializeValues(j);                                         \
    std::string in_arg_names = std::string(#__VA_ARGS__);               \
    std::vector<std::string> names = ara::split(in_arg_names, ", ");    \
    for (auto &it : names) {                                            \
        it = it.substr(2, it.size());                                   \
    }                                                                   \
    ara::node::deserializeSingleValue(j, names.begin(), __VA_ARGS__);   \
}


#define ARA_NODE_ADD_VIRTUAL_SERIALIZE_FUNCTIONS(...)                   \
virtual void serializeValues(nlohmann::json& j) {                       \
    std::string in_arg_names = std::string(#__VA_ARGS__);               \
    std::vector<std::string> names = ara::split(in_arg_names, ", ");    \
    for (auto &it : names) {                                            \
        it = it.substr(2, it.size());                                   \
    }                                                                   \
    ara::node::serializeSingleValue(j, names.begin(), __VA_ARGS__);     \
}                                                                       \
                                                                        \
virtual void deserializeValues(const nlohmann::json& j) {               \
    for (auto & it: m_changeCb[cbType::preChange]) {                    \
        it.second();                                                    \
    }                                                                   \
    std::string in_arg_names = std::string(#__VA_ARGS__);               \
    std::vector<std::string> names = ara::split(in_arg_names, ", ");    \
    for (auto &it : names) {                                            \
        it = it.substr(2, it.size());                                   \
    }                                                                   \
    ara::node::deserializeSingleValue(j, names.begin(), __VA_ARGS__);   \
}


namespace ara::node {

// Base case that handles when there are no remaining arguments
static void serializeSingleValue(nlohmann::json& j, std::vector<std::string>::iterator name) {}

template <typename T, typename... Args>
static void serializeSingleValue(nlohmann::json& j, std::vector<std::string>::iterator name, T&& arg, Args&&... args) {
    j[*name] = arg;
    serializeSingleValue(j, ++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
}

// Base case that handles when there are no remaining arguments
static void deserializeSingleValue(const nlohmann::json& j, std::vector<std::string>::iterator name) {}

template <typename T, typename... Args>
static void deserializeSingleValue(const nlohmann::json& j, std::vector<std::string>::iterator name, T&& arg, Args&&... args) {
    if (j.contains(*name) && !j[*name].is_null()) {
        arg = j[*name];
    }
    deserializeSingleValue(j, ++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
}

}

