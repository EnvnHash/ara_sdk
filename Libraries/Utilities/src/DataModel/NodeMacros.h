
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
#define ARA_NODE_ADD_SERIALIZE_FUNCTIONS(baseClassName, ...)                                                            \
std::vector<std::string> serializeClassValues(nlohmann::json& j) override {                                             \
    auto bn = std::string(#baseClassName);\
    baseClassName::serializeClassValues(j);                                                                             \
    auto argNames = ara::node::splitAndSerializeClassValues(j, std::string(#__VA_ARGS__), __VA_ARGS__);                 \
    checkClassKeyEntry(m_typeName, argNames);                                                                           \
    return argNames;                                                                                                    \
}                                                                                                                       \
                                                                                                                        \
std::vector<std::string> deserializeClassValues(const nlohmann::json& j) override {                                     \
    callChangeCbs(cbType::preChange);                                                                                   \
    baseClassName::deserializeClassValues(j);                                                                           \
    auto argNames = ara::node::splitAndDeserializeClassValues(j, std::string(#__VA_ARGS__), __VA_ARGS__);               \
    checkClassKeyEntry(m_typeName, argNames);                                                                           \
    return argNames;                                                                                                    \
}                                                                                                                       \

#define ARA_NODE_ADD_VIRTUAL_SERIALIZE_FUNCTIONS(...)                                                                   \
virtual std::vector<std::string> serializeClassValues(nlohmann::json& j) {                                              \
    auto argNames = ara::node::splitAndSerializeClassValues(j, std::string(#__VA_ARGS__), __VA_ARGS__);                 \
    checkClassKeyEntry(m_typeName, argNames);                                                                           \
    return argNames;                                                                                                    \
}                                                                                                                       \
                                                                                                                        \
virtual std::vector<std::string> deserializeClassValues(const nlohmann::json& j) {                                      \
    callChangeCbs(cbType::preChange);                                                                                   \
    auto argNames = ara::node::splitAndDeserializeClassValues(j, std::string(#__VA_ARGS__), __VA_ARGS__);               \
    checkClassKeyEntry(m_typeName, argNames);                                                                           \
    return argNames;                                                                                                    \
}                                                                                                                       \

namespace ara::node {

// Base case that handles when there are no remaining arguments
static void serializeSingleClassValue(nlohmann::json&, std::vector<std::string>::iterator) {}

template <typename T, typename... Args>
static void serializeSingleClassValue(nlohmann::json& j, std::vector<std::string>::iterator name, T&& arg, Args&&... args) {
    j[*name] = arg;
    serializeSingleClassValue(j, ++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
}

// Base case that handles when there are no remaining arguments
static void deserializeSingleClassValue(const nlohmann::json&, std::vector<std::string>::iterator) {}

template <typename T, typename... Args>
static void deserializeSingleClassValue(const nlohmann::json& j, std::vector<std::string>::iterator name, T&& arg, Args&&... args) {
    if (j.contains(*name) && !j[*name].is_null()) {
        arg = j[*name];
    }
    deserializeSingleClassValue(j, ++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
}

static auto splitMacroStringArgs(const std::string& inArgNames) {
    auto names = split(inArgNames, ", ");
    std::ranges::transform(std::as_const(names), names.begin(), [] (auto& it) {
        return it.substr(2, it.size());
    });
    return names;
}

template <typename... Args>
static auto splitAndSerializeClassValues(nlohmann::json& j, const std::string& inArgNames, Args&&... args) {
    auto names = splitMacroStringArgs(inArgNames);
    serializeSingleClassValue(j, names.begin(), std::forward<Args>(args)...);
    return names;
}

template <typename... Args>
static auto splitAndDeserializeClassValues(const nlohmann::json& j, const std::string& inArgNames, Args&&... args) {
    auto names = splitMacroStringArgs(inArgNames);
    deserializeSingleClassValue(j, names.begin(), std::forward<Args>(args)...);
    return names;
}

}

