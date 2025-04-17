//
// Created by sven on 04-03-25.
//

#include "NodeColor.h"

namespace ara::node {

Color::Color() {
    ara::Node::setTypeName<node::Color>();
    m_changeCb[Node::cbType::postChange][this] = [this]{
        m_rgba[3] = 1.f;

        if (m_format == "rgb") {
            for (int i = 0; i < 3; i++) {
                m_rgba[i] = m_color[i] / 255.f;
            }
        } else if (m_format == "rgba") {
            for (int i = 0; i < 4; i++) {
                m_rgba[i] = m_color[i] / 255.f;
            }
        } else if (m_format == "rgbf") {
            for (int i = 0; i < 3; i++) {
                m_rgba[i] = m_color[i];
            }
        } else if (m_format == "rgbaf") {
            for (int i = 0; i < 4; i++) {
                m_rgba[i] = m_color[i];
            }
        } else if (m_format == "hsl") {
            hsla2rgba(m_rgba.data(), fmodf(m_color[0], 360.f), m_color[1] / 100.f, m_color[2] / 100.f, 1.f);
        } else if (m_format == "hsla") {
            hsla2rgba(m_rgba.data(), fmodf(m_color[0], 360.f), m_color[1] / 100.f, m_color[2] / 100.f, m_color[3]);
        }
    };
}

bool Color::hexColor2rgba(float *rgba, const char *str) {
    int len = 0;
    if (!str || str[0] != '#') {
        return false;
    }

    str++;

    while (str[len] && ((str[len] >= '0' && str[len] <= '9') || (str[len] >= 'A' && str[len] <= 'F') ||
                        (str[len] >= 'a' && str[len] <= 'f'))) {
        len++;
    }

    if (len != 3 && len != 6) {
        return false;
    }
    if (!rgba) {
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

bool Color::hsla2rgba(float *rgba, float h, float s, float l, float a) {
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

}