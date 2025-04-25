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

#include <vector>

namespace ara {

template <typename T>
class Median {
public:
    Median(float _medAmt, unsigned int bufferSize = 2) {
        m_bufferSize = bufferSize;
        m_medAmt     = _medAmt;
        m_medDiv     = 1.f / (m_medAmt + 1.f);
        m_ptr        = 0;
        m_filled     = false;

        m_values.resize(bufferSize);
    }

    void update(T newVal) {
        if (m_filled) {
            m_values[m_ptr] = (newVal + m_values[(m_ptr - 1 + m_bufferSize) % m_bufferSize] * m_medAmt) * m_medDiv;
        } else {
            m_values[m_ptr] = newVal;
        }

        ++m_ptr;
        if (!m_filled && m_ptr == 2) {
            m_filled = true;
        }
        m_ptr %= m_bufferSize;
    }

    void add(T newVal) {
        m_values[m_ptr % m_bufferSize] = newVal;
        ++m_ptr;
    }

    void calcMed() {
        T tmp;
        for (int i = 0; i < m_bufferSize; i++) {
            tmp += m_values[i];
        }
        tmp /= static_cast<float>(m_bufferSize);
        m_medValue = tmp;
    }

    T get() {
        return m_values[(m_ptr - 1 + m_bufferSize) % m_bufferSize];
    }

    T getMed() {
        return m_medValue;
    }

    void clear() {
        std::fill(m_values.begin(), m_values.end(), {});
        m_filled = false;
    }

private:
    std::vector<T> m_values;
    T              m_medValue;
    float          m_medAmt     = 0.f;
    float          m_medDiv     = 0.f;
    bool           m_filled     = false;
    unsigned short m_ptr        = 0;
    unsigned int   m_bufferSize = 0;
};
}  // namespace ara
