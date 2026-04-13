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

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4244)
#endif

#include <Asset/ParVec.h>
#include <string_utils.h>

namespace ara {

class AssetManager;
class GLBase;

class SrcLine {
public:
    int         index = 0;
    std::string str;

    bool isEmpty();
    [[nodiscard]] bool isComment() const;
};

class ResNode {
public:
    enum class ResNodeType : int32_t { standard = 0, font, color };

    struct e_error {
        int         lineIndex;  // starting from 0
        std::string errorString;
    };

    struct e_repitem {
        ResNode    *node;
        int         level;
        std::string path;
    };

    class ErrorList : public std::vector<e_error> {};

    using Ptr = std::unique_ptr<ResNode>;

    explicit ResNode(std::string name, GLBase *glbase) {
        m_name   = std::move(name);
        m_glbase = glbase;
    }

    ResNode(std::string name, const SrcLine *line, GLBase *glBase);

    virtual ~ResNode() = default;

    ResNode *getRoot() {
        if (getParent() == nullptr) {
            return this;
        }
        return getParent()->getRoot();
    }

    std::string getPath();
    bool        rGetPath(std::vector<ResNode *> &nl);
    ResNode*    findNode(const std::string &path);  // Finds a node in the tree, in the form of "node.child.child"


    template <typename T>
    std::unique_ptr<T> clone(ResNode *unode);

    template <typename T>
    T *findNode(const std::string &path) {
        ResNode *n = findNode(path);
        if (!n) {
            return nullptr;
        }

        if (typeid(n[0]) == typeid(T)) {
            return static_cast<T *>(n);
        }

        // check if this is a reference to another style definition
        auto n2 = getRoot()->findNode(n->m_value);
        if (n2 && typeid(n2[0]) == typeid(T)) {
            return static_cast<T *>(n2);
        }

        return nullptr;
    }

    ResNode                                     *findNode(std::vector<std::string> &v, int level) const;  // Recursive function to find a child in the tree
    std::tuple<ResNode *, unitType, std::string> findNumericNode(const std::string &path);  // same as above but resolves reference to other styles and checks if the value is numeric
    ResNode *findNodeFromNode(const std::string &path, ResNode *rnode);  // Finds a node by path starting from a root rnode (snode)
    ResNode *findNodeFromRoot(const std::string& path) {
        return findNodeFromNode(path, nullptr);
    }  // Finds a node by path starting from the root
    ResNode *getByName(const std::string &name) const;  // Finds an immediate child with the giving name (childs in m_Node)

    void setValue(std::string value) { m_value = std::move(value); }
    void setIsReference() { m_isReference = true; }
    void setFunc(std::string func, const ParVec &par) {
        m_func = std::move(func);
        m_par  = par;
    }

    void                    logtree(int level = 0);
    ResNode*                add(Ptr node);
    ResNode*                getParent() const { return m_parent; }
    bool                    has(const std::string& path) { return findNode(path) != nullptr; };
    [[nodiscard]] bool      isFlag() const { return m_children.empty(); };
    [[nodiscard]] bool      isFlag(const std::string &flagName) const {
        return m_children.empty() && isName(flagName);
    }  // Finds a flag in the immediate childs (flag is a sole name item, such as "bold", "img", ...)

    bool hasFlag(const std::string &flagName) const {
        return getFlag(flagName) != nullptr; // Returns true if it finds a flag with the giving name
    }

    ResNode *getFlag(const std::string &flagName) const;  // Returns the pointer to the flag

    bool hasFunc() const { return !m_func.empty(); }  // Does this have a function
    Ptr  preprocess(int level = 0);
    Ptr  choose();
    void process();

    virtual void onProcess() { }
    bool load();  // if any node fails to load will stop and return false
    void insertChild(const ResNode * fromNode, ResNode* toNode);
    virtual bool onLoad() { return true; }
    virtual bool onResourceChange(bool deleted, const std::string &res_fpath) { return true; };  // Called if a loadresource source has changed and used by this node
    virtual bool onSourceResUpdate(bool deleted, ResNode *unode) { return true; }  // Called when the source resource file has changed, this function will
       // be called even if no data has changed, use unode to compare the parameters
    virtual bool isOK() { return true; }

    bool grabNode(ResNode *from);  // Copies and transfers the from node to this
    bool isName(const std::string &str) const { return m_name == str; }
    bool isFunc(const std::string &str) const { return m_func == str; }

    const auto&     getFunc() { return m_func; }
    std::string     getPar(const int index, const std::string &def = {}) { return m_par.getPar(index, def); }  // Gets the token value for the current m_Par
    int             getIntPar(const int index, const int def = 0) const { return m_par.getIntPar(index, def); }
    float           getFloatPar(const int index, const float def = 0) const { return m_par.getFloatPar(index, def); }
    int             getParCount() const { return m_par.getParCount(); }
    bool            hasValue(const std::string &path) const { return getByName(path) != nullptr; }
    std::string     getValue(const std::string &name, std::string def = {}) const;  // Returns the value of a
    std::string     getName() const { return m_name; }

    template<CoordinateType32Signed T>
    T value(const std::string &name, T def) {
        T      v = def;
        ResNode *ptr;

        if ((ptr = getByName(name)) == nullptr) {
            return def;
        }

        try {
            if (const auto vp = split(ptr->m_value, "px"); vp.size() > 1) {
                v = typeid(T) == typeid(int32_t) ? stoi(vp[0]) : stof(vp[0]);
            } else {
                v = typeid(T) == typeid(int32_t) ? stoi(ptr->m_value) : stof(ptr->m_value);
            }
        } catch (...) {
            std::cerr << "ResNode::value() failed" << std::endl;
        }

        return v;
    }

    template<CoordinateType32Signed T>
    bool value_v(std::vector<T> &v, const std::string &path, int floatCount = 0, T def = 0) {
        v.clear();
        const auto node = findNode(path);
        if (node == nullptr) {
            return false;
        }
        auto tok = node->splitValue();
        floatCount     = floatCount > 0 ? floatCount : tok.getParCount();
        for (int i = 0; i < floatCount; i++) {
            v.emplace_back(typeid(T) == typeid(int32_t) ? tok.getIntPar(i, def) : tok.getFloatPar(i, def));
        }
        return true;
    }

    std::vector<float>          valuefv(const std::string &path, int floatCount = 0, float def = 0);
    bool                        isInPixels(const std::string &name) const;
    bool                        isInPercent(const std::string &name) const;
    ParVec                      splitValue(char sep = ',') const;  // Splits in tokens the m_value for this node
    ParVec                      splitNodeValue(const std::string &valueName, char sep = ',') const;  // Splits in tokens the m_value for a node with name value_name
    bool                        generateReport(std::vector<e_repitem> &ritem, int level = 0);
    void                        setAssetManager(AssetManager *inst) { m_assetManager = inst; }
    AssetManager*               getAssetManager() const { return m_assetManager; }
    std::string                 getRawValue() const { return m_value; }
    bool                        isReference() const { return m_isReference; }
    bool                        isEqual(const ResNode *unode) const;
    bool                        copy(ResNode *unode);
    std::unique_ptr<ResNode>    clone(const ResNode *unode);

    ErrorList           errList;
    int                 srcLineIndex = -1;
    std::string         m_func;
    ParVec              m_par;
    std::vector<Ptr>    m_children;

protected:
    ResNodeType         m_type = ResNodeType::standard;
    std::string         m_name;
    std::string         m_value;
    bool                m_isReference = false;

    ResNode *setParent(ResNode *parent);

    AssetManager    *m_assetManager = nullptr;
    ResNode         *m_parent   = nullptr;
    GLBase          *m_glbase   = nullptr;

    std::unordered_map<std::string, ResNode *> m_findNodeCache;
};

#ifdef _WIN32
#pragma warning(pop)
#endif

}  // namespace ara