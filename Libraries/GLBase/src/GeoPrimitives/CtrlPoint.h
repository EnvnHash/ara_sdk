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

#include <GlbCommon/CatmullCentri.h>
#include <GlbCommon/GlbCommon.h>

#include <earcut/earcut.hpp>
#include <pugixml/pugixml.hpp>

namespace ara {
class CtrlPoint {
public:
    enum moveDir { UI_CTRL_HORI = 0, UI_CTRL_VERT = 1 };

    CtrlPoint() = default;

    CtrlPoint(float x, float y) {
        position.x = x;
        position.y = y;
    }

    CtrlPoint(float x, float y, float tex_x, float tex_y) {
        position.x    = x;
        position.y    = y;
        refTexCoord.x = tex_x;
        refTexCoord.y = tex_y;
    }

    void                   setMouseIcon(MouseIcon icon) { mouseIcon = icon; }
    void                   setId(uint32_t inId) { id = inId; }
    [[nodiscard]] uint32_t getId() const { return id; }

    std::vector<glm::vec4>         movePath[2][2];
    glm::vec2                      position{0.f};
    glm::vec2                      screenPos{0.f};
    glm::vec2                      offsetPos{0.f};
    glm::vec2                      refPosition{0.f};
    glm::vec2                      refTexCoord{0.f};
    extrapolM                      extrPolMethod = extrapolM::Mirror;
    interpolM                      intrPolMethod = interpolM::Bilinear;
    uint32_t                       id            = 0;
    std::shared_ptr<SplineSegment> seg;
    unsigned int                   refGridPos[2] = {0, 0};
    unsigned int                   patchInd      = 0;
    float                          iPolTime      = 0.f;
    bool                           selected      = false;
    bool                           texBasePoint  = false;
    moveDir                        mDir          = moveDir::UI_CTRL_HORI;
    MouseIcon                      mouseIcon     = MouseIcon::arrow;

    template <class B>
    void serialize(B &buf) const {
        buf << position.x << position.y << refTexCoord.x << refTexCoord.y << offsetPos.x << offsetPos.y << refPosition.x
            << refPosition.y << static_cast<int>(extrPolMethod) << static_cast<int>(intrPolMethod) << id << iPolTime << refGridPos[0]
            << refGridPos[1] << patchInd << static_cast<int>(texBasePoint) << static_cast<int>(selected) << static_cast<int>(mDir) << static_cast<int>(mouseIcon);
    }

    template <class B>
    void parse(B &buf) {
        buf >> position.x >> position.y >> refTexCoord.x >> refTexCoord.y >> offsetPos.x >> offsetPos.y >>
            refPosition.x >> refPosition.y;

        int temp;
        buf >> temp;
        extrPolMethod = static_cast<extrapolM>(temp);
        buf >> temp;
        intrPolMethod = static_cast<interpolM>(temp);
        buf >> id >> iPolTime >> refGridPos[0] >> refGridPos[1] >> patchInd;

        buf >> temp;
        texBasePoint = static_cast<bool>(temp);
        buf >> temp;
        selected = static_cast<bool>(temp);
        buf >> temp;
        mDir = static_cast<moveDir>(temp);
        buf >> temp;
        mouseIcon = static_cast<MouseIcon>(temp);
    }

    void serializeToXml(pugi::xml_node &parent) {
        const char *ivecCompNames[2] = {"x", "y"};
        const char *ivecNames[3]     = {"position", "refPosition", "refTexCoord"};
        glm::vec2  *vals[3]          = {&position, &refPosition, &refTexCoord};
        for (size_t j = 0; j < 3; j++) {
            auto node = parent.append_child("vec2");
            node.append_attribute("propertyName").set_value(ivecNames[j]);
            for (int i = 0; i < 2; i++) node.append_attribute(ivecCompNames[i]).set_value((*vals[j])[i]);
        }

        auto node = parent.append_child("extrPolMethod");
        node.text().set(static_cast<int>(extrPolMethod));
        node.append_attribute("propertyName").set_value("extrPolMethod");

        node = parent.append_child("intrPolMethod");
        node.text().set(static_cast<int>(intrPolMethod));
        node.append_attribute("propertyName").set_value("intrPolMethod");

        node = parent.append_child("UInt");
        node.text().set(id);
        node.append_attribute("propertyName").set_value("id");

        node = parent.append_child("Float");
        node.text().set(iPolTime);
        node.append_attribute("propertyName").set_value("iPolTime");

        node = parent.append_child("Bool");
        node.text().set(texBasePoint);
        node.append_attribute("propertyName").set_value("texBasePoint");

        if (seg) {
            node = parent.append_child("SplineSegment");
            node.append_attribute("propertyName").set_value("SplineSegment");
            std::vector<std::string> attrNames = {"aX", "aY", "bX", "bY", "cX", "cY", "dX", "dY"};
            float                   *ptr       = &seg->a[0];
            for (auto &it : attrNames) node.append_attribute(it.c_str()).set_value(*ptr++);
        }
    }

    void parseFromXml(const pugi::xml_node &node) {
        pugi::xml_attribute attr;

        auto child = node.first_child();
        if ((attr = child.attribute("x"))) {
            position.x = attr.as_float();
        }
        if ((attr = child.attribute("y"))) {
            position.y = attr.as_float();
        }

        child = child.next_sibling();
        if ((attr = child.attribute("x"))) {
            refPosition.x = attr.as_float();
        }
        if ((attr = child.attribute("y"))) {
            refPosition.y = attr.as_float();
        }

        child = child.next_sibling();
        if ((attr = child.attribute("x"))) {
            refTexCoord.x = attr.as_float();
        }
        if ((attr = child.attribute("y"))) {
            refTexCoord.y = attr.as_float();
        }

        child         = child.next_sibling();
        extrPolMethod = static_cast<extrapolM>(child.text().as_int());

        child         = child.next_sibling();
        intrPolMethod = static_cast<interpolM>(child.text().as_int());

        child = child.next_sibling();
        id    = child.text().as_int();

        child    = child.next_sibling();
        iPolTime = child.text().as_float();

        child        = child.next_sibling();
        texBasePoint = child.text().as_bool();
        child        = child.next_sibling();

        if (!strcmp(child.name(), "SplineSegment")) {
            if (!seg) seg = std::make_unique<SplineSegment>();
            std::vector<std::string> attrNames = {"aX", "aY", "bX", "bY", "cX", "cY", "dX", "dY"};
            float                   *ptr       = &seg->a[0];
            for (auto &it : attrNames) {
                if ((attr = child.attribute(it.c_str()))) {
                    *ptr++ = attr.as_float();
                }
            }
        }
    }
};

}  // namespace ara

// wrapper for use with earcut tesselator

namespace mapbox::util {

template <>
struct nth<0, ara::CtrlPoint> {
    static auto get(const ara::CtrlPoint &t) { return t.position.x; };
};

template <>
struct nth<1, ara::CtrlPoint> {
    static auto get(const ara::CtrlPoint &t) { return t.position.y; };
};

template <>
struct nth<0, glm::vec2> {
    static auto get(const glm::vec2 &t) { return t.x; };
};

template <>
struct nth<1, glm::vec2> {
    static auto get(const glm::vec2 &t) { return t.y; };
};

} // namespace mapbox::util

