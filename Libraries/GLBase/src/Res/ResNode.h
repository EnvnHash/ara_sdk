//
// Created by user on 04.02.2021.
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {

class Instance;

class SrcLine {
public:
    int         index = 0;
    std::string str;

    bool isEmpty();
    [[nodiscard]] bool isComment() const;
};

class ParVec : public std::vector<std::string> {
public:
    virtual ~ParVec() = default;

    bool        validIndex(size_t index) { return (index >= 0 && index < size()); }
    std::string getPar(size_t index, const std::string &def = {}) { return validIndex(index) ? at(index) : def; }

    int getIntPar(size_t index, int def) {
        if (!validIndex(index)) return def;
        int v = def;
        try {
            v = stoi(at(index));
        } catch (...) {
        }
        return v;
    }

    float getFloatPar(size_t index, float def) {
        if (!validIndex(index)) return def;
        float v = def;
        try {
            v = stof(at(index));
        } catch (...) {
        }
        return v;
    }

    int         getParCount() { return (int)size(); }
    std::string operator()(size_t index) { return !validIndex(index) ? std::string{} : at(index); }
    std::string s(size_t index) { return !validIndex(index) ? std::string{} : at(index); }

    int i(size_t index, int def = 0) {
        if (!validIndex(index)) return def;
        int v = def;
        try {
            v = stoi(at(index));
        } catch (...) {
        }
        return v;
    }

    float f(size_t index, float def = 0.f) {
        if (!validIndex(index)) return def;
        float v = def;
        try {
            v = stof(at(index));
        } catch (...) {
            std::cerr << "ParVec::f(size_t index, float def=0.f) exception thrown" << std::endl;
        }
        return v;
    }

    int n() { return (int)size(); }
};

class GLBase;

class ResNode {
public:
    struct e_error {
        int         lineIndex;  // starting from 0
        std::string errorString;
    };

    class ErrorList : public std::vector<e_error> {};

    ErrorList errList;

    using Ptr = std::unique_ptr<ResNode>;

    int srcLineIndex = -1;

    std::string m_Name;
    std::string m_Value;
    std::string m_Func;
    ParVec      m_Par;

    std::vector<Ptr> m_Node;

    explicit ResNode(std::string name, GLBase *glbase) {
        m_Name   = std::move(name);
        m_glbase = glbase;
    }

    ResNode(std::string name, SrcLine *line, GLBase *glbase);

    virtual ~ResNode() = default;

    ResNode *getRoot() {
        if (getParent() == nullptr) return this;
        return getParent()->getRoot();
    }

    std::string getPath();

    bool _r_getPath(std::vector<ResNode *> &nl);

    ResNode *findNode(const std::string &path);  // Finds a node in the tree, in the form of
                                                 // "node.child.child"
    template <typename T>
    T *findNode(const std::string &path) {
        ResNode *n = findNode(path);
        if (!n) return nullptr;
        if (typeid(n[0]) == typeid(T)) {
            return (T *)n;
        } else {  // check if this is a reference to another style definition
            ResNode *n2 = getRoot()->findNode(n->m_Value);
            if (n2 && typeid(n2[0]) == typeid(T)) {
                return (T *)n2;
            } else {
                return nullptr;
            }
        }
    }

    ResNode                                     *findNode(std::vector<std::string> &v, int level);  // Recursive function to find a child in the tree
    std::tuple<ResNode *, unitType, std::string> findNumericNode(const std::string &path);  // same as above but resolves reference to other styles and
                                   // checks if the value is numeric
    ResNode *findNodeFromNode(const std::string &path, ResNode *rnode);  // Finds a node by path starting
                                                // from a root rnode (snode)
    ResNode *findNodeFromRoot(const std::string& path) {
        return findNodeFromNode(path, nullptr);
    }  // Finds a node by path starting from the root
    ResNode *getByName(const std::string &name);  // Finds an immediate child with the giving
                                                  // name (childs in m_Node)

    bool error_string(const std::string &str);
    bool error(char *str, ...);
    void setValue(std::string value) { m_Value = std::move(value); }

    void setFunc(std::string func, ParVec &par) {
        m_Func = std::move(func);
        m_Par  = par;
    }

    void logtree(int level = 0);

    ResNode *add(Ptr node);

    ResNode *getParent() { return m_Parent; }
    bool     has(const std::string& path) { return findNode(path) != nullptr; };  // Checks for a node existence recursively
    bool     isFlag() const { return m_Node.empty(); };                    // Questions if this node is a flag
    bool     isFlag(const std::string &flagname) const {
        return m_Node.empty() && isName(flagname);
    }  // Finds a flag in the immediate childs (flag is a sole name item, such
       // as "bold", "img", ...)
    bool hasFlag(const std::string &flagname) {
        return getFlag(flagname) != nullptr;
    }  // Returns true if finds a flag with the giving name
    ResNode *getFlag(const std::string &flagname);  // Returns the pointer to the flag

    bool hasFunc() const { return !m_Func.empty(); }  // Does this have a function
    Ptr  Preprocess(int level = 0);
    Ptr  Choose();
    bool Process();

    virtual bool OnProcess() { return true; }
    bool Load();  // if any node fails to load will stop and return false
                  // (marco.g: this is subject to change, let's discuss it)
    virtual bool OnLoad() { return true; }
    virtual bool OnResourceChange(bool deleted, const std::string &res_fpath) {
        return true;
    };  // Called if a loadresource source has changed and used by this node

    virtual bool OnSourceResUpdate(bool deleted, ResNode *unode) {
        return true;
    }  // Called when the source resource file has changed, this function will
       // be called even if no data has changed, use unode to compare the
       // parameters

    virtual bool isOK() { return true; }

    bool grabNode(ResNode *from);  // Copies and transfers the from node to this

    bool        isName(const std::string &str) const { return m_Name == str; }
    bool        isFunc(const std::string &str) const { return m_Func == str; }
    std::string getPar(int index, const std::string &def = {}) {
        return m_Par.getPar(index, def);
    }  // Gets the token value for the current m_Par
    int   getIntPar(int index, int def = 0) { return m_Par.getIntPar(index, def); }
    float getFloatPar(int index, float def = 0) { return m_Par.getFloatPar(index, def); }
    int   getParCount() { return (int)m_Par.getParCount(); }

    // Values
    // --------------------------------------------------------------------------------------

    bool        hasValue(const std::string &path) { return getByName(path) != nullptr; }
    std::string getValue(const std::string &name, std::string def = {});  // Returns the value of a
                                                                          // giving immediate child node
    int   value1i(const std::string &name, int def);
    float value1f(const std::string &name, float def);
    bool  valueiv(std::vector<int> &v, const std::string &path, int fcount = 0, int def = 0);
    bool  valuefv(std::vector<float> &v, const std::string &path, int fcount = 0, float def = 0);

    std::vector<float> valuefv(const std::string &path, int fcount = 0, float def = 0);

    bool   isInPixels(const std::string &name);
    bool   isInPercent(const std::string &name);
    ParVec splitValue(char sep = ',');  // Splits in tokens the m_Value for this node
    ParVec splitNodeValue(const std::string &value_name, char sep = ',');  // Splits in tokens the m_Value for a node with name value_name

    struct e_repitem {
        ResNode    *node;
        int         level;
        std::string path;
    };

    bool generateReport(std::vector<e_repitem> &ritem, int level = 0);

    void      setInstance(Instance *inst) { m_Instance = inst; }
    Instance *getInstance() { return m_Instance; }

    bool     IsEqual(ResNode *unode);
    bool     Copy(ResNode *unode);
    unsigned setFlags(unsigned flags) { return (m_Flags = flags); }
    unsigned addFlags(unsigned flags) { return (m_Flags |= flags); }
    unsigned removeFlags(unsigned flags) { return (m_Flags &= ~flags); }
    unsigned getFlags() const { return m_Flags; }

protected:
    Instance *m_Instance = nullptr;
    ResNode  *m_Parent   = nullptr;
    GLBase   *m_glbase   = nullptr;

    ResNode *setParent(ResNode *parent);

    unsigned m_Flags = 0;

    std::unordered_map<std::string, ResNode *> m_findNodeCache;
};

}  // namespace ara