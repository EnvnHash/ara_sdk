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

    void Reset() { m_Pos = 0; }

    int  MoveToNextBoundary();  // moves to the position after the boundary
    void SetBoundary(const std::string &bstr);

    uint8_t *getData() { return (uint8_t *)m_ContentData.data(); }
    int      getSize() { return (int)m_ContentData.size(); }
    int      ReadLine(std::string &s, int max_size = 1024);

    int getCurrentPos() { return m_Pos; }

private:
    std::vector<uint8_t> m_ContentData;

    int m_Size    = 0;
    int m_Pos     = 0;
    int m_LastPos = 0;

    std::string m_Boundary;
};

}