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

#include "RwBinFile.h"

using namespace std;

namespace ara {
std::size_t ReadBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath) {
    vp.clear();

    if (!std::filesystem::exists(filepath)) {
        LOGE << "ReadBinFile Error, " << filepath.string() << " does not exist";
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
        file.read(reinterpret_cast<char *>(&vp[0]), static_cast<streamsize>(size));
    }

    return vp.size();
}

std::size_t WriteBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath) {
    ofstream file(filepath, ios::out | ios::binary);

    if (!file.is_open()) {
        return 0;
    }

    if (!vp.empty()) {
        file.write(reinterpret_cast<char *>(&vp[0]),  static_cast<streamsize>(vp.size()));
    }

    return vp.size();
}

std::size_t WriteBinFile(void *buff, std::size_t size, const std::filesystem::path& filepath) {
    ofstream file(filepath, ios::out | ios::binary);

    if (!file.is_open()) {
        return 0;
    }

    if (size > 0 && buff != nullptr) {
        file.write(static_cast<char *>(buff),  static_cast<streamsize>(size));
    }

    return size;
}

}  // namespace ara
