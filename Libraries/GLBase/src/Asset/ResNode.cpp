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

#include <Asset/ResNode.h>
#include <Asset/AssetImageSource.h>
#include <Asset/AssetImageSection.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetFont.h>
#include <Asset/ResSrcFile.h>

using namespace std;

namespace ara {

bool SrcLine::isEmpty() {
    return !(std::find_if(str.begin(), str.end(), [&](auto &x) { return x > 32; }) != str.end());
}

bool SrcLine::isComment() const {
    const char *e = SrcFile::clearSpaces(str.c_str());
    return (e[0] == '#');
}

ResNode::ResNode(std::string name, SrcLine *line, GLBase *glbase) {
    m_name       = std::move(name);
    m_glbase     = glbase;
    srcLineIndex = line != nullptr ? line->index : 0;
}

string ResNode::getPath() {
    string            str;
    vector<ResNode *> nl;
    _r_getPath(nl);
    int i, n = static_cast<int>(nl.size());
    for (i = n - 2; i >= 0; i--) {
        str += nl[i]->m_name;
        if (i > 0) {
            str += ".";
        }
    }

    return str;
}

bool ResNode::_r_getPath(vector<ResNode *> &nl) {
    nl.push_back(this);
    return !getParent() || getParent()->_r_getPath(nl);
}

/*
bool ResNode::error_string(const string &str) {
    ResNode *root = getRoot();
    root->errList.push_back(e_error{srcLineIndex, string(string(typeid(this[0]).name()) + " / (" + m_name + ") : " + str)});
    return false;
}*/

void ResNode::logtree(int level) {
    if (!m_value.empty()) {
    } else {
        if (!m_func.empty()) {
            string str;
            for (string &sp : m_par) {
                str += sp + "|";
            }
        }
    }

    for (Ptr &node : m_node) {
        node->logtree(level + 1);
    }
}

ResNode *ResNode::add(ResNode::Ptr node) {
    if (!node) {
        return nullptr;
    }
    m_node.emplace_back(std::move(node));
    auto rnode = m_node.back().get();
    rnode->setParent(this);
    return rnode;
}

ResNode *ResNode::setParent(ResNode *parent) {
    m_parent = parent;
    setAssetManager(parent != nullptr ? parent->getAssetManager() : nullptr);
    return m_parent;
}

ResNode *ResNode::getFlag(const string &flagname) {
    for (Ptr &node : m_node) {
        if (node->isFlag(flagname)) {
            return node.get();
        }
    }
    return nullptr;
}

void ResNode::process() {
    onProcess();
    for (Ptr &node : m_node) {
        node->process();
    }
}

bool ResNode::load() {
    m_findNodeCache.clear();

    try {
        onLoad();
        for (Ptr &node : m_node) {
            node->load();
        }
        return true;
    } catch (std::runtime_error &err) {
        LOGE << err.what() << endl;
        return false;
    }
}

bool ResNode::grabNode(ResNode *from) {
    m_value      = from->m_value;
    m_func       = from->m_func;
    m_par        = from->m_par;
    srcLineIndex = from->srcLineIndex;
    setAssetManager(from->getAssetManager());

    for (Ptr &node : from->m_node) {
        add(std::move(node));
    }

    return true;
}

ResNode::Ptr ResNode::choose() {
    if (AssetColor::isClass(this)) {
        return make_unique<AssetColor>(m_name, m_glbase);
    }

    if (AssetImageSource::isClass(this)) {
        return make_unique<AssetImageSource>(m_name, m_glbase);
    }

    if (AssetImageSection::isClass(this)) {
        return make_unique<AssetImageSection>(m_name, m_glbase);
    }

    if (AssetFont::isClass(this)) {
        return make_unique<AssetFont>(m_name, m_glbase);
    }

    return nullptr;
}

ResNode::Ptr ResNode::preprocess(int level) {
    int      i, n = static_cast<int>(m_node.size());
    Ptr      pret;

    for (i = 0; i < n; i++) {
        auto node = m_node[i].get();

        if ((pret = node->choose()) != nullptr) {
            auto newnode = pret.get();
            newnode->setParent(this);
            newnode->grabNode(node);

            m_node[i] = std::move(pret);  // previous node gets deleted here
        }
    }

    for (Ptr &nd : m_node) {
        nd->preprocess(level + 1);
    }

    return nullptr;
}

ResNode *ResNode::findNode(const string &path) {
    // first check for cached results
    auto n = m_findNodeCache.find(path);
    if (n != m_findNodeCache.end()) {
        return n->second;
    }

    vector<string> tok;
    istringstream  f(path);
    string         s;

    while (getline(f, s, '.')) {
        tok.emplace_back(s);
    }

    ResNode *r            = findNode(tok, 0);
    m_findNodeCache[path] = r;

    return r;
}

tuple<ResNode *, unitType, std::string> ResNode::findNumericNode(const string &path) {
    auto ptr = findNode(path);

    // check if result is numeric, if this is not the case, try to resolve it as
    // a reference to another node
    if (ptr) {
        auto v = split(ptr->m_value, "px");
        if (v.size() > 1) {
            return make_tuple(ptr, unitType::Pixels, v[0]);
        } else {
            v = split(ptr->m_value, "%");
            if (v.size() > 1) {
                return make_tuple(ptr, unitType::Percent, v[0]);
            } else if (is_number(ptr->m_value)) {
                return make_tuple(ptr, unitType::Pixels, ptr->m_value);
            } else {
                // try to resolve as node reference
                ptr = getRoot()->findNode(ptr->m_value);
                if (ptr) {
                    v = split(ptr->m_value, "px");
                    if (v.size() > 1) {
                        return make_tuple(ptr, unitType::Pixels, v[0]);
                    } else {
                        v = split(ptr->m_value, "%");
                        if (v.size() > 1) {
                            return make_tuple(ptr, unitType::Percent, v[0]);
                        } else if (is_number(ptr->m_value)) {
                            return make_tuple(ptr, unitType::Pixels, ptr->m_value);
                        }
                    }
                }
            }
        }
    }

    return make_tuple(nullptr, unitType::Pixels, "");
}

ResNode *ResNode::getByName(const string &name) {
    if (!m_node.empty()) {
        for (Ptr &node : m_node) {
            if (node->isName(name)) {
                return node.get();
            }
        }
    }

    return nullptr;
}

ResNode *ResNode::findNode(vector<string> &v, int level) {
    ResNode *r;
    if (level >= static_cast<int>(v.size())) {
        return nullptr;
    }
    if ((r = getByName(v[level])) == nullptr) {
        return nullptr;
    }
    return level == v.size() - 1 ? r : r->findNode(v, level + 1);
}

ResNode *ResNode::findNodeFromNode(const string &path, ResNode *rnode) {
    if (rnode == nullptr) {
        rnode = getRoot();
    }
    return rnode->findNode(path);
}

// ---------------------------------------------------[ VALUES ]--------------------------------------------------------

string ResNode::getValue(const string &name, string def) {
    for (Ptr &node : m_node) {
        if (node->isName(name)) {
            return node->m_value;
        }
    }

    return def;
}

std::vector<float> ResNode::valuefv(const string &path, int fcount, float def) {
    std::vector<float> v;
    ResNode           *node = findNode(path);
    if (node == nullptr) {
        return std::move(v);
    }
    ParVec tok = node->splitValue();
    fcount     = fcount ? fcount : tok.getParCount();
    for (int i = 0; i < fcount; i++) {
        v.emplace_back(tok.getFloatPar(i, def));
    }
    return std::move(v);
}

bool ResNode::isInPixels(const string &name) {
    ResNode *ptr = getByName(name);
    if (!ptr) {
        return false;
    }
    return split(ptr->m_value, "px").size() > 1;
}

bool ResNode::isInPercent(const string &name) {
    ResNode *ptr = getByName(name);
    if (!ptr) {
        return false;
    }
    return split(ptr->m_value, "%").size() > 1;
}

ParVec ResNode::splitValue(char sep) {
    ParVec        tok;
    istringstream f(m_value);
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
    for (Ptr &node : m_node) {
        node->generateReport(ritem, level + 1);
    }
    return true;
}

bool ResNode::isEqual(ResNode *unode) {
    if (unode == nullptr) {
        return false;
    }
    if (m_name != unode->m_name) {
        return false;
    }
    if (m_value != unode->m_value) {
        return false;
    }
    if (m_func != unode->m_func) {
        return false;
    }
    if (m_par.getParCount() != unode->m_par.getParCount()) {
        return false;
    }
    int i, n = m_par.getParCount();
    for (i = 0; i < n; i++) {
        if (m_par[i] != unode->m_par[i]) {
            return false;
        }
    }
    return true;
}

bool ResNode::copy(ResNode *unode) {
    if (unode == nullptr) {
        return false;
    }
    m_name  = unode->m_name;
    m_value = unode->m_value;
    m_func  = unode->m_func;
    m_par.clear();
    for (string &s : unode->m_par) {
        m_par.emplace_back(s);
    }
    return true;
}
}  // namespace ara