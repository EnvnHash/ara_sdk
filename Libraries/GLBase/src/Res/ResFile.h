//
// Created by user on 04.02.2021.
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {

class ResFile {
public:
    struct e_hdr {
        uint32_t fid;
        uint32_t ver;
        uint32_t entry_count;
        uint64_t entry_pos;
        uint64_t extra_pos;
        char     encode_par[256];  // encoding parameters, maybe to use compression
                                   // or encyption
    };

    struct e_entry {
        char     path[256];
        uint64_t pos;
        uint64_t comp_size;
        uint64_t uncomp_size;
    };

    ResFile();

    virtual ~ResFile() = default;

    bool   OpenFromFile(const std::filesystem::path &srcpath);
    size_t ReadEntry(std::vector<uint8_t> &dest, std::string &path);
    bool   BeginCreate(const std::filesystem::path &fpath);
    bool   Feed(std::vector<uint8_t> &vp, const std::string &path);
    bool   Finish();
    bool   isOK() { return !m_Entry.empty(); }

private:
    std::filesystem::path src_FilePath;
    std::vector<e_entry>  m_Entry;
    e_hdr                 m_Header{};

    e_entry *FindEntry(std::string &path);

    std::unique_ptr<std::ofstream> o_File;
};

}  // namespace ara
