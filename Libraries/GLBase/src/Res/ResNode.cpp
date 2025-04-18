//
// Created by user on 04.02.2021.
//
#include "Res/ResNode.h"

#include <string_utils.h>

#include "Res/ImgSection.h"
#include "Res/ResColor.h"
#include "Res/ResFont.h"
#include "Res/ResSrcFile.h"

using namespace std;

namespace ara {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SrcLine
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SrcLine::isComment() const {
    const char *e = SrcFile::clearSpaces(str.c_str());
    return (e[0] == '#');
}

ResNode::ResNode(std::string name, SrcLine *line, GLBase *glbase) {
    m_Name       = std::move(name);
    m_glbase     = glbase;
    srcLineIndex = line != nullptr ? line->index : 0;
}

string ResNode::getPath() {
    string            str;
    vector<ResNode *> nl;
    _r_getPath(nl);
    int i, n = (int)nl.size();
    for (i = n - 2; i >= 0; i--) {
        str += nl[i]->m_Name;
        if (i > 0) str += ".";
    }

    return str;
}

bool ResNode::_r_getPath(vector<ResNode *> &nl) {
    nl.push_back(this);
    return !getParent() || getParent()->_r_getPath(nl);
}

bool ResNode::error_string(const string &str) {
    ResNode *root = getRoot();
    root->errList.push_back(
        e_error{srcLineIndex, string(string(typeid(this[0]).name()) + " / (" + m_Name + ") : " + str)});
    return false;
}

bool ResNode::error(char *str, ...) {
    char    estr[1024];
    va_list p;

    va_start(p, str);
    vsprintf(estr, str, p);
    va_end(p);

    return error_string(estr);
}

void ResNode::logtree(int level) {
    char *parent_name = (char *)(getParent() != nullptr ? getParent()->m_Name.c_str() : "");
    if (!m_Value.empty()) {
        // appmsg("%p %sPar> (%s)=(%s) parent=(%s)", this,std::string(level * 4,
        // ' ').c_str(), m_Name.c_str(),m_Value.c_str(),parent_name);
    } else {
        if (!m_Func.empty()) {
            string str;
            for (string &sp : m_Par) str += sp + "|";
            // appmsg("%p %sFunc> %s=%s(%s) parent=(%s)", this,std::string(level
            // * 4, ' ').c_str(),
            // m_Name.c_str(),m_Func.c_str(),str.c_str(),parent_name);
        } else {
            if (isFlag()) {
                // appmsg("%p %sFlag %s parent=(%s)", this,std::string(level *
                // 4, ' ').c_str(), m_Name.c_str(),parent_name);
            } else {
                // appmsg("%p %s%s parent=(%s)", this,std::string(level * 4, '
                // ').c_str(), m_Name.c_str(),parent_name);
            }
        }
    }

    for (Ptr &node : m_Node) {
        node->logtree(level + 1);
    }
}

ResNode *ResNode::add(ResNode::Ptr node) {
    if (!node) return nullptr;
    m_Node.emplace_back(std::move(node));
    ResNode *rnode = m_Node.back().get();
    rnode->setParent(this);
    return rnode;
}

ResNode *ResNode::setParent(ResNode *parent) {
    m_Parent = parent;
    setInstance(parent != nullptr ? parent->getInstance() : nullptr);
    return m_Parent;
}

ResNode *ResNode::getFlag(const string &flagname) {
    for (Ptr &node : m_Node)
        if (node->isFlag(flagname)) return node.get();

    return nullptr;
}

bool ResNode::Process() {
    if (!OnProcess()) {
        return false;
    }

    for (Ptr &node : m_Node) {
        node->Process();
    }

    return true;
}

bool ResNode::Load() {
    m_findNodeCache.clear();

    if (!OnLoad()) {
        return error((char *)"OnLoad()");
    }

    for (Ptr &node : m_Node) {
        if (!node->Load()) {
            return false;
        }
    }

    return true;
}

bool ResNode::grabNode(ResNode *from) {
    m_Value      = from->m_Value;
    m_Func       = from->m_Func;
    m_Par        = from->m_Par;
    srcLineIndex = from->srcLineIndex;
    setInstance(from->getInstance());

    for (Ptr &node : from->m_Node) {
        add(std::move(node));
    }

    return true;
}

ResNode::Ptr ResNode::Choose() {
    if (ResColor::isClass(this)) return make_unique<ResColor>(m_Name, m_glbase);
    if (ImgSrc::isClass(this)) return make_unique<ImgSrc>(m_Name, m_glbase);
    if (ImgSection::isClass(this)) return make_unique<ImgSection>(m_Name, m_glbase);
    if (ResFont::isClass(this)) return make_unique<ResFont>(m_Name, m_glbase);

    return nullptr;
}

ResNode::Ptr ResNode::Preprocess(int level) {
    int      i, n = (int)m_Node.size();
    ResNode *node, *newnode;
    Ptr      pret;

    for (i = 0; i < n; i++) {
        node = m_Node[i].get();

        if ((pret = node->Choose()) != nullptr) {
            newnode = pret.get();
            newnode->setParent(this);
            newnode->grabNode(node);

            m_Node[i] = std::move(pret);  // previous node gets deleted here
        }
    }

    for (Ptr &nd : m_Node) nd->Preprocess(level + 1);

    return nullptr;
}

ResNode *ResNode::findNode(const string &path) {
    // first check for cached results
    auto n = m_findNodeCache.find(path);
    if (n != m_findNodeCache.end()) return n->second;

    vector<string> tok;
    istringstream  f(path);
    string         s;

    while (getline(f, s, '.')) tok.emplace_back(s);

    ResNode *r            = findNode(tok, 0);
    m_findNodeCache[path] = r;

    return r;
}

tuple<ResNode *, unitType, std::string> ResNode::findNumericNode(const string &path) {
    auto ptr = findNode(path);

    // check if result is numeric, if this is not the case, try to resolve it as
    // a reference to another node
    if (ptr) {
        auto v = split(ptr->m_Value, "px");
        if (v.size() > 1)
            return make_tuple(ptr, unitType::Pixels, v[0]);
        else {
            v = split(ptr->m_Value, "%");
            if (v.size() > 1)
                return make_tuple(ptr, unitType::Percent, v[0]);
            else if (is_number(ptr->m_Value))
                return make_tuple(ptr, unitType::Pixels, ptr->m_Value);
            else {
                // try to resolve as node reference
                ptr = getRoot()->findNode(ptr->m_Value);
                if (ptr) {
                    v = split(ptr->m_Value, "px");
                    if (v.size() > 1)
                        return make_tuple(ptr, unitType::Pixels, v[0]);
                    else {
                        v = split(ptr->m_Value, "%");
                        if (v.size() > 1)
                            return make_tuple(ptr, unitType::Percent, v[0]);
                        else if (is_number(ptr->m_Value))
                            return make_tuple(ptr, unitType::Pixels, ptr->m_Value);
                    }
                }
            }
        }
    }

    return make_tuple(nullptr, unitType::Pixels, "");
}

ResNode *ResNode::getByName(const string &name) {
    if (!m_Node.empty())
        for (Ptr &node : m_Node)
            if (node->isName(name)) return node.get();

    return nullptr;
}

ResNode *ResNode::findNode(vector<string> &v, int level) {
    ResNode *r;
    if (level >= (int)v.size()) return nullptr;
    if ((r = getByName(v[level])) == nullptr) return nullptr;
    return level == v.size() - 1 ? r : r->findNode(v, level + 1);
}

ResNode *ResNode::findNodeFromNode(const string &path, ResNode *rnode) {
    if (rnode == nullptr) rnode = getRoot();
    return rnode->findNode(path);
}

// ---------------------------------------------------[ VALUES
// ]-----------------------------------------------------------------

string ResNode::getValue(const string &name, string def) {
    for (Ptr &node : m_Node)
        if (node->isName(name)) return node->m_Value;

    return def;
}

int ResNode::value1i(const string &name, int def) {
    int      v = def;
    ResNode *ptr;

    if ((ptr = getByName(name)) == nullptr) return def;

    try {
        auto vp = split(ptr->m_Value, "px");
        if (vp.size() > 1) {
            v = stoi(vp[0]);
        } else {
            v = stoi(ptr->m_Value);
        }
    } catch (...) {
    }

    return v;
}

float ResNode::value1f(const string &name, float def) {
    float    v = def;
    ResNode *ptr;
    if ((ptr = getByName(name)) == nullptr) return def;
    try {
        auto vp = split(ptr->m_Value, "%");
        if (vp.size() > 1)
            v = stof(vp[0]);
        else
            v = stof(ptr->m_Value);
    } catch (...) {
    }
    return v;
}

bool ResNode::valueiv(std::vector<int> &v, const string &path, int fcount, int def) {
    v.clear();
    ResNode *node = findNode(path);
    if (node == nullptr) return false;
    ParVec tok = node->splitValue();
    fcount     = fcount > 0 ? fcount : tok.getParCount();
    for (int i = 0; i < fcount; i++) v.emplace_back(tok.getIntPar(i, def));
    return true;
}

bool ResNode::valuefv(std::vector<float> &v, const string &path, int fcount, float def) {
    v.clear();
    ResNode *node = findNode(path);
    if (node == nullptr) return false;
    ParVec tok = node->splitValue();
    fcount     = fcount > 0 ? fcount : tok.getParCount();
    for (int i = 0; i < fcount; i++) v.emplace_back(tok.getFloatPar(i, def));
    return true;
}

std::vector<float> ResNode::valuefv(const string &path, int fcount, float def) {
    std::vector<float> v;
    ResNode           *node = findNode(path);
    if (node == nullptr) return std::move(v);
    ParVec tok = node->splitValue();
    fcount     = fcount ? fcount : tok.getParCount();
    for (int i = 0; i < fcount; i++) v.emplace_back(tok.getFloatPar(i, def));
    return std::move(v);
}

bool ResNode::isInPixels(const string &name) {
    ResNode *ptr = getByName(name);
    if (!ptr) return false;
    return split(ptr->m_Value, "px").size() > 1;
}

bool ResNode::isInPercent(const string &name) {
    ResNode *ptr = getByName(name);
    if (!ptr) return false;
    return split(ptr->m_Value, "%").size() > 1;
}

ParVec ResNode::splitValue(char sep) {
    ParVec        tok;
    istringstream f(m_Value);
    string        s;
    while (getline(f, s, sep)) {
        tok.emplace_back(s);
    }
    return tok;
}

ParVec ResNode::splitNodeValue(const string &value_name, char sep) {
    ParVec   tok;
    ResNode *node;
    if ((node = getByName(value_name)) != nullptr) {
        return node->splitValue(sep);
    }
    return tok;
}

bool ResNode::generateReport(std::vector<e_repitem> &ritem, int level) {
    ritem.emplace_back(e_repitem{this, level, getPath()});
    for (Ptr &node : m_Node) node->generateReport(ritem, level + 1);
    return true;
}

bool ResNode::IsEqual(ResNode *unode) {
    if (unode == nullptr) return false;
    if (m_Name != unode->m_Name) return false;
    if (m_Value != unode->m_Value) return false;
    if (m_Func != unode->m_Func) return false;
    if (m_Par.getParCount() != unode->m_Par.getParCount()) return false;
    int i, n = m_Par.getParCount();
    for (i = 0; i < n; i++) {
        if (m_Par[i] != unode->m_Par[i]) {
            return false;
        }
    }
    return true;
}

bool ResNode::Copy(ResNode *unode) {
    if (unode == nullptr) return false;
    m_Name  = unode->m_Name;
    m_Value = unode->m_Value;
    m_Func  = unode->m_Func;
    m_Par.clear();
    for (string &s : unode->m_Par) {
        m_Par.emplace_back(s);
    }
    return true;
}
}  // namespace ara