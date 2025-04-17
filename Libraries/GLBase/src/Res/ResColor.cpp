#include "Res/ResColor.h"

#include <utility>

using namespace std;

namespace ara {

ResColor::ResColor(string name, GLBase *glbase) : ResNode(std::move(name), glbase) {}

bool ResColor::OnProcess() {
    if (hasFunc()) {
        rgba[3] = 1.f;

        if (isFunc("rgb")) {
            for (int i = 0; i < 3; i++) rgba[i] = getIntPar(i, 0) / 255.f;
        } else if (isFunc("rgba")) {
            for (int i = 0; i < 4; i++) rgba[i] = getIntPar(i, 0) / 255.f;
        } else if (isFunc("rgbf")) {
            for (int i = 0; i < 3; i++) rgba[i] = getFloatPar(i, 0);
        } else if (isFunc("rgbaf")) {
            for (int i = 0; i < 4; i++) rgba[i] = getFloatPar(i, 0);
        } else if (isFunc("hsl")) {
            hsla2rgba(rgba, fmodf(getFloatPar(0), 360.f), getFloatPar(1) / 100.f, getFloatPar(2) / 100.f, 1.f);
        } else if (isFunc("hsla")) {
            hsla2rgba(rgba, fmodf(getFloatPar(0), 360.f), getFloatPar(1) / 100.f, getFloatPar(2) / 100.f,
                      getFloatPar(3));
        }

    } else {
        if (!m_Value.empty()) hexColor2rgba(rgba, m_Value.c_str());
    }

    return true;
}

bool ResColor::isClass(ResNode *snode) {
    if (snode->hasFunc()) {
        for (string &s : colorFunc) {
            if (snode->isFunc(s)) return true;
        }
    } else {
        if (!snode->m_Value.empty() && hexColor2rgba(nullptr, snode->m_Value.c_str())) {
            return true;
        }
    }
    return false;
}

bool ResColor::OnSourceResUpdate(bool deleted, ResNode *unode) {
    auto *c = static_cast<ResColor *>(unode);

    rgba[0] = c->rgba[0];
    rgba[1] = c->rgba[1];
    rgba[2] = c->rgba[2];
    rgba[3] = c->rgba[3];

    return true;
}

bool ResColor::hexColor2rgba(float *rgba, const char *str) {
    int len = 0;

    if (str == nullptr) return false;

    if (str[0] != '#') return false;

    str++;

    while (str[len] && ((str[len] >= '0' && str[len] <= '9') || (str[len] >= 'A' && str[len] <= 'F') ||
                        (str[len] >= 'a' && str[len] <= 'f')))
        len++;

    if (len != 3 && len != 6) return false;

    if (rgba == nullptr) return true;

    rgba[3] = 1.f;

    if (len == 3) {  // hex code shorthand? (#FFF => #FFFFFF, #D29 => #DD2299)

        for (int i = 0, aux; i < 3; i++) {
            aux = hex2dec(str[i]);

            rgba[i] = ((aux << 4) | aux) / 255.f;
        }

        return true;
    }

    if (len == 6) {  // conventional

        for (int i = 0; i < 3; i++) rgba[i] = (hex2dec(str[i << 1]) << 4 | hex2dec(str[(i << 1) + 1])) / 255.f;

        return true;
    }

    return false;
}

bool ResColor::hsla2rgba(float *rgba, float h, float s, float l, float a) {
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
