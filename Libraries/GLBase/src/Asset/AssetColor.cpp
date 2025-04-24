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

#include "Asset/AssetColor.h"

using namespace std;

namespace ara {

AssetColor::AssetColor(string name, GLBase *glbase) : ResNode(std::move(name), glbase) {}

void AssetColor::onProcess() {
    if (hasFunc()) {
        rgba[3] = 1.f;

        if (isFunc("rgb")) {
            for (int i = 0; i < 3; i++) {
                rgba[i] = static_cast<float>(getIntPar(i, 0)) / 255.f;
            }
        } else if (isFunc("rgba")) {
            for (int i = 0; i < 4; i++) {
                rgba[i] = static_cast<float>(getIntPar(i, 0)) / 255.f;
            }
        } else if (isFunc("rgbf")) {
            for (int i = 0; i < 3; i++) {
                rgba[i] = getFloatPar(i, 0);
            }
        } else if (isFunc("rgbaf")) {
            for (int i = 0; i < 4; i++) {
                rgba[i] = getFloatPar(i, 0);
            }
        } else if (isFunc("hsl")) {
            hsla2rgba(rgba, fmodf(getFloatPar(0), 360.f), getFloatPar(1) / 100.f, getFloatPar(2) / 100.f, 1.f);
        } else if (isFunc("hsla")) {
            hsla2rgba(rgba, fmodf(getFloatPar(0), 360.f), getFloatPar(1) / 100.f, getFloatPar(2) / 100.f,
                      getFloatPar(3));
        }

    } else {
        if (!m_value.empty()) {
            hexColor2rgba(rgba, m_value.c_str());
        }
    }
}

bool AssetColor::isClass(const ResNode *snode) {
    if (snode->hasFunc()) {
        for (string &s : colorFunc) {
            if (snode->isFunc(s)) {
                return true;
            }
        }
    } else {
        if (!snode->m_value.empty() && hexColor2rgba(nullptr, snode->m_value.c_str())) {
            return true;
        }
    }
    return false;
}

bool AssetColor::onSourceResUpdate(bool deleted, ResNode *unode) {
    auto *c = dynamic_cast<AssetColor *>(unode);
    for (int i=0; i < 4; i++) {
        rgba[i] = c->rgba[i];
    }
    return true;
}

bool AssetColor::hexColor2rgba(float *rgba, const char *str) {
    int len = 0;
    if (str == nullptr) {
        return false;
    }

    if (str[0] != '#') {
        return false;
    }

    ++str;

    while (str[len] && ((str[len] >= '0' && str[len] <= '9') || (str[len] >= 'A' && str[len] <= 'F') ||
                        (str[len] >= 'a' && str[len] <= 'f'))) {
        ++len;
    }

    if (len != 3 && len != 6) {
        return false;
    }

    if (rgba == nullptr) {
        return true;
    }

    rgba[3] = 1.f;

    if (len == 3) {  // hex code shorthand? (#FFF => #FFFFFF, #D29 => #DD2299)
        for (int i = 0, aux; i < 3; i++) {
            aux = static_cast<int>(hex2dec(str[i]));
            rgba[i] = static_cast<float>((aux << 4) | aux) / 255.f;
        }
        return true;
    }

    if (len == 6) {  // conventional
        for (int i = 0; i < 3; i++) {
            rgba[i] = static_cast<float>(hex2dec(str[i << 1]) << 4 | hex2dec(str[(i << 1) + 1])) / 255.f;
        }

        return true;
    }
    return false;
}

bool AssetColor::hsla2rgba(float *rgba, float h, float s, float l, float a) {
    float C = (1 - fabsf(2 * l - 1)) * s;
    float X = C * (1 - fabsf(fmodf(h / 60.f, 2) - 1));
    float m = l - C / 2.f;

    if (h >= 0 && h < 60) {
        rgba[0] = C;
        rgba[1] = X;
        rgba[2] = 0.f;
    } else if (h >= 60 && h < 120) {
        rgba[0] = X;
        rgba[1] = C;
        rgba[2] = 0.f;
    } else if (h >= 120 && h < 180) {
        rgba[0] = 0.f;
        rgba[1] = C;
        rgba[2] = X;
    } else if (h >= 180 && h < 240) {
        rgba[0] = 0.f;
        rgba[1] = X;
        rgba[2] = C;
    } else if (h >= 240 && h < 300) {
        rgba[0] = X;
        rgba[1] = 0.f;
        rgba[2] = C;
    } else if (h >= 300 && h < 360) {
        rgba[0] = C;
        rgba[1] = 0.f;
        rgba[2] = X;
    }

    rgba[0] = rgba[0] + m;
    rgba[1] = rgba[1] + m;
    rgba[2] = rgba[2] + m;
    rgba[3] = a;

    return true;
}

}  // namespace ara
