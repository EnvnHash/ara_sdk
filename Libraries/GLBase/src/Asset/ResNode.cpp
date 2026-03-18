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
    return ranges::find_if(str, [&](auto &x) { return x > 32; }) == str.end();
}

bool SrcLine::isComment() const {
    const char *e = SrcFile::clearSpaces(str.c_str());
    return (e[0] == '#');
}

ResNode::ResNode(std::string name, const SrcLine *line, GLBase *glBase) {
    m_name       = std::move(name);
    m_glbase     = glBase;
    srcLineIndex = line != nullptr ? line->index : 0;
}

string ResNode::getPath() {
    string            str;
    vector<ResNode *> nl;
    _r_getPath(nl);
    const int n = static_cast<int>(nl.size());
    for (int i = n - 2; i >= 0; i--) {
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

void ResNode::logtree(const int level) {
    if (m_value.empty() && !m_func.empty()) {
        string str;
        for (string &sp : m_par) {
            str += sp + "|";
        }
    }

    for (const auto &node : m_node) {
        node->logtree(level + 1);
    }
}

ResNode *ResNode::add(Ptr node) {
    if (!node) {
        return nullptr;
    }
    m_node.emplace_back(std::move(node));
    const auto rnode = m_node.back().get();
    rnode->setParent(this);
    return rnode;
}

ResNode *ResNode::setParent(ResNode *parent) {
    m_parent = parent;
    setAssetManager(parent != nullptr ? parent->getAssetManager() : nullptr);
    return m_parent;
}

ResNode *ResNode::getFlag(const string &flagname) const {
    for (const auto &node : m_node) {
        if (node->isFlag(flagname)) {
            return node.get();
        }
    }
    return nullptr;
}

void ResNode::process() {
    onProcess();
    for (const auto &node : m_node) {
        node->process();
    }
}

bool ResNode::load() {
    m_findNodeCache.clear();

    try {
        onLoad();
        for (const auto &node : m_node) {
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

ResNode::Ptr ResNode::preprocess(const int level) {
    const int   n = static_cast<int>(m_node.size());
    Ptr         pret;

    for (int i = 0; i < n; i++) {
        if (const auto node = m_node[i].get(); (pret = node->choose()) != nullptr) {
            const auto newNode = pret.get();
            newNode->setParent(this);
            newNode->grabNode(node);

            m_node[i] = std::move(pret);  // previous node gets deleted here
        }
    }

    for (const auto &nd : m_node) {
        nd->preprocess(level + 1);
    }

    return nullptr;
}

ResNode *ResNode::findNode(const string &path) {
    // first check for cached results
    if (const auto n = m_findNodeCache.find(path); n != m_findNodeCache.end()) {
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
    // check if result is numeric, if this is not the case, try to resolve it as
    // a reference to another node
    if (auto ptr = findNode(path)) {
        auto v = split(ptr->m_value, "px");
        if (v.size() > 1) {
            return make_tuple(ptr, unitType::Pixels, v[0]);
        }

        v = split(ptr->m_value, "%");
        if (v.size() > 1) {
            return make_tuple(ptr, unitType::Percent, v[0]);
        }

        if (is_number(ptr->m_value)) {
            return make_tuple(ptr, unitType::Pixels, ptr->m_value);
        }

        // try to resolve as node reference
        ptr = getRoot()->findNode(ptr->m_value);
        if (ptr) {
            v = split(ptr->m_value, "px");
            if (v.size() > 1) {
                return make_tuple(ptr, unitType::Pixels, v[0]);
            }

            v = split(ptr->m_value, "%");
            if (v.size() > 1) {
                return make_tuple(ptr, unitType::Percent, v[0]);
            }

            if (is_number(ptr->m_value)) {
                return make_tuple(ptr, unitType::Pixels, ptr->m_value);
            }
        }
    }

    return make_tuple(nullptr, unitType::Pixels, "");
}

ResNode *ResNode::getByName(const string &name) const {
    if (!m_node.empty()) {
        for (const auto &node : m_node) {
            if (node->isName(name)) {
                return node.get();
            }
        }
    }

    return nullptr;
}

ResNode *ResNode::findNode(vector<string> &v, const int level) const {
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

string ResNode::getValue(const string &name, string def) const {
    for (const auto &node : m_node) {
        if (node->isName(name)) {
            return node->m_value;
        }
    }

    return def;
}

std::vector<float> ResNode::valuefv(const string &path, int fcount, float def) {
    std::vector<float> v;
    const ResNode *node = findNode(path);
    if (node == nullptr) {
        return std::move(v);
    }
    const ParVec tok = node->splitValue();
    fcount = fcount ? fcount : tok.getParCount();
    for (int i = 0; i < fcount; i++) {
        v.emplace_back(tok.getFloatPar(i, def));
    }
    return std::move(v);
}

bool ResNode::isInPixels(const string &name) const {
    const auto ptr = getByName(name);
    if (!ptr) {
        return false;
    }
    return split(ptr->m_value, "px").size() > 1;
}

bool ResNode::isInPercent(const string &name) const {
    const auto ptr = getByName(name);
    if (!ptr) {
        return false;
    }
    return split(ptr->m_value, "%").size() > 1;
}

ParVec ResNode::splitValue(const char sep) const {
    ParVec        tok;
    istringstream f(m_value);
    string        s;
    while (getline(f, s, sep)) {
        tok.emplace_back(s);
    }
    return tok;
}

ParVec ResNode::splitNodeValue(const string &valueName, const char sep) const {
    ParVec   tok;
    if (ResNode *node; (node = getByName(valueName)) != nullptr) {
        return node->splitValue(sep);
    }
    return tok;
}

bool ResNode::generateReport(std::vector<e_repitem> &ritem, const int level) {
    ritem.emplace_back(e_repitem{this, level, getPath()});
    for (const auto &node : m_node) {
        node->generateReport(ritem, level + 1);
    }
    return true;
}

bool ResNode::isEqual(const ResNode *unode) const {
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
    const int n = m_par.getParCount();
    for (int i = 0; i < n; i++) {
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