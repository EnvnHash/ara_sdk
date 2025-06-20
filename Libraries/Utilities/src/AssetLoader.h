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

#pragma once

#include <util_common.h>

using memRet = std::pair<size_t, std::filesystem::file_time_type>;

namespace ara {

class AssetLoader {
public:
    struct memPtr {
        const uint8_t* data{};
        size_t size{};
        std::filesystem::file_time_type modTime{};
    };

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    static std::string loadAssetAsString(const T& path);

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    static memRet loadAssetToMem(std::vector<uint8_t>& buf, const T& path);

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    static memPtr mapAssetToMem(const T& path);

    static void unMapAsset(memPtr& memPtr);

    static std::string getSanitizedAssetPath(const std::filesystem::path& p);

    static void setAssetPath(const std::filesystem::path& p) {
        m_assetPath = p;
    }

    static std::filesystem::path resolveAssetPath(const std::filesystem::path& p) {
        return m_assetPath / p;
    }

    static std::filesystem::file_time_type getLastFileUpdate(const std::string& path);
    static std::filesystem::path& getAssetPath() { return m_assetPath; }

    static bool usingCmrc() {
#ifdef ARA_USE_CMRC
        return true;
#else
        return false;
#endif
    }

private:
    static inline std::filesystem::path m_assetPath;
};

}
