
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

#include <Network/NetworkCommon.h>

namespace ara::microhttp {

class Content {
public:
    bool        setName(const std::string& str);
    std::string getName();
    bool        setType(const std::string& str);
    std::string getType();
    bool        setFileName(const std::string& str);
    std::string getFileName();
    bool        setData(uint8_t *dataptr, int datasize);

    [[nodiscard]] uint8_t    *getData() const;
    [[nodiscard]] int         getSize() const;
    [[nodiscard]] bool        storeToFile(const std::filesystem::path &p) const;

private:
    std::string m_name;
    std::string m_type;
    std::string m_fileName;
    uint8_t*    m_dataPtr  = nullptr;
    int         m_dataSize = 0;
};



}

