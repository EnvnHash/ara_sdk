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

#include <glb_common/glb_common.h>

namespace ara {

class ParVec : public std::vector<std::string> {
public:
    ~ParVec() = default;

    [[nodiscard]] bool validIndex(size_t index) const {
        return index < size();
    }

    std::string getPar(size_t index, const std::string &def = {}) {
        return validIndex(index) ? at(index) : def;
    }

    [[nodiscard]] int getIntPar(size_t index, int def) const {
        if (!validIndex(index)) {
            return def;
        }
        try {
            return stoi(at(index));
        } catch (...) {
            LOGE << "ParVec::getIntPar string to integer conversion failed on " << at(index);
            return def;
        }
    }

    [[nodiscard]] float getFloatPar(size_t index, float def) const {
        if (!validIndex(index)) {
            return def;
        }
        try {
            return stof(at(index));
        } catch (...) {
            LOGE << "ParVec::getIntPar string to float conversion failed on " << at(index);
            return def;
        }
    }

    [[nodiscard]] int getParCount() const {
        return static_cast<int>(size());
    }

    std::string operator()(size_t index) {
        return !validIndex(index) ? std::string{} : at(index);
    }

    std::string s(size_t index) {
        return !validIndex(index) ? std::string{} : at(index);
    }

    [[nodiscard]] int i(size_t index, int def = 0) const {
        if (!validIndex(index)) {
            return def;
        }
        try {
            return stoi(at(index));
        } catch (...) {
            LOGE << "ParVec::i string to integer conversion failed on " << at(index);
            return def;
        }
    }

    [[nodiscard]] float f(size_t index, float def = 0.f) const {
        if (!validIndex(index)) {
            return def;
        }
        try {
            return stof(at(index));
        } catch (...) {
            LOGE << "ParVec::f string to float conversion failed on " << at(index);
            return def;
        }
    }

    [[nodiscard]] int n() const { return static_cast<int>(size()); }
};

}