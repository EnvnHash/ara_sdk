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

#include <Network/NetworkCommon.h>

namespace ara::microhttp {

class Conn;

class RawContent {
public:
    bool readContentData(Conn *conn, uint32_t count);
    bool readContentData(SOCKET sock, uint32_t count);

    void reset() { m_pos = 0; }

    int  moveToNextBoundary();  // moves to the position after the boundary
    bool isEndBoundary(const char* src, int pos);
    bool isLineBreak(const char* src, int pos);
    void setBoundary(const std::string &bstr);

    uint8_t*                getData() { return m_contentData.data(); }
    [[nodiscard]] int32_t   getSize() const { return static_cast<int>(m_contentData.size()); }
    int32_t                 readLine(std::string &s, int max_size = 1024);
    [[nodiscard]] int32_t   getCurrentPos() const { return m_pos; }

private:
    std::vector<uint8_t>    m_contentData;
    int32_t                 m_size    = 0;
    int32_t                 m_pos     = 0;
    int32_t                 m_lastPos = 0;
    std::string             m_boundary;
};

}