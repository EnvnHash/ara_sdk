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

namespace ara {

template <typename T>
class CycleBuffer {
public:
    template <typename T2>
    class CIBufferArray {
    public:
        CIBufferArray() = default;
        explicit CIBufferArray(size_t s) { m_data.resize(s); }
        T2    *getData() { return &m_data[0]; }
        size_t size() { return m_data.size(); }
        void   clear() { m_data.clear(); }

        std::vector<T2> m_data;
    };

    void allocateBuffers(size_t nrBuffs, size_t bufferSize) {
        m_buffer.clear();
        for (size_t i = 0; i < nrBuffs; i++) {
            m_buffer.emplace_back(CIBufferArray<T>(bufferSize));
        }
    }

    // ...for use with c legacy code, suppose an input c-array of the same type and size as CIBufferArray<T>
    size_t feed(T *buffer) {
        std::copy(buffer, buffer + m_buffer[m_buffPos].size(), m_buffer[m_buffPos].getData());
        return countUp();
    }

    size_t countUp() {
        m_buffLastPos = m_buffPos;
        ++m_buffPos;
        m_buffPos %= m_buffer.size();
        ++m_fillAmt;
        return m_buffPos;
    }

    CIBufferArray<T> *getLastBuff() {
        if (m_buffLastPos < 0) {
            return nullptr;
        }
        return &m_buffer[m_buffLastPos];
    }

    CIBufferArray<T> *getBuff(size_t idx) {
        if (idx >= m_buffer.size()) {
            return nullptr;
        }
        return &m_buffer[idx];
    }

    CIBufferArray<T> *consume() {
        if (m_buffer.empty()) {
            return nullptr;
        }

        --m_fillAmt;
        m_tcp        = m_consumePos;
        m_consumePos = ++m_consumePos % m_buffer.size();
        return &m_buffer[m_tcp];
    }

    void clear() {
        for (auto &it : m_buffer) {
            it.clear();
        }
        m_buffer.clear();
        m_buffPos     = 0;
        m_buffLastPos = 0;
        m_fillAmt = 0;
        m_lastUplBuf = nullptr;
    }

    CIBufferArray<T> *getWriteBuff() { return &m_buffer[m_buffPos]; }
    bool              isFilled() { return m_fillAmt >= m_buffer.size(); }
    size_t            getLastBuffPos() const { return m_buffLastPos; }
    size_t            size() { return m_buffer.size(); }
    bool              empty() { return m_buffer.empty(); }
    size_t            getWritePos() const { return m_buffPos; }
    size_t            getFillAmt() { return m_fillAmt; }
    size_t            getFreeSpace() { return m_buffer.size() - m_fillAmt; }

protected:
    T                            *m_lastUplBuf = nullptr;
    std::vector<CIBufferArray<T>> m_buffer;
    size_t                        m_buffPos     = 0;
    size_t                        m_buffLastPos = 0;
    std::atomic<size_t>           m_fillAmt     = 0;
    size_t                        m_consumePos  = 0;
    size_t                        m_tcp         = 0;
};
}  // namespace ara