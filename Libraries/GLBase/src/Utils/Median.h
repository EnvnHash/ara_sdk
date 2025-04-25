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

namespace ara {

template <typename T>
class Median {
public:
    Median(float _medAmt, unsigned int _bufferSize = 2) {
        bufferSize = _bufferSize;
        medAmt     = _medAmt;
        medDiv     = 1.f / (medAmt + 1.f);
        ptr        = 0;
        filled     = false;
        values     = new T[_bufferSize];
    }

    ~Median() = default;

    void update(T newVal) {
        if (filled)
            values[ptr] = (newVal + values[(ptr - 1 + bufferSize) % bufferSize] * medAmt) * medDiv;
        else
            values[ptr] = newVal;

        ptr++;
        if (!filled && ptr == 2) filled = true;
        ptr %= bufferSize;
    }

    void add(T _newVal) {
        values[ptr % bufferSize] = _newVal;
        ptr++;
    }

    void calcMed() {
        T tmp;
        for (int i = 0; i < bufferSize; i++) tmp += values[i];
        tmp /= static_cast<float>(bufferSize);
        medValue = tmp;
    }

    T get() { return values[(ptr - 1 + bufferSize) % bufferSize]; }
    T getMed() { return medValue; }

    void clear() {
        delete[] values;
        values = new T[bufferSize];
        filled = false;
    }

private:
    T             *values = nullptr;
    T              medValue;
    float          medAmt     = 0.f;
    float          medDiv     = 0.f;
    bool           filled     = false;
    unsigned short ptr        = 0;
    unsigned int   bufferSize = 0;
};
}  // namespace ara
