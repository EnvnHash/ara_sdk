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
#include "Conditional.h"

namespace ara {

template <typename T>
struct is_vector : std::false_type {};

template <typename T>
struct is_list : std::false_type {};

template <typename T>
struct is_deque : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T, typename Alloc>
struct is_list<std::list<T, Alloc>> : std::true_type {};

template <typename T, typename Alloc>
struct is_deque<std::list<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value || is_list<T>::value || is_deque<T>::value;

template <typename T>
class CycleBuffer {
public:
    void allocate(size_t nrBuffs, size_t bufferSize=1) {
        m_buffer.clear();
        for (size_t i = 0; i < nrBuffs; i++) {
            if constexpr (is_vector_v<T>) {
                m_buffer.emplace_back(T(bufferSize));
            } else {
                m_buffer.emplace_back();
            }
        }
        m_filledCond.setFlagResetOnWaitEnd(false);
        m_filledCond.notify();
    }

    template<typename C>
    // ...for use with c legacy code, suppose an input c-array of the same type and size as CIBufferArray<T>
    size_t feed(C *content) {
        std::copy(content, content + m_buffer[m_writePos].size(), m_buffer[m_writePos].data());
        return feedCountUp();
    }

    size_t feed(T& elem) {
        m_buffer[m_writePos] = elem;
        return feedCountUp();
    }

    size_t feedCountUp() {
        m_buffLastPos = m_writePos;
        ++m_writePos %= m_buffer.size();
        ++m_fillAmt;
        if (m_fillAmt == m_buffer.size()) {
            m_filledCond.reset();
        }
        return m_writePos;
    }

    size_t consumeCountUp() {
        --m_fillAmt;
        if (!m_filledCond.isNotified()) {
            m_filledCond.notify();
        }
        auto rp        = m_readPos;
        ++m_readPos %= m_buffer.size();
        return rp;
    }

    T *getLastBuff() {
        if (m_buffLastPos < 0) {
            return nullptr;
        }
        return &m_buffer[m_buffLastPos];
    }

    T *getBuff(size_t idx) {
        if (idx >= m_buffer.size()) {
            return nullptr;
        }
        return &m_buffer[idx];
    }

    T *consume() {
        if (m_buffer.empty() || m_fillAmt == 0) {
            return nullptr;
        }
        auto rp = consumeCountUp();
        if (m_consumedCb) {
            m_consumedCb();
        }
        return &m_buffer[rp];
    }

    size_t getCapacity() {
        return m_buffer.size();
    }

    void clear() {
        if constexpr (is_vector_v<T>) {
            for (auto &it : m_buffer) {
                it.clear();
            }
        }
        m_buffer.clear();
        m_writePos     = 0;
        m_buffLastPos = 0;
        m_fillAmt = 0;
        m_lastUplBuf = nullptr;
    }

    void waitUntilNotFilled() {
        m_filledCond.wait(0);
    }

    auto&   getBuffer() { return m_buffer; }
    T&      getWriteBuff() { return m_buffer[m_writePos]; }
    T&      getReadBuff() { return m_buffer[m_readPos]; }
    bool    isFilled() { return m_fillAmt >= m_buffer.size(); }
    size_t  getLastBuffPos() const { return m_buffLastPos; }
    size_t  size() { return m_buffer.size(); }
    bool    empty() { return m_buffer.empty(); }
    size_t  getWritePos() const { return m_writePos; }
    size_t  getReadPos() const { return m_readPos; }
    size_t  getFillAmt() { return m_fillAmt; }
    size_t  getFreeSpace() { return m_buffer.size() - m_fillAmt; }
    void    setConsumedCb(const std::function<void()>& f) { m_consumedCb = f; }
    void    setReadPos(size_t pos) { m_readPos = pos; }

protected:
    T                       *m_lastUplBuf = nullptr;
    std::vector<T>          m_buffer;
    size_t                  m_writePos = 0;
    size_t                  m_buffLastPos = 0;
    std::atomic<size_t>     m_fillAmt = 0;
    size_t                  m_readPos = 0;
    std::function<void()>   m_consumedCb;
    Conditional             m_filledCond;
};

}  // namespace ara