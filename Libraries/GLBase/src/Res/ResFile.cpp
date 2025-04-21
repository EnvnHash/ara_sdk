#include "Res/ResFile.h"

#include <GLBase.h>

#include <cstring>

using namespace std;

namespace ara {

ResFile::ResFile() { m_Header.entry_count = 0; }

bool ResFile::OpenFromFile(const filesystem::path &srcpath) {
    m_Entry.clear();

    if (!filesystem::exists(srcpath)) {
        return false;
    }

    src_FilePath = srcpath;
    ifstream file(srcpath, ios::in | ios::binary | ios::ate);

    if (!file.is_open()) {
        return false;
    }

    size_t size = static_cast<int>(file.tellg());
    if (size < sizeof(e_hdr)) {
        return false;
    }

    file.seekg(0, ios::beg);
    file.read((char *)&m_Header, sizeof(e_hdr));

    if (m_Header.fid != ('R' | ('E' << 8) | ('S' << 16) | ('F' << 24))) {
        return false;
    }

    if (m_Header.ver != 0x0100) {
        return false;
    }

    if (m_Header.entry_count <= 0) {
        return true;
    }

    // let's set a limit, just in case file is corrupt (1M elements?  should this be fine?)
    if (m_Header.entry_count > 1 << 20) {
        return true;
    }

    file.seekg(m_Header.entry_pos);
    m_Entry.resize(m_Header.entry_count);
    file.read((char *)&m_Entry[0], sizeof(e_entry) * m_Header.entry_count);

    return true;
}

ResFile::e_entry *ResFile::FindEntry(string &path) {
    for (ResFile::e_entry &ee : m_Entry) {
        if (ee.path == path) {
            return &ee;
        }
    }

    return nullptr;
}

size_t ResFile::ReadEntry(std::vector<uint8_t> &dest, string &path) {
    e_entry *e;
    dest.clear();

    if ((e = FindEntry(path)) == nullptr) {
        return 0;
    }

    ifstream file(src_FilePath, ios::in | ios::binary);

    if (!file.is_open()) {
        LOGE << "[ERROR] ReadEntry(" << path << ") File not open";
        return 0;
    }

    if (e->pos == 0 || e->uncomp_size == 0) {
        LOGE << "[ERROR] ReadEntry(" << path << ") Position=" << e->pos << " / " << e->uncomp_size;
        return 0;
    }

    file.seekg(e->pos);

    if (e->comp_size == e->uncomp_size && !m_Header.encode_par[0]) {  // uncompressed
        dest.resize(e->uncomp_size);
        file.read((char *)&dest[0], e->uncomp_size);
        return dest.size();
    } else {
        // marco.g: implement compression later
        LOGE << "[ERROR] ReadEntry(" << path << ") Compression method not found (" << e->comp_size << ","
             << e->uncomp_size << "," << m_Header.encode_par;

        return 0;
    }

    LOGE << "[ERROR] ReadEntry(" << path << ") Unknown error";

    return 0;
}

bool ResFile::BeginCreate(const filesystem::path &fpath) {
    o_File = make_unique<ofstream>(fpath, ios::binary);
    m_Entry.clear();

    if (!o_File->is_open()) {
        return false;
    }

    memset(&m_Header, 0, sizeof(e_hdr));

    m_Header.fid = 'R' | ('E' << 8) | ('S' << 16) | ('F' << 24);
    m_Header.ver = 0x0100;

    o_File->write((char *)&m_Header, sizeof(ResFile::e_hdr));

    return true;
}

bool ResFile::Feed(std::vector<uint8_t> &vp, const string &path) {
    if (o_File == nullptr) {
        return false;
    }

    e_entry e{};

    memset(&e, 0, sizeof(e_entry));

    e.path          = path;
    e.pos         = o_File->tellp();
    e.comp_size   = vp.size();
    e.uncomp_size = vp.size();

    if (!m_Header.encode_par[0]) {
        o_File->write((char *)&vp[0], e.uncomp_size);
    } else {
        return false;
    }

    m_Entry.push_back(e);
    return true;
}

bool ResFile::Finish() {
    if (o_File == nullptr) {
        return false;
    }

    m_Header.entry_pos   = o_File->tellp();
    m_Header.entry_count = static_cast<uint32_t>(m_Entry.size());

    if (m_Header.entry_count > 0) {
        o_File->write((char *)&m_Entry[0], sizeof(ResFile::e_entry) * m_Entry.size());
    }

    o_File->seekp(0);
    o_File->write((char *)&m_Header, sizeof(ResFile::e_hdr));
    o_File->close();
    o_File.reset();

    return true;
}

}  // namespace ara