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


#include "GeoPrimitives/Polygon.h"

#include <Meshes/Mesh.h>

#include "Shaders/ShaderCollector.h"

using namespace std;
using namespace glm;

namespace ara {
Polygon::Polygon() : GeoPrimitive(), m_color({1.f, 1.f, 1.f, 1.f}), m_subDivFact(0.01f) {
    m_polygon.emplace_back();  // we need at least one main polygon
}

Polygon::Polygon(ShaderCollector *shCol)
    : GeoPrimitive(), m_shCol(shCol), m_color({1.f, 1.f, 1.f, 1.f}), m_subDivFact(0.01f) {
    m_polygon.emplace_back();  // we need at least one main polygon
}

void Polygon::draw(TFO *_tfo) {
    if (!m_vaoFilled.isInited() || !m_vaoFilled.getElementBuffer()) {
        return;
    }

    m_vaoFilled.enableVertexAttribs();
    m_vaoFilled.bindElementBuffer();

    glDrawElements(GL_TRIANGLES, static_cast<int>(m_indices.size()), GL_UNSIGNED_INT, nullptr);

    VAO::unbindElementBuffer();
    m_vaoFilled.disableVertexAttribs();
}

void Polygon::drawAsPatch() {
#ifndef ARA_USE_GLES31
    if (!m_vaoFilled.isInited()) return;
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    m_vaoFilled.enableVertexAttribs();
    glDrawArrays(GL_PATCHES, 0, static_cast<int>(m_indices.size()));
    m_vaoFilled.disableVertexAttribs();
#endif
}

/// ...work in progress
void Polygon::drawHighRes(float *m_pvm, GLuint texId, float fboWidth, float fboHeight, TFO *_tfo) {
#ifndef ARA_USE_GLES31
    if (!m_vaoFilled.isInited() || !m_tessShdr) {
        return;
    }

    // setting up a opengl tesselation for catmull-rom interpolation if set
    glPatchParameteri(GL_PATCH_VERTICES, 3);  // interpolation triangles

    m_tessShdr->begin();
    m_tessShdr->setUniformMatrix4fv("m_pvm", m_pvm);
    m_tessShdr->setUniform1i("maxDiff", static_cast<int>(getTotalNrPoints()) - 1);
    m_tessShdr->setUniform2f("fboSize", fboWidth, fboHeight);
    m_tessShdr->setUniform1f("segsPerPixel", 0.05f);  // every 5 pixels a segment
    m_tessShdr->setUniform1i("tex", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    // m_vaoFilled.bind();
    // m_vaoFilled.enableVertexAttribs();
    m_vaoFilled.drawElements(GL_PATCHES, _tfo, GL_TRIANGLES, static_cast<int>(m_indices.size()), 0);

    Shaders::end();
#endif
}

void Polygon::drawOutline(GLenum drawMode, TFO *tfo) {
    if (!m_vaoFilled.isInited()) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vaoFilled.getVBO(CoordType::Position));
    glEnableVertexAttribArray(static_cast<int>(CoordType::Position));
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    // draw outlines of all polygon levels
    size_t offs = 0;
    for (auto &p : m_polygonInterpFlat) {
        glDrawArrays(drawMode, static_cast<GLint>(offs), static_cast<GLsizei>(p.size()));
        offs += p.size();
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(static_cast<int>(CoordType::Position));
}

// Note: tesselating non-interpolated polygon on CPU and do catmull-rom
// interpolation with Tesselation shaders on the gpu doesn't work, since there
// may be overlapping and flipped triangles. Here we do low-res interpolation on
// the CPU and use the following shader for generating a high-res on GPU
void Polygon::initTessShader() {
    if (!m_shCol) return;

    string vert = STRINGIFY(precise layout(location = 0) in vec4 position;\n
                                        layout(location = 2) in vec2 texCoord;\n
                                        out FS_TC { \n
                                        int ind; \n
                                        vec2 texCoord; \n
                                } vertOut; \n
                                        void main() { \n
                                        vertOut.ind = int(position.z); \n
                                        vertOut.texCoord = texCoord; \n
                                        gl_Position = vec4(position.xy, 0.f, 1.0); \n
                                });
    vert = "// polygon tesselation shader, vert\n" + m_shCol->getShaderHeader() + vert;

    string cont = STRINGIFY(layout(vertices = 3) out;\n
                                        in FS_TC { \n
                                        int ind; \n
                                        vec2 texCoord; \n
                                } vertIn[]; \n  // note although TC execution is per vertex, inputs provide access to all patches vertices
                                        \n
                                        out TC_TE { \n
                                        vec2 texCoord; \n
                                        int calcTex; \n
                                } vertOut[]; \n // note: per vertex, can only we indexed via gl_InvocationID, TC execution is per vertex
                                        \n
                                        uniform int maxDiff; \n
                                        uniform vec2 fboSize; \n
                                        uniform float segsPerPixel; \n
                                        \n
                                        void main() { \n
                                        gl_TessLevelInner[0] = 1.0;  \n// center

                                        // calculate amount of subdivision
                                        int pairDiff[3]; \n
                                        for (int i=0; i<3;i++) { \n
                                        pairDiff[i] = abs(vertIn[i].ind - vertIn[(i+1) %3].ind); \n
                                } \n
                                        // if this triangle side is not an edge, subdivide it
                                        for (int i=0; i<3;i++) { \n
                                        gl_TessLevelOuter[(i+2) %3] = pairDiff[i] != 1 ? 2.0 : 1.0;  \n// second point pair of the input patch
                                }\n
                                        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n
                                        vertOut[gl_InvocationID].texCoord = vertIn[gl_InvocationID].texCoord; \n
                                        vertOut[gl_InvocationID].calcTex = int(pairDiff[gl_InvocationID] != 1); \n
                                });
    cont = "// polygon tesselation shader, vert\n" + m_shCol->getShaderHeader() + cont;

    // tesselation evaluation shader deals with path vertices in form of
    // baycentric cordinates we take those as t for calculating our
    // interpolation. (P0,P1,P2 in order of vertex input / patch) [0], [1], [2]
    // are the respective gl_TessLevelOuter[] indices
    //              P2 (0,0,1)
    //               /\
    //          [1] /  \  [0]
    //             /____\
    //   P0 (1,0,0)  [2]  P1 (0,1,0)
    // if gl_TessCoord.x == 0.0 we are on [0], take z coord [2]
    // if gl_TessCoord.y == 0.0 we are on [1], take x coord [0]
    // if gl_TessCoord.z == 0.0 we are on [2], take y coord [1]

    string eval = STRINGIFY(layout(triangles, equal_spacing, ccw) in;
                                        in TC_TE{\n
                        vec2 texCoord; \n
                        int calcTex; \n
                } vertIn[]; \n  // access to all patches vertices from the TC stage, although TE is invocated per Vertex
                                        out TE_FS { \n
                                        vec2 texCoord; \n
                                } vertOut; \n
                                        uniform mat4 m_pvm;
                                        void main() {
                                        vec4 iPoint = gl_TessCoord.x * gl_in[0].gl_Position
                                        + gl_TessCoord.y * gl_in[1].gl_Position
                                        + gl_TessCoord.z * gl_in[2].gl_Position;
                                        vertOut.texCoord = gl_TessCoord.x * vertIn[0].texCoord
                                        + gl_TessCoord.y * vertIn[1].texCoord
                                        + gl_TessCoord.z * vertIn[2].texCoord;
                                        gl_Position = iPoint;
                                });
    eval = "// polygon tesselation shader, vert\n" + m_shCol->getShaderHeader() + eval;

    string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
                                        in TE_FS { \n
                                        vec2 texCoord; \n
                                } vertIn; \n  // access to all patches vertices from the TC stage, although TE is invocated per Vertex
                                        uniform sampler2D tex;\n
                                        uniform vec4 color;\n
                                        void main() { \n
                                        fragColor = texture(tex, vertIn.texCoord);
                                        // fragColor = vec4(1.0, 0.0, 0.0, 1.0);
                                });
    frag = "// polygon tesselation shader, vert\n" + m_shCol->getShaderHeader() + frag;

    if (m_shCol->hasShader("PolygonTessShader")) m_shCol->deleteShader("PolygonTessShader");

    m_tessShdr = m_shCol->add("PolygonTessShader", vert, cont, eval, std::string(), frag);
}

// Returns array of indices that refer to the vertices of the input polygon.
// e.g: the index 6 would refer to {25, 75} in this example.
// Three subsequent indices form a triangle. Output triangles are clockwise.
void Polygon::tesselate() {
    if (!m_tessShdr) initTessShader();

    if (m_polygon.empty() || (!m_polygon.empty() && m_polygon.begin()->empty())) {
        // if the polygon exits, but all points have been deleted, delete the
        // vaos
        if (!m_polygon.empty() && m_inited && m_vaoFilled.isInited()) m_vaoFilled.remove();

        if (!m_polygonInterpFlat.empty()) m_polygonInterpFlat.clear();

        return;
    }

    if (m_polygonInterpFlat.size() != m_polygon.size()) {
        m_polygonInterpFlat.resize(m_polygon.size());
        m_polygonInterpCount.resize(m_polygon.size());
    }

    m_polyIntrIt      = m_polygonInterpFlat.begin();
    m_polyIntrCountIt = m_polygonInterpCount.begin();

    // iterate through all levels of the polygon and calculate the catmull-rom coefficients
#pragma omp parallel for
    for (auto &pIt : m_polygon) {
        m_polyIntrNumPoints = 0;

        // make sure the array for counting the amount of points for each polygon points is big enough for this polygon level
        if (m_polyIntrCountIt->size() != pIt.size()) {
            m_polyIntrCountIt->resize(pIt.size());
        }

        auto pCountIt = m_polyIntrCountIt->begin();
        for (auto point = pIt.begin(); point != pIt.end(); ++point) {
            m_next = point == pIt.end() - 1 ? pIt.begin() : point + 1;

            if (point->intrPolMethod == interpolM::CatmullRomCentri ||
                m_next->intrPolMethod == interpolM::CatmullRomCentri) {
                auto pointSet = getCatmull4PointSeg(point, pIt);
                if (!point->seg) {
                    point->seg = make_shared<SplineSegment>();
                }
                catmullRomCentriCalcCoeff2(point->seg.get(), pointSet.get());

                // estimate a number of subdivision
                m_numSubDiv = std::max<size_t>(
                    1, static_cast<size_t>(glm::length(pointSet->at(2) - pointSet->at(1)) / m_subDivFact));

                *pCountIt = m_numSubDiv;
                m_polyIntrNumPoints += m_numSubDiv;
            } else {
                *pCountIt = 1;
                m_polyIntrNumPoints++;
            }

            ++pCountIt;
        }

        // make sure, that the destination vector for the interpolated polygon
        // has enough space
        if (m_polyIntrIt->size() != m_polyIntrNumPoints) {
            m_polyIntrIt->resize(m_polyIntrNumPoints);
        }

        ++m_polyIntrIt;
        ++m_polyIntrCountIt;
    }

    // now run again through segments and create and expanded pointset with
    // interpolated points we use two passes in order to know the final vector
    // size and thus being able to parallelize the computation
    m_polyIntrIt      = m_polygonInterpFlat.begin();
    m_polyIntrCountIt = m_polygonInterpCount.begin();

#pragma omp parallel for
    for (auto &pIt : m_polygon) {
        float  intr       = 0.f;
        size_t ind        = 0;
        auto   pointIp    = m_polyIntrIt->begin();
        auto   pointCount = m_polyIntrCountIt->begin();
        for (auto point = pIt.begin(); point != pIt.end(); ++point, ++ind) {
            m_next          = point == pIt.end() - 1 ? pIt.begin() : point + 1;
            m_mixedInterpol = point->intrPolMethod != m_next->intrPolMethod;

            if (point->intrPolMethod == interpolM::CatmullRomCentri ||
                m_next->intrPolMethod == interpolM::CatmullRomCentri) {
                // we already have a destination vector of the right size, so
                // iterate through it, straight forward
                for (size_t i = 0; i < *pointCount; i++) {
                    pointIp->intrPolMethod = interpolM::CatmullRomCentri;
                    pointIp->iPolTime      = static_cast<float>(i) / static_cast<float>(*pointCount);
                    pointIp->seg           = point->seg;  // copy for later use
                    pointIp->refTexCoord   = glm::mix(point->refTexCoord, m_next->refTexCoord, pointIp->iPolTime);

                    if (point->seg) catmullRomCentriGetPoint(point->seg.get(), pointIp->position, pointIp->iPolTime);

                    // in case one of the points between which we are
                    // interpolation is not cattmull rom, blend to towards
                    // linear interpolation in this direction
                    if (m_mixedInterpol) {
                        intr              = point->intrPolMethod == interpolM::CatmullRomCentri ? pointIp->iPolTime
                                                                                                : 1.f - pointIp->iPolTime;
                        pointIp->position = glm::mix(
                            pointIp->position, glm::mix(point->position, m_next->position, pointIp->iPolTime), intr);
                    }

                    // limit point to borders
                    pointIp->position = glm::max(glm::min(pointIp->position, vec2{1.f, 1.f}), vec2{-1.f, -1.f});

                    // checkNextLineOverlap(pointIp, point, pIt);

                    ++pointIp;
                }
            } else {
                // in case of no interpolation, just copy
                std::copy_n(&point->position[0], 8, &pointIp->position[0]);
                std::copy_n(&point->refTexCoord[0], 8, &pointIp->refTexCoord[0]);

                ++pointIp;
            }

            ++pointCount;
        }
        ++m_polyIntrIt;
        ++m_polyIntrCountIt;
    }

    // first pass: do tesselation ignoring interpolation modes, interpolating bilinearly
    m_indices = mapbox::earcut<GLuint>(m_polygonInterpFlat);

    // init the m_flatVert array, in case this has not already happened
    // we need: position, normals, texture coords and aux0 - 2 for catmull rom interpolation
    if (m_flatVert.empty()) {
        for (auto &it : m_flatVertCoordTypes) {
            m_flatVert[it].init(getCoTypeStdSize()[static_cast<int>(it)]);
        }
    }

    // init the VAO if not done
    if (!m_vaoFilled.isInited()) {
        string attribs;
        for (auto it = m_flatVertCoordTypes.begin(); it != m_flatVertCoordTypes.end(); ++it) {
            attribs += getStdAttribNames()[static_cast<int>(*it)];
            attribs += ":" + to_string(getCoTypeStdSize()[static_cast<int>(*it)]) + "f";
            if (it != (m_flatVertCoordTypes.end() - 1)) {
                attribs += ",";
            }
        }

        m_vaoFilled.init(attribs.c_str());
        m_vaoFilled.setStaticColor(m_color);
    }

    uint32_t nrVert = getTotalNrPoints();
    if (m_vaoFilled.getNrVertices() != 0 && m_vaoFilled.getNrVertices() < static_cast<int>(nrVert)) {
        m_vaoFilled.resize(nrVert);
    }

    // create a temporary array with CtrlPoints positions
    for (auto &it : m_flatVert) {
        it.second.resize(nrVert);
    }

    size_t ind = 0;  // global point index
    for (auto &pIt : m_polygonInterpFlat) {
        for (auto point = pIt.begin(); point != pIt.end(); ++point, ++ind) {
            // z coordinate to 0/1 defines if we are using catmull-rom or not
            m_flatVert[CoordType::Position].set(ind, vec4(point->position.x, point->position.y, 0.f, 1.f));
            m_flatVert[CoordType::Normal].set(ind, vec3(0.f, 0.f, 1.f));
            m_flatVert[CoordType::TexCoord].set(ind, point->refTexCoord);
        }
    }

    for (auto &it : m_flatVertCoordTypes) {
        m_vaoFilled.upload(it, m_flatVert[it].getPtr(), nrVert);
    }

    if (!m_indices.empty()) {
        m_vaoFilled.setElemIndices(static_cast<uint32_t>(m_indices.size()), &m_indices[0]);
    }

    if (nrVert && !m_inited) {
        m_inited = true;
    }
}

void Polygon::createInv(Polygon *poly) {
    if (!poly) {
        return;
    }

    m_polygon.clear();
    m_polygon.emplace_back();

    // create the first layer which is a normalized quad corresponding to the
    // borders
    auto p          = addPoint(0, -1.f, 1.f, 0.f, 1.f);
    p->texBasePoint = true;
    p               = addPoint(0, -1.f, -1.f, 0.f, 0.f);
    p->texBasePoint = true;
    p               = addPoint(0, 1.f, -1.f, 1.f, 0.f);
    p->texBasePoint = true;
    p               = addPoint(0, 1.f, 1.f, 1.f, 1.f);
    p->texBasePoint = true;

    // insert the poly argument as hole
    addHole(poly->getPoints(0));
}

// check if the interpolated line segments overlap with a preceding or following non-interpolated line segment
void Polygon::checkNextLineOverlap(const std::vector<CtrlPoint>::iterator& point, const std::vector<CtrlPoint>::iterator &base,
                                   std::vector<CtrlPoint> &polySeg) {
    constexpr size_t nrLinesToCheck = 2;
    vec2             linePoints[nrLinesToCheck][4];
    auto             ctrlPointIdx = static_cast<size_t>(base - polySeg.begin());

    // check overlap with following non-interpolated line-segment
    linePoints[0][0] = (polySeg.begin() + ((ctrlPointIdx + 1) % polySeg.size()))->position;
    linePoints[0][1] = (polySeg.begin() + ((ctrlPointIdx + 2) % polySeg.size()))->position;
    linePoints[0][2] = base->position;
    linePoints[0][3] = point->position;

    // check overlap with preceeding non-interpolated line-segment
    linePoints[1][0] = base->position;
    linePoints[1][1] = (polySeg.begin() + ((ctrlPointIdx - 1 + polySeg.size()) % polySeg.size()))->position;
    linePoints[1][2] = (polySeg.begin() + ((ctrlPointIdx + 1) % polySeg.size()))->position;
    linePoints[1][3] = point->position;

    for (auto &linePoint : linePoints) {
        pair<bool, vec2> intersLinePoint = lineIntersect(linePoint[0], linePoint[1], linePoint[2], linePoint[3]);
        if (intersLinePoint.first) {
            point->position = intersLinePoint.second;
        }
    }
}

void Polygon::checkCrossingLines(std::vector<CtrlPoint> *polySeg, const vector<CtrlPoint>::iterator& kv,
                                 glm::vec2 pointInitPos) {
    // create sets of three points defining all edges, this point is not part of
    size_t                                      polySize       = polySeg->size();
    size_t                                      nrLinesToCheck = polySize - 2;
    vector<vector<vector<CtrlPoint>::iterator>> linePoints;
    linePoints.resize(nrLinesToCheck);
    for (auto &it : linePoints) it.resize(2);
    vec2             initKvPos = kv->position;

    // run through all segments this point is not part of
    auto ctrlPointIdx = static_cast<size_t>(kv - polySeg->begin());
    for (size_t i = 0; i < nrLinesToCheck; i++, ctrlPointIdx++) {
        // calculate two line points
        for (size_t j = 0; j < 2; j++) {
            linePoints[i][j] = polySeg->begin() + ((ctrlPointIdx + 1 + j) % polySize);
        }
    }

    // check if one of the connecting lines of this Ctrlpoint is crossing with another line of the polygon
    vec2 connectingLines[2][2] = {{kv->position, linePoints[0][0]->position},
                                  {kv->position, linePoints[nrLinesToCheck - 1][1]->position}};
    vec2 connLines[2][2]       = {{initKvPos, linePoints[0][0]->position},
                                  {initKvPos, linePoints[nrLinesToCheck - 1][1]->position}};
    for (size_t i = 0; i < nrLinesToCheck; i++, ctrlPointIdx++) {
        for (size_t j = 0; j < 2; j++) {
            pair<bool, vec2> intersLinePoint = lineIntersect(connectingLines[j][0], connectingLines[j][1],
                                                             linePoints[i][0]->position,
                                                             linePoints[i][1]->position);

            if (intersLinePoint.first && glm::length(connectingLines[j][0] - intersLinePoint.second) > 0.0001f &&
                glm::length(connectingLines[j][1] - intersLinePoint.second) > 0.0001f) {
                // recalculate the intersection with the original and not the angle-limited CtrlPoint
                pair<bool, vec2> inters2 = lineIntersect(connLines[j][0], connLines[j][1], linePoints[i][0]->position,
                                                         linePoints[i][1]->position);
                kv->position             = (inters2.second - pointInitPos) * 0.99f + pointInitPos;
            }
        }
    }
}

unique_ptr<vector<vec2>> Polygon::getCatmull4PointSeg(const vector<CtrlPoint>::iterator &point, const vector<CtrlPoint> &polygon) {
    std::array pOffs = {-1, 1, 2};
    std::array pOffsIndx = {0, 0, 0};
    auto    out = make_unique<vector<vec2>>(4);

    std::array<vector<CtrlPoint>::const_iterator, 3> pOffsIt;

#pragma omp parallel for
    for (int i = 0; i < 3; i++) {
        pOffsIndx[i] = (static_cast<int>(point - polygon.begin()) + pOffs[i] + static_cast<int>(polygon.size())) % static_cast<int>(polygon.size());
        pOffsIt[i] = polygon.begin() + pOffsIndx[i];
    }

#pragma omp parallel for
    for (int i = 0; i < 3; i++) {
        extrpSeg(i, pOffsIt, point, polygon, pOffsIndx, out);
    }

    out->at(1).x = point->position.x;
    out->at(1).y = point->position.y;

    return out;
}

void Polygon::extrpSeg(int i, array<vector<CtrlPoint>::const_iterator, 3>& pOffsIt, const vector<CtrlPoint>::iterator &point,
                       const vector<CtrlPoint> &polygon, std::array<int, 3>& pOffsIndx, std::unique_ptr<vector<vec2>>& out) {
    bool p1IsCatM = point->intrPolMethod == interpolM::CatmullRomCentri;
    bool isCatM = pOffsIt[i]->intrPolMethod == interpolM::CatmullRomCentri;
    std::array outIdx = {0, 2, 3};

    // in case we are not at P0 or P3, take the found points straight away
    if (i == 1
        || isCatM
        || (i == 2 && pOffsIt[1]->intrPolMethod == interpolM::CatmullRomCentri)
        || (i == 0 && p1IsCatM))
    {
        out->at(outIdx[i]) = pOffsIt[i]->position;
    } else {
        // in case we are at P3 and this point doesn't use cattmull-rom,
        // extrapolate a new point by mirroring P1 on P2
        auto p2              = polygon.begin() + pOffsIndx[1];
        vec2 extrPolPoint = i == 2 ? (p2->position - point->position) + p2->position
                                    : (point->position - p2->position) + point->position;
        out->at(outIdx[i]) = extrPolPoint;
    }
}

// TODO: check for polygons with holes
void Polygon::updatePosVao(const map<uint32_t, CtrlPoint *> *pointMap) {
    if (m_vaoFilled.isInited()) {
        auto ptr = static_cast<vec4 *>(m_vaoFilled.getMapBuffer(CoordType::Position));
        for (auto &it : *pointMap) {
            ptr[it.first].x = it.second->position.x;
            ptr[it.first].y = it.second->position.y;
        }
        ara::VAO::unMapBuffer();
    }
}

void Polygon::setMainPolygon(const vector<CtrlPoint> &poly) {
    if (m_polygon[0].size() != poly.size()) {
        m_polygon[0].reserve(poly.size());
    }
    ranges::copy(poly, m_polygon[0].begin());
}

vector<CtrlPoint> *Polygon::addHole(vector<CtrlPoint> *poly) {
    if (!poly) {
        return nullptr;
    }

    m_polygon.emplace_back();
    m_polygon.back().insert(m_polygon.back().begin(), poly->begin(), poly->end());

    return &m_polygon.back();
}

CtrlPoint *Polygon::addPoint(size_t level, float x, float y) {
    if (m_polygon.size() > level) {
        m_polygon[level].emplace_back(x, y);
        return &m_polygon[level].back();
    } else {
        return nullptr;
    }
}

CtrlPoint *Polygon::addPoint(size_t level, float x, float y, float tex_x, float tex_y) {
    if (m_polygon.size() > level) {
        m_polygon[level].emplace_back(x, y, tex_x, tex_y);
        return &m_polygon[level].back();
    } else {
        return nullptr;
    }
}

CtrlPoint *Polygon::insertBeforePoint(size_t level, uint32_t idx, float x, float y) {
    if (m_polygon.size() > level && m_polygon[level].size() > idx) {
        m_polygon[level].insert(m_polygon[level].begin() + idx, CtrlPoint(x, y));
        return &m_polygon[level].back();
    } else {
        return nullptr;
    }
}

CtrlPoint *Polygon::insertBeforePoint(size_t level, uint32_t idx, float x, float y, float tex_x, float tex_y) {
    if (m_polygon.size() > level && m_polygon[level].size() > idx) {
        m_polygon[level].insert(m_polygon[level].begin() + idx, CtrlPoint(x, y, tex_x, tex_y));
        return &m_polygon[level].back();
    } else {
        return nullptr;
    }
}

void Polygon::deletePoint(size_t level, uint32_t idx) {
    if (m_polygon.size() > level && m_polygon[level].size() > idx) {
        m_polygon[level].erase(m_polygon[level].begin() + idx);
    }
}

CtrlPoint *Polygon::getPoint(size_t level, uint32_t idx) {
    if (m_polygon.size() > level && m_polygon[level].size() > idx) {
        return &m_polygon[level][idx];
    } else {
        return nullptr;
    }
}

vector<CtrlPoint> *Polygon::getPoints(size_t level) {
    if (m_polygon.size() > level) {
        return &m_polygon[level];
    } else {
        return nullptr;
    }
}

uint32_t Polygon::getTotalNrPoints() const {
    uint32_t nrPoints = 0;
    if (!m_polygonInterpFlat.empty()) {
        for (auto &poly : m_polygonInterpFlat) {
            nrPoints += static_cast<uint32_t>(poly.size());
        }
    } else {
        for (auto &poly : m_polygon) {
            nrPoints += static_cast<uint32_t>(poly.size());
        }
    }

    return nrPoints;
}

void Polygon::serializeToXml(pugi::xml_node &parent) {
    parent.append_attribute("nrShapes").set_value(m_polygon.size());
    for (auto &shp : m_polygon) {
        auto node = parent.append_child("Shape");
        node.append_attribute("nrPoints").set_value(shp.size());
        for (auto &point : shp) {
            auto ctrlpoint = node.append_child("CtrlPoint");
            point.serializeToXml(ctrlpoint);
        }
    }
}

void Polygon::parseFromXml(const pugi::xml_node &node) {
    pugi::xml_attribute attr;

    size_t nrShapes = 0;
    if ((attr = node.attribute("nrShapes"))) {
        nrShapes = static_cast<size_t>(attr.as_uint());
    }

    if (m_polygon.size() != nrShapes) {
        m_polygon.resize(nrShapes);
    }

    auto shpIt = m_polygon.begin();
    for (auto xmlShp = node.begin(); xmlShp != node.end(); ++xmlShp, ++shpIt) {
        size_t nrPoints = 0;
        if ((attr = xmlShp->attribute("nrPoints"))) {
            nrPoints = static_cast<size_t>(attr.as_uint());
        }

        if (shpIt->size() != nrPoints) {
            shpIt->resize(nrPoints);
        }

        auto pointIt = shpIt->begin();
        for (auto xmlPoint = xmlShp->begin(); xmlPoint != xmlShp->end(); ++xmlPoint, ++pointIt) {
            pointIt->parseFromXml(*xmlPoint);
        }
    }
}

}  // namespace ara
