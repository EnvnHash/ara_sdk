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

#include <GlbCommon/GlbCommon.h>

namespace ara {

class UtfIterator {
public:
    std::string::const_iterator str_iter;
    size_t                      cplen{};

    explicit UtfIterator(std::string::const_iterator str_iter) : str_iter(std::move(str_iter)) {
        find_cplen();
    }

    std::string operator*() const {
        return {str_iter, str_iter + static_cast<int32_t>(cplen)};
    }

    UtfIterator &operator++() {
        str_iter += static_cast<int32_t>(cplen);
        find_cplen();
        return *this;
    }

    bool operator!=(const UtfIterator &o) const { return this->str_iter != o.str_iter; }

private:
    void find_cplen() {
        cplen = 1;
        if ((*str_iter & 0xf8) == 0xf0) {
            cplen = 4;
        } else if ((*str_iter & 0xf0) == 0xe0) {
            cplen = 3;
        } else if ((*str_iter & 0xe0) == 0xc0) {
            cplen = 2;
        }
    }
};

}

