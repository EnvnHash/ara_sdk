//
// Created by hahne on 22.04.2025.
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

#include <AssetLoader.h>
#include <RwBinFile.h>
#include <string_utils.h>

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(ara);
#endif

using namespace std;
using memRet = std::pair<size_t, std::filesystem::file_time_type>;

namespace ara {

string AssetLoader::loadAssetAsString(const filesystem::path& path) {
    return loadAssetAsString(path.string());
}

string AssetLoader::loadAssetAsString(const string& path) {
    auto compPath = resolveAssetPath(path);
    try {
#ifdef ARA_USE_CMRC
        auto compPathStr = getSanitizedAssetPath(compPath);
        auto fs = cmrc::ara::get_filesystem();
        if (fs.exists(compPathStr)) {
            auto file = fs.open(compPathStr);
            auto cont = string(file.begin(), file.end());
            return ReplaceString(cont, "\r\n", "\n");
        } else {
            throw runtime_error("AssetLoader::loadAssetAsString: Could not open file " + compPath.string());
        }
#else
        string content;
        if (filesystem::exists(compPath)) {
            ifstream file(compPath);
            if (!file.is_open()) {
                throw runtime_error("AssetLoader::getAssetAsString: Could not open file " + compPath.string());
            }

            file.seekg(0, ios::end);
            size_t fileSize = static_cast<size_t>(file.tellg());
            if (fileSize == 0) {
                content.clear();
                throw runtime_error("AssetLoader::getAssetAsString: file " + compPath.string() + " is empty");
            }

            file.seekg(0, ios::beg);
            content.resize(fileSize);
            file.read(&content[0], fileSize);
            if (content.empty()) {
                throw runtime_error("AssetLoader::getAssetAsString: Error reading from file " + compPath.string());
            }

            file.close();
            return content;
        } else {
            throw runtime_error("AssetLoader::getAssetAsString: Could not open file");
        }
#endif
    } catch (runtime_error& e) {
        LOGE << e.what();
    }

    return {};
}

memRet AssetLoader::loadAssetToMem(vector<uint8_t>& buf, const filesystem::path& path) {
    return loadAssetToMem(buf, path.string());
}

memRet AssetLoader::loadAssetToMem(vector<uint8_t>& buf, const string& path) {
    auto compPath = resolveAssetPath(path);
#ifdef ARA_USE_CMRC
    buf.clear();

    auto compPathStr = getSanitizedAssetPath(compPath);
    auto fs = cmrc::ara::get_filesystem();
    if (!fs.exists(compPathStr)) {
        return { 0, std::filesystem::file_time_type{} };
    }

    auto file = fs.open(compPathStr);
    if (file.size() == 0) {
        return { 0, std::filesystem::file_time_type{} };
    }

    size_t size = file.size();
    if (size > 0) {
        buf.resize(size);
        copy(file.begin(), file.end(), buf.begin());
    }

    return { buf.size(), std::filesystem::file_time_type{} };

#else
    return { ReadBinFile(buf, compPath), filesystem::last_write_time(compPath) };
#endif
}

string AssetLoader::getSanitizedAssetPath(const filesystem::path& p) {
#ifdef _WIN32
    // CMRC uses linux separators, using filesystem::path separators on windows will fail here
    return ReplaceString(p.string(), "\\", "/");
#else
    return path.string();
#endif
}

std::filesystem::file_time_type AssetLoader::getLastFileUpdate(const std::string& path) {
#ifdef ARA_USE_CMRC
    return  std::filesystem::file_time_type{};
#else
    auto compPath = resolveAssetPath(path);
    return  filesystem::last_write_time(compPath);
#endif
}

}