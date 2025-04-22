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

#include <Network/MicroHttpContent.h>

namespace ara::microhttp {

bool Content::setName(const std::string& str) {
    m_Name = str;
    return true;
}

std::string Content::getName() {
    return m_Name;
}

bool Content::setType(const std::string& str) {
    m_Type = str;
    return true;
}

std::string Content::getType() {
    return m_Type;
}

bool Content::setFileName(const std::string& str) {
    m_FileName = str;
    return true;
}

std::string Content::getFileName() {
    return m_FileName;
}

bool Content::setData(uint8_t *dataptr, int datasize) {
    m_DataPtr  = dataptr;
    m_DataSize = datasize;
    return true;
}

uint8_t *Content::getData() const {
    return m_DataPtr;
}

int Content::getSize() const {
    return m_DataSize;
}

bool Content::storeToFile(const std::filesystem::path &p) const {
    if (getData() == nullptr) {
        return false;
    }

    std::ofstream f;

    try {
        f.open(p, std::ios::out | std::ios::binary | std::ios::trunc);
    } catch (...) {
        return false;
    }

    f.write(reinterpret_cast<char *>(getData()), getSize());
    f.close();

    return true;
}

}