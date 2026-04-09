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

AssetColor::AssetColor(string name, GLBase *glBase) : ResNode(std::move(name), glBase) {}

void AssetColor::onProcess() {
    if (hasFunc()) {
        m_rgba[3] = 1.f;

        static unordered_map<string, function<void(AssetColor*)>> funcMap {
            {"rgb",     [] (AssetColor* ctx) { ctx->setRgbFromIntParams(ctx->m_rgba, 3); }},
            {"rgba",    [] (AssetColor* ctx) { ctx->setRgbFromIntParams(ctx->m_rgba, 4); }},
            {"rgbf",    [] (AssetColor* ctx) { ctx->setRgbFromFloatParams(ctx->m_rgba, 3); }},
            {"rgbaf",   [] (AssetColor* ctx) { ctx->setRgbFromFloatParams(ctx->m_rgba, 4); }},
            {"hsl",     [] (AssetColor* ctx) { ctx->hsla2rgba(ctx->m_rgba,
                                                fmodf(ctx->getFloatPar(0), 360.f),
                                                ctx->getFloatPar(1) / 100.f,
                                                ctx->getFloatPar(2) / 100.f, 1.f); }},
            {"hsla",    [] (AssetColor* ctx) { hsla2rgba(ctx->m_rgba,
                                                fmodf(ctx->getFloatPar(0), 360.f),
                                                ctx->getFloatPar(1) / 100.f,
                                                ctx->getFloatPar(2) / 100.f,
                                                ctx->getFloatPar(3)); }}
        };
        funcMap[getFunc()](this);
    } else {
        if (!m_value.empty()) {
            hexColor2rgba(m_rgba, m_value.c_str());
        }
    }
}

void AssetColor::setRgbFromIntParams(float *rgba, const int count) const {
    for (int i = 0; i < count; i++) {
        rgba[i] = static_cast<float>(getIntPar(i, 0)) / 255.f;
    }
}

void AssetColor::setRgbFromFloatParams(float *rgba, const int count) const {
    for (int i = 0; i < count; i++) {
        rgba[i] = getFloatPar(i, 0);
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
        if (!snode->getRawValue().empty() && hexColor2rgba(nullptr, snode->getRawValue().c_str())) {
            return true;
        }
    }
    return false;
}

bool AssetColor::onSourceResUpdate(bool deleted, ResNode *unode) {
    auto *c = dynamic_cast<AssetColor *>(unode);
    for (int i=0; i < 4; i++) {
        m_rgba[i] = c->m_rgba[i];
    }
    return true;
}

bool AssetColor::hexColor2rgba(float *rgba, const char *str) {
        if (str == nullptr || str[0] != '#') {
            return false;
        }

        ++str;

        int len = 0;
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

        const auto setChannelFromShortHex = [&](const int i) {
            const auto aux = static_cast<int>(hex2dec(str[i]));
            rgba[i] = static_cast<float>((aux << 4) | aux) / 255.f;
        };

        const auto setChannelFromLongHex = [&](const int i) {
            rgba[i] = static_cast<float>(hex2dec(str[i << 1]) << 4 | hex2dec(str[(i << 1) + 1])) / 255.f;
        };

        const auto setChannels = [&](auto&& setter) {
            for (int i = 0; i < 3; i++) {
                setter(i);
            }
        };

        if (len == 3) {
            setChannels(setChannelFromShortHex);
            return true;
        }

        setChannels(setChannelFromLongHex);
        return true;
    }

bool AssetColor::hsla2rgba(float *rgba, const float h, const float s, const float l, const float a) {
    const auto C = (1 - fabsf(2 * l - 1)) * s;
    const auto X = C * (1 - fabsf(fmodf(h / 60.f, 2) - 1));
    const auto m = l - C / 2.f;

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
