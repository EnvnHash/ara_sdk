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

#include <Utils/VAO.h>

#include "GeoPrimitives/CtrlPoint.h"
#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

class Shaders;
class ShaderCollector;

class CoordVariantAr {
public:
    void resize(size_t newSize) {
        switch (m_vec.index()) {
            case 0: std::get<std::vector<glm::vec2>>(m_vec).resize(newSize); break;
            case 1: std::get<std::vector<glm::vec3>>(m_vec).resize(newSize); break;
            case 2: std::get<std::vector<glm::vec4>>(m_vec).resize(newSize); break;
            default: break;
        }
    }

    void init(int nrCoords) {
        switch (nrCoords) {
            case 2: m_vec = std::vector<glm::vec2>(); break;
            case 3: m_vec = std::vector<glm::vec3>(); break;
            case 4: m_vec = std::vector<glm::vec4>(); break;
            default: break;
        }
    }

    template <typename T>
    void set(size_t idx, T val) {
        std::get<std::vector<T>>(m_vec).at(idx) = val;
    }

    template <typename T>
    T get(size_t idx) {
        return std::get<std::vector<T>>(m_vec).at(idx);
    }

    float *getPtr() {
        switch (m_vec.index()) {
            case 0: return &std::get<std::vector<glm::vec2>>(m_vec).at(0)[0];
            case 1: return &std::get<std::vector<glm::vec3>>(m_vec).at(0)[0];
            case 2: return &std::get<std::vector<glm::vec4>>(m_vec).at(0)[0];
            default: return nullptr;
        }
    }

    std::variant<std::vector<glm::vec2>, std::vector<glm::vec3>, std::vector<glm::vec4>> m_vec;
};

class Polygon : public GeoPrimitive {
public:
    Polygon();
    explicit Polygon(ShaderCollector *shCol);

    void draw(TFO *_tfo = nullptr) override;
    void drawAsPatch();
    void drawHighRes(float *m_pvm, GLuint texId, float fboWidth, float fboHeight, TFO *tfo = nullptr);
    void drawOutline(GLenum drawMode = GL_LINE_LOOP, TFO *_tfo = nullptr);
    void init() override { m_polygon.emplace_back(); }  // we need at least one main polygon
    void initTessShader();
    void tesselate();
    void createInv(Polygon *poly);

    static void checkNextLineOverlap(const std::vector<CtrlPoint>::iterator& point, const std::vector<CtrlPoint>::iterator& base,
                                     std::vector<CtrlPoint> &polySeg);
    static void checkCrossingLines(std::vector<CtrlPoint> *polySeg, const std::vector<CtrlPoint>::iterator& kv,
                                   glm::vec2 point);

    static std::unique_ptr<std::vector<glm::vec2>> getCatmull4PointSeg(const std::vector<CtrlPoint>::iterator &point,
                                                                       const std::vector<CtrlPoint>           &polygon);
    static void extrpSeg(int i, std::array<std::vector<CtrlPoint>::const_iterator, 3>& pOffsIt,
                        const std::vector<CtrlPoint>::iterator &point, const std::vector<CtrlPoint> &polygon,
                        std::array<int, 3>& pOffsIndx, std::unique_ptr<std::vector<glm::vec2>>& out);
    void updatePosVao(const std::map<uint32_t, CtrlPoint *> *pointMap);
    void setMainPolygon(const std::vector<CtrlPoint> &poly);

    std::vector<CtrlPoint> *addHole() {
        m_polygon.emplace_back();
        return &m_polygon.back();
    }

    std::vector<CtrlPoint> *addHole(std::vector<CtrlPoint> *poly);
    CtrlPoint              *addPoint(size_t level, float x, float y);
    CtrlPoint              *addPoint(size_t level, float x, float y, float tex_x, float tex_y);
    CtrlPoint              *insertBeforePoint(size_t level, uint32_t idx, float x, float y);
    CtrlPoint              *insertBeforePoint(size_t level, uint32_t idx, float x, float y, float tex_x, float tex_y);
    void                    deletePoint(size_t level, uint32_t idx);
    CtrlPoint              *getPoint(size_t level, uint32_t idx);
    std::vector<CtrlPoint> *getPoints(size_t level);
    uint32_t                getTotalNrPoints() const;

    void deleteHole(size_t idx) {
        if (m_polygon.size() > idx) {
            m_polygon.erase(m_polygon.begin() + idx);
        }
    }
    std::vector<CtrlPoint>::iterator begin(size_t level) { return m_polygon[level].begin(); }
    std::vector<CtrlPoint>::iterator end(size_t level) { return m_polygon[level].end(); }
    void                             clear(size_t level = 0) { m_polygon[level].clear(); }
    bool                             isInited() const { return m_inited; }
    VAO                             &getVao() { return m_vaoFilled; }
    void                             setShaderCollector(ShaderCollector *shCol) { m_shCol = shCol; }

    void setColor(glm::vec4 col) override {
        m_color = col;
        if (m_vaoFilled.isInited()) {
            m_vaoFilled.setStaticColor(col);
        }
    }

    void serializeToXml(pugi::xml_node &parent);
    void parseFromXml(const pugi::xml_node &node);

    template <class B>
    void serialize(B &buf) const {
        buf << m_polygon;
        buf << m_color.r;
        buf << m_color.g;
        buf << m_color.b;
        buf << m_color.a;
    }

    template <class B>
    void parse(B &buf) {
        buf >> m_polygon;
        buf >> m_color.r;
        buf >> m_color.g;
        buf >> m_color.b;
        buf >> m_color.a;
    }

private:
    ShaderCollector                    *m_shCol    = nullptr;
    glm::mat4                           m_modelMat = glm::mat4(1.f);
    std::vector<std::vector<CtrlPoint>> m_polygon;
    std::vector<std::vector<CtrlPoint>> m_polygonInterpFlat;
    std::vector<std::vector<size_t>>    m_polygonInterpCount;
    VAO                                 m_vaoFilled;
    Shaders                            *m_tessShdr = nullptr;

    std::map<CoordType, CoordVariantAr> m_flatVert;
    std::vector<CoordType> m_flatVertCoordTypes{CoordType::Position, CoordType::Normal, CoordType::TexCoord};
    std::vector<GLuint>    m_indices;

    glm::vec4 m_color       = { 1.f, 1.f, 1.f, 1.f };
    float m_subDivFact      = 0.01f;
    bool m_inited           = false;
    bool m_mixedInterpol    = false;

    // temporary variables made local for performance reasons
    std::vector<std::vector<CtrlPoint>>::iterator m_polyIntrIt;
    std::vector<CtrlPoint>::iterator              m_next;
    std::vector<std::vector<size_t>>::iterator    m_polyIntrCountIt;
    size_t                                        m_polyIntrNumPoints = 0;
    size_t                                        m_numSubDiv         = 0;
};

}  // namespace ara
