#include "Res/ResInstance.h"

#include <RwBinFile.h>

#include "Res/ImgSection.h"
#include "Res/ResColor.h"
#include "Res/ResSrcFile.h"

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(ara);
#endif

using namespace std;

namespace ara {

Instance::Instance(const string &data_root_path, const string &compilation_filepath, GLBase *glbase)
    : m_glbase(glbase) {
    m_DataRootPath = data_root_path;
    m_FontList.setGlbase(glbase);

    if (!m_DataRootPath.empty())
        if (m_DataRootPath[m_DataRootPath.size() - 1] != '/')
#if defined(_WIN32) && !defined(ARA_USE_CMRC)
            m_DataRootPath += '\\';
#else
            m_DataRootPath += '/';
#endif

    rootNode = std::make_unique<ResNode>("root", m_glbase);
    rootNode->setInstance(this);

    // try to open the compiled resource file, if it doesn't exist, try to load
    // resources as separate files
    m_ResFile.OpenFromFile(compilation_filepath);

    populateFolderFiles();
}

bool Instance::Load(const string &path, bool force_external) {
    SrcFile              sfile(m_glbase);
    std::vector<uint8_t> vp;

    m_LoadState   = 1;
    m_ResFilePath = path;

    if (!usingComp()) {
        m_ResSysFile = m_DataRootPath + m_ResFilePath;

#ifdef ARA_USE_CMRC
        auto fs = cmrc::ara::get_filesystem();
        if (!fs.exists(m_ResSysFile.string())) return false;
#else
        if (!filesystem::exists(m_ResSysFile)) {
            LOGE << "Instance::Load Error: could not open " << m_ResSysFile;
            return false;
        }

        m_ResFileLastTime = filesystem::last_write_time(m_ResSysFile);
#endif
    }

    loadResource(nullptr, vp, path, force_external);

    if (!getPreContent().empty()) {
        vp.insert(vp.begin(), getPreContent().size(), 0);
        memcpy(&vp[0], getPreContent().c_str(), (int)getPreContent().size());
    }

    if (!getPostContent().empty()) {
        size_t p = vp.size();
        vp.resize(p + getPostContent().size());
        memcpy(&vp[p], getPostContent().c_str(), (int)getPostContent().size());
    }

    m_LoadState = false;

    if (sfile.Process(rootNode.get(), vp)) {
        rootNode->Preprocess();
        rootNode->Process();

        if (!rootNode->errList.empty()) {
            return rootNode->error((char *)"instance: Cannot process");
        }

        if (!rootNode->Load()) {
            return rootNode->error((char *)"instance: Cannot load");
        }

        // get fonts
        auto fontsNode = findNode("fonts");
        if (fontsNode) {
            for (auto &f : fontsNode->m_Node) {
                m_FontLUT.insert({f->m_Name, e_font_lut{f->m_Value, 20}});
            }
        }

        m_LoadState = true;
        return true;
    }

    return rootNode->error((char *)"instance: Cannot process source file");
}

bool Instance::getvalue(string &dest, const string &path) {
    auto *node = findNode<ResFont>(path);
    if (node == nullptr) {
        return false;
    }

    dest = node->m_Value;
    return true;
}

bool Instance::getvalue(string &dest, const string &path, int index) {
    auto *node = findNode<ResFont>(path);

    if (node == nullptr) {
        return false;
    }

    ParVec tok = node->splitValue();

    if (index < 0 || index >= tok.getParCount()) {
        return false;
    }

    dest = tok.getPar(index);
    return true;
}

bool Instance::valueiv(std::vector<int> &v, const string &path, int fcount, int def) {
    v.clear();

    auto *node = findNode<ResFont>(path);
    if (node == nullptr) {
        return false;
    }

    ParVec tok = node->splitValue();

    fcount = fcount > 0 ? fcount : tok.getParCount();
    for (int i = 0; i < fcount; i++) {
        v.push_back(tok.getIntPar(i, def));
    }

    return true;
}

bool Instance::valuefv(std::vector<float> &v, const string &path, int fcount, float def) {
    v.clear();

    auto *node = findNode<ResFont>(path);
    if (node == nullptr) {
        return false;
    }

    ParVec tok = node->splitValue();
    fcount     = fcount > 0 ? fcount : tok.getParCount();

    for (int i = 0; i < fcount; i++) {
        v.push_back(tok.getFloatPar(i, def));
    }

    return true;
}

float *Instance::color(const string& path) {
    auto c = findNode<ResColor>(path);
    return c == nullptr ? default_Color : c->getColor4fv();
}

Font *Instance::font(const string& path, float pixRatio) {
    auto font = findNode<ResFont>(path);
    auto f    = getGLFont(string("Fonts/verdana.ttf"), 20, pixRatio);

    if (font != nullptr) {
        Font *faux;
        if ((faux = getGLFont(font->m_FontPath, font->m_Size, pixRatio)) != nullptr) f = faux;
    }

    return f;
}

Font *Instance::getGLFont(string font_type_path, int size, float pixRatio) {
    if (m_FontLUT.find(font_type_path) != m_FontLUT.end()) {
        e_font_lut f   = m_FontLUT[font_type_path];
        font_type_path = f.path;
    }

    auto f = m_FontList.find(font_type_path, size, pixRatio);

    if (f == nullptr) {
        std::vector<uint8_t> vp;
        if (loadResource(nullptr, vp, font_type_path, false) > 0) {
            if ((f = m_FontList.add(vp, font_type_path, size, pixRatio)) != nullptr) {
                return f;
            }
        }
    }

    if (!f) {
        LOGE << "Cannot open font (" << font_type_path << ") size=" << size;
    }

    return f;
}

size_t Instance::loadResource(ResNode *node, std::vector<uint8_t> &dest, const string& path, bool force_external) {
    bool use_external = force_external;

    if (usingComp() && !force_external) {  // try internal compilation first unless force_external is required
        return loadResourceFromCompilation(dest, path);
    }

    if (node) {
        e_file_bind                   eb{node->getPath(), path};
        vector<e_file_bind>::iterator wIt;
        if ((wIt = find_if(m_FileBind.begin(), m_FileBind.end(), [&](e_file_bind &e) {
                 return e.file_path == eb.file_path && e.node_path == eb.node_path;
             })) == m_FileBind.end()) {
            m_FileBind.push_back(eb);
        }
    }

    return loadResourceFromExternal(dest, m_DataRootPath + path);
}

size_t Instance::loadResourceFromCompilation(std::vector<uint8_t> &dest, std::string path) {
    if (!m_ResFile.isOK()) {
        return 0;
    }
    return m_ResFile.ReadEntry(dest, path);
}

size_t Instance::loadResourceFromExternal(std::vector<uint8_t> &dest, const std::string &path) {
    size_t ts = ReadBinFile(dest, path);
    if (!ts) {
        LOGE << "[ERROR] loadResourceFromExternal(" << path << ") size=" << ts;
    }
    return ts;
}

Font *Instance::loadFont(const string& path, int size, float pixRatio) {
    std::vector<uint8_t> v;
    if (loadResource(nullptr, v, path) <= 0) {
        return nullptr;
    }
    return m_FontList.add(v, path, size, pixRatio);
}

void Instance::populateFolderFiles() {
    if (m_ResFile.isOK()) {
        return;
    }

    if (!std::filesystem::exists(m_DataRootPath)) {
        return;
    }

    for (const filesystem::directory_entry &file : filesystem::recursive_directory_iterator(m_DataRootPath)) {
        m_ResFolderFiles[file] = filesystem::last_write_time(file);
    }
}

bool Instance::checkForResSourceChange() {
    if (usingComp() || !isOK()) {
        return false;
    }

    filesystem::file_time_type ft;

    try {
        ft = filesystem::last_write_time(m_ResSysFile);
    } catch (...) {
        return false;
    }

    return ft != m_ResFileLastTime;
}

bool Instance::checkForChangesInFolderFiles() {
    bool change = false;

    if (usingComp()) {
        return false;
    }

    if (!isOK()) {
        return false;
    }

    filesystem::file_time_type ft;

    // Check for new files...
    try {
        for (const filesystem::directory_entry &file : filesystem::recursive_directory_iterator(m_DataRootPath)) {
            if (m_ResFolderFiles.find(file) == m_ResFolderFiles.end()) {
                m_ResFolderFiles[file] = filesystem::last_write_time(file);
            }
        }
    } catch (...) {
    }

    // check for file deletion or modification
    bool keep;

    do {
        keep = false;
        for (auto &e : m_ResFolderFiles) {
            if (!filesystem::exists(e.first)) {
                change = true;
                break;
            } else {
                try {
                    ft = filesystem::last_write_time(e.first);
                } catch (...) {
                }

                if (ft != e.second) change = true;
            }
        }
    } while (keep);

    return change;
}

bool Instance::Reload() {
    ResNode::Ptr nroot;

    nroot = std::make_unique<ResNode>("root", m_glbase);
    nroot->setInstance(this);

    SrcFile              sfile(m_glbase);
    std::vector<uint8_t> vp;
    bool                 err = true;

    loadResource(nullptr, vp, m_ResFilePath, true);

    if (!getPreContent().empty()) {
        vp.insert(vp.begin(), getPreContent().size(), 0);
        memcpy(&vp[0], getPreContent().c_str(), (int)getPreContent().size());
    }

    if (!getPostContent().empty()) {
        size_t p = vp.size();
        vp.resize(p + getPostContent().size());
        memcpy(&vp[p], getPostContent().c_str(), (int)getPostContent().size());
    }

    if (sfile.Process(nroot.get(), vp)) {
        nroot->Preprocess();
        nroot->Process();

        if (nroot->errList.empty()) {
            if (nroot->Load()) {
                if (nroot->errList.empty()) {
                    rootNode = std::move(nroot);
                    err      = false;
                    return true;
                }
            }
        }

        if (err) {
            LOGE << "New resource file has errors";
            for (ResNode::e_error &err : nroot->errList) {
                LOGE << "Line " << std::to_string(err.lineIndex + 1) << " " << err.errorString;
            }
        }
    }

    return err;
}

void Instance::CallResSourceChange() {
    if (usingComp() || !isOK()) {
        return;
    }

    filesystem::file_time_type ft;

    try {
        ft = filesystem::last_write_time(m_ResSysFile);
    } catch (...) {
        return;
    }

    // std::unique_lock<mutex> lock(m_updtMtx);

    if (ft != m_ResFileLastTime) {
        ResNode::Ptr nroot;

        LOG << "\x1B[93mRes-File Changed (" << m_ResSysFile << ") \x1B[37m";

        nroot = std::make_unique<ResNode>("root", m_glbase);
        nroot->setInstance(this);

        SrcFile              sfile(m_glbase);
        std::vector<uint8_t> vp;
        bool                 err = true;

        loadResource(nullptr, vp, m_ResFilePath, true);

        if (!getPreContent().empty()) {
            vp.insert(vp.begin(), getPreContent().size(), 0);
            memcpy(&vp[0], getPreContent().c_str(), (int)getPreContent().size());
        }

        if (!getPostContent().empty()) {
            size_t p = vp.size();
            vp.resize(p + getPostContent().size());
            memcpy(&vp[p], getPostContent().c_str(), (int)getPostContent().size());
        }

        if (sfile.Process(nroot.get(), vp)) {
            nroot->Preprocess();
            nroot->Process();

            if (nroot->errList.empty())
                if (nroot->Load())
                    if (nroot->errList.empty()) {
                        rootNode = std::move(nroot);
                        err      = false;
                    }

            if (err) {
                LOGE << "New resource file has errors";
                for (ResNode::e_error &e : nroot->errList) {
                    LOGE << "Line " << std::to_string(e.lineIndex + 1) << " " << e.errorString;
                }
            }
        }

        m_ResFileLastTime = ft;
    }
}

void Instance::CallForChangesInFolderFiles() {
    if (usingComp() || !isOK()) return;

    filesystem::file_time_type ft;

    // Check for new files...
    for (const filesystem::directory_entry &file : filesystem::recursive_directory_iterator("resdata")) {
        if (m_ResFolderFiles.find(file) == m_ResFolderFiles.end()) {
            m_ResFolderFiles[file] = filesystem::last_write_time(file);
        }
    }

    // check for file deletion or modification
    bool keep;

    do {
        keep = false;

        for (auto &e : m_ResFolderFiles) {
            if (!filesystem::exists(e.first)) {
                m_ResFolderFiles.erase(e.first);
                keep = true;
                break;
            } else {
                try {
                    ft = filesystem::last_write_time(e.first);
                } catch (...) {
                }

                if (ft != e.second) {
                    string str = e.first.path().string();
                    std::replace(str.begin(), str.end(), '\\', '/');
                    str.erase(0, m_DataRootPath.size());
                    PropagateFileChange(false, str);
                    m_ResFolderFiles[e.first] = ft;
                }
            }
        }
    } while (keep);
}

void Instance::PropagateFileChange(bool deleted, const string &fpath) {
    if (usingComp()) {
        return;
    }

    ResNode *node;
    LOG << "PROPAGATE " << fpath;

    for (e_file_bind &fb : m_FileBind) {
        if (fb.file_path == fpath) {
            if ((node = findNode(fb.node_path)) != nullptr) {
                LOG << "propagate to " << fpath;
                node->OnResourceChange(deleted, fpath);
            }
        }
    }
}

bool Instance::Compile(const filesystem::path& p) {
    if (usingComp()) return false;

    ResFile         rfile;
    vector<uint8_t> vp;

    if (!rfile.BeginCreate(p)) return false;

    for (auto &elem : m_ResFolderFiles) {
        string ppath = elem.first.path().string();
        std::replace(ppath.begin(), ppath.end(), '\\', '/');
        ppath.erase(0, m_DataRootPath.size());

        if (elem.first.is_regular_file()) {
            if (ReadBinFile(vp, elem.first.path()) >= 0) {
                rfile.Feed(vp, ppath);
            }
        }
    }

    rfile.Finish();

    return true;
}

ImageBase *Instance::img(const string& path) {
    ResNode *node = findNode(path);

    if (node == nullptr || !(typeid(node[0]) == typeid(ImgSrc) || typeid(node[0]) == typeid(ImgSection))) {
        return nullptr;
    }

    return (ImageBase *)node;
}

std::string Instance::value(const std::string &path) {
    auto node = findNode<ResFont>(path);
    if (node == nullptr) {
        return {};
    }
    return node->m_Value;
}

std::string Instance::value(const std::string &path, const std::string& def) {
    auto node = findNode<ResFont>(path);
    if (node == nullptr) {
        return def;
    }
    return node->m_Value;
}

int Instance::value1i(const std::string &path, int def) {
    std::string s;
    if (!getvalue(s, path)) return def;
    int v;
    try {
        v = stoi(s);
    } catch (...) {
        v = def;
    }
    return v;
}

float Instance::value1f(const std::string &path, float def) {
    std::string s;
    if (!getvalue(s, path)) return def;
    float v;
    try {
        v = stof(s);
    } catch (...) {
        v = def;
    }
    return v;
}

}  // namespace ara
