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

#include <Network/MicroHttpContentRaw.h>
#include <Network/MicroHttpConnection.h>

namespace ara::microhttp {

bool RawContent::readContentData(Conn *conn, uint32_t count) {
    m_contentData.reserve(count + 1024);

    uint32_t tread = 0;
    uint32_t left  = count;
    uint32_t mtr  = 16 << 10;
    uint8_t *dest = m_contentData.data();
    int      ret;

    m_contentData.resize(count);

    while (left > 0) {
        auto tr = std::min<uint32_t>(mtr, left);

        if ((ret = conn->sRecv(dest, static_cast<int>(tr))) > 0) {
            tread += ret;
            left -= ret;
            dest += ret;
        } else {
            return false;
        }
    }

    return true;
}

bool RawContent::readContentData(SOCKET sock, uint32_t count) {
    m_contentData.reserve(count + 1024);

    uint32_t tread = 0;
    uint32_t left  = count;
    uint32_t mtr  = 16 << 10;
    uint8_t *dest = m_contentData.data();
    int      ret;

    m_contentData.resize(count);

    while (left > 0) {
        uint32_t tr = std::min<uint32_t>(mtr, left);

        if ((ret = recv(sock, reinterpret_cast<char *>(dest), static_cast<int>(tr), 0)) > 0) {
            tread += ret;
            left -= ret;
            dest += ret;
        } else {
            return false;
        }
    }

    return true;
}

int RawContent::moveToNextBoundary() {
    const char *bstr = m_boundary.c_str();
    int         blen = static_cast<int>(m_boundary.length());
    int         lpos = static_cast<int>(m_contentData.size());

    if (blen <= 0) {
        return -1;
    }

    lpos -= blen - 4;  // --\r\n

    if (lpos < 0) {
        return -2;
    }

    auto src = reinterpret_cast<char *>(m_contentData.data());

    while (m_pos < lpos) {
        if (src[m_pos] == bstr[0] && !memcmp(&src[m_pos], bstr, blen)) {  // check first byte and if it passes then perform
                                                                          // a full check with memcmp, this increases speed
            m_pos += blen;

            if (src[m_pos + 0] == '-' && src[m_pos + 1] == '-') {
                m_pos += 2;
                return -4;  // reached the end
            }

            if (src[m_pos + 0] == '\r' && src[m_pos + 1] == '\n') {
                m_pos += 2;
            }
            return m_pos;
        }
        ++m_pos;
    }
    return -3;
}

bool RawContent::isEndBoundary(const char* src, int pos) {
    return src[pos] == '-' && src[pos + 1] == '-';
}

bool RawContent::isLineBreak(const char* src, int pos) {
    return src[pos] == '\r' && src[pos + 1] == '\n';
}

int RawContent::readLine(std::string &s, int max_size) {
    int lpos = static_cast<int>(m_contentData.size());
    int ipos = m_pos, len = 0;

    if (max_size <= 0 || m_pos >= lpos) {
        return -1;
    }

    auto src = reinterpret_cast<char *>(m_contentData.data());

    --lpos;

    while (m_pos < lpos && (len = (m_pos - ipos)) <= max_size) {
        if (src[m_pos + 0] == '\r' && src[m_pos + 1] == '\n') {
            s.assign(&src[ipos], len);
            m_pos += 2;
            return len;
        }

        ++m_pos;
    }

    s.assign(&src[ipos], len);
    return -2;
}

void RawContent::setBoundary(const std::string &bstr) {
    m_boundary = bstr;
}

}