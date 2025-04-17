//
// Created by sven on 05-03-25.
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

