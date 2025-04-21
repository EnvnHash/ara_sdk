#include "RwBinFile.h"

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(ara);
#endif

using namespace std;

namespace ara {
std::size_t ReadBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath) {
    vp.clear();

#ifdef ARA_USE_CMRC
    auto fs = cmrc::ara::get_filesystem();
    if (!fs.exists(filepath.string())) return false;

    auto file = fs.open(filepath.string());
    if (!file.size()) return 0;

    size_t size = file.size();
    if (size > 0) {
        vp.resize(size);
        std::copy(file.begin(), file.end(), vp.begin());
    }

#else
    if (!std::filesystem::exists(filepath)) {
        return false;
    }

    ifstream file(filepath, ios::in | ios::binary | ios::ate);
    if (!file.is_open()) {
        return 0;
    }

    size_t size = static_cast<int>(file.tellg());

    if (size > 0) {
        vp.resize(size);
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char *>(&vp[0]), size);
    }
#endif

    return vp.size();
}

std::size_t WriteBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath) {
    ofstream file(filepath, ios::out | ios::binary);

    if (!file.is_open()) {
        return 0;
    }

    if (vp.size() > 0) {
        file.write((char *)&vp[0], vp.size());
    }

    return vp.size();
}

std::size_t WriteBinFile(const void *buff, std::size_t size, const std::filesystem::path& filepath) {
    ofstream file(filepath, ios::out | ios::binary);

    if (!file.is_open()) {
        return 0;
    }

    if (size > 0 && buff != nullptr) {
        file.write((char *)buff, size);
    }

    return size;
}

}  // namespace ara
