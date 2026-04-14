//
// Created by sven on 13-04-26.
//

#pragma once

#include <DataModel/Node.h>

class DataBindingTestNode : public ara::Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_boolVal, m_intVal, m_floatVal, m_stringVal, m_pathVal, m_vectorInt, m_vectorFloat, m_vec4, m_ivec4)
    DataBindingTestNode() { setTypeName<DataBindingTestNode>(); }

    bool m_boolVal=true;
    int32_t m_intVal=2;
    float m_floatVal=0.234f;
    std::string m_stringVal{"hello"};
    std::filesystem::path m_pathVal{"file.txt"};

    std::vector<int32_t> m_vectorInt { 1, 2, 3 };
    std::vector<float> m_vectorFloat { 0.1f, 0.2f, 0.3f };
    glm::vec4 m_vec4 { 1.1f, 1.2f, 1.3f, 1.4f };
    glm::ivec4 m_ivec4 { 1, 2, 3, 4 };
};
