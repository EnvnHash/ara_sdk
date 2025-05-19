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

//
//  assumes vertex to be in the m_format x, y, z
//  color: r m_g b a
//  normal: x y z
//  texCoord: x, y

#include <Meshes/Mesh.h>
#include <string_utils.h>

namespace ara {

Mesh::Mesh() {
    const char *_format = "position:3f,normal:3f,texCoord:2f,color:4f";
    init(_format);
}

Mesh::Mesh(const char *format) { init(format); }

void Mesh::init(const char *format) {
    m_entr_types.resize(toType(CoordType::Count));
    m_entr_types[toType(CoordType::Position)] = "position";
    m_entr_types[toType(CoordType::Normal)]   = "normal";
    m_entr_types[toType(CoordType::TexCoord)] = "texCoord";
    m_entr_types[toType(CoordType::Color)]    = "color";

    m_bUsing.resize(toType(CoordType::Count));
    for (int i = 0; i < toType(CoordType::Count); i++) {
        m_bUsing[i] = false;
    }

    m_allCoords.emplace_back(&m_positions);
    m_allCoords.emplace_back(&m_normals);
    m_allCoords.emplace_back(&m_texCoords);
    m_allCoords.emplace_back(&m_colors);

    // init coord sizes
    m_coordTypeSize.resize(toType(CoordType::Count));
    m_coordTypeSize[toType(CoordType::Position)] = 3;
    m_coordTypeSize[toType(CoordType::Normal)]   = 3;
    m_coordTypeSize[toType(CoordType::TexCoord)] = 2;
    m_coordTypeSize[toType(CoordType::Color)]    = 4;

    m_format    = std::string(format);
    auto coords = split(format, ',');

    for (auto &c : coords) {
        auto c_split = split(format, ':');
        if (c_split.size() == 2) {
            if (c_split[0] == "position") {
                m_coordTypeSize[toType(CoordType::Position)] = atoi(c_split[1].c_str());
            } else if (c_split[0] == "normal") {
                m_coordTypeSize[toType(CoordType::Normal)] = atoi(c_split[1].c_str());
            } else if (c_split[0] == "texCoord") {
                m_coordTypeSize[toType(CoordType::TexCoord)] = atoi(c_split[1].c_str());
            } else if (c_split[0] == "color") {
                m_coordTypeSize[toType(CoordType::Color)] = atoi(c_split[1].c_str());
            }
        }
    }
}

void Mesh::scale(glm::vec3 scale) {
    m_transfMatr = glm::scale(scale);
    doTransform(false);
}

void Mesh::rotate(float angle, float rotX, float rotY, float rotZ) {
    m_transfMatr = glm::rotate(angle, glm::vec3(rotX, rotY, rotZ));
    doTransform(true);
}

void Mesh::translate(glm::vec3 trans) {
    m_transfMatr = glm::translate(trans);
    doTransform(false);
}

void Mesh::doTransform(bool transfNormals) {
    if (!m_useIntrl) {
        for (int i = 0; i < static_cast<int>(m_positions.size()) / m_coordTypeSize[toType(CoordType::Position)]; i++) {
            glm::vec4 res =
                m_transfMatr * glm::vec4(m_positions[i * 3], m_positions[i * 3 + 1], m_positions[i * 3 + 2], 1.0f);
            m_positions[i * 3]     = res.r;
            m_positions[i * 3 + 1] = res.g;
            m_positions[i * 3 + 2] = res.b;

            if (static_cast<int>(m_normals.size()) != 0 && transfNormals) {
                glm::vec3 nor =
                    glm::mat3(m_transfMatr) * glm::vec3(m_normals[i * 3], m_normals[i * 3 + 1], m_normals[i * 3 + 2]);
                m_normals[i * 3]     = nor.r;
                m_normals[i * 3 + 1] = nor.g;
                m_normals[i * 3 + 2] = nor.b;
            }
        }
    } else {
        LOGE << "Mesh Error: can´t transform interleaved mesh!";
    }
}

void Mesh::invertNormals() {
    if (static_cast<int>(m_normals.size()) != 0) {
        for (int i = 0; i < static_cast<int>(m_positions.size()) / m_coordTypeSize[toType(CoordType::Position)]; i++) {
            m_normals[i * 3] *= -1.0f;
            m_normals[i * 3 + 1] *= -1.0f;
            m_normals[i * 3 + 2] = -1.0f;
        }
    }
}

// calculate normals assuming triangles
// should happen before transforming
void Mesh::calcNormals() {
    int m    = m_coordTypeSize[toType(CoordType::Position)] * 3;
    int offs = m_coordTypeSize[toType(CoordType::Position)];

    if (!m_useIntrl) {
        // takes two following positions, calulates the cross product
        // and set the result for all positions of the specific triangle
        for (uint i = 0; i < m_positions.size() / m; i++) {
            int       ind1 = i * m + offs;
            int       ind2 = i * m + 2 * offs;
            glm::vec3 side1 =
                glm::vec3(m_positions[ind1] - m_positions[i * m], m_positions[ind1 + 1] - m_positions[i * m + 1],
                          m_positions[ind1 + 2] - m_positions[i * m + 2]);

            glm::vec3 side2 =
                glm::vec3(m_positions[ind2] - m_positions[i * m], m_positions[ind2 + 1] - m_positions[i * m + 1],
                          m_positions[ind2 + 2] - m_positions[i * m + 2]);

            glm::vec3 normal = glm::cross(side2, side1);
            normal           = glm::normalize(normal);

            // run through triangle
            for (int j = 0; j < 3; j++) {
                if (static_cast<uint>(m_normals.size()) <
                    (((i * 3) + j + 1) * m_coordTypeSize[toType(CoordType::Normal)])) {
                    m_normals.emplace_back(normal.x);
                    m_normals.push_back(normal.y);
                    m_normals.push_back(normal.z);
                } else {
                    m_normals[(i * m_coordTypeSize[toType(CoordType::Normal)] * 3) +
                              (m_coordTypeSize[toType(CoordType::Normal)] * j)]     = normal.x;
                    m_normals[(i * m_coordTypeSize[toType(CoordType::Normal)] * 3) +
                              (m_coordTypeSize[toType(CoordType::Normal)] * j) + 1] = normal.y;
                    m_normals[(i * m_coordTypeSize[toType(CoordType::Normal)] * 3) +
                              (m_coordTypeSize[toType(CoordType::Normal)] * j) + 2] = normal.z;
                }
            }
        }
    }
    if (!m_bUsing[toType(CoordType::Normal)]) {
        m_bUsing[toType(CoordType::Normal)] = true;
        m_usedCoordTypes.push_back(CoordType::Normal);
    }
}

// assuming triangles
// take two positions, checks where they connect,
// calculates two normals, corresponding to the two triangles
// takes the medium of the two
// NOT TESTED!!!
void Mesh::calcSmoothNormals() {
    /*
     if ( !m_useIntrl )
     {
     // run through all triangles
     for (int i=1;i<(m_positions.size()/9)+1;i++)
     {
     // take each point
     for (int j=0;j<3;j++)
     {
     // look for a connecting point in the rest of the triangles
     bool found = false; int triCoordType::Count = 0; int vertCoordType::Count =
     0; int actInd = ((i+1)*9 +j) % m_positions.size(); int actInd2;

     while (!found && triCoordType::Count < (int)(m_positions.size()/9 - 1))
     {
     // check each one of the three m_positions in the triangle
     for (int k=0;k<3;k++)
     {
     int ind = ( ((i+1+triCoordType::Count)*9) +(k*3) ) % m_positions.size();

     if (m_positions[actInd] == m_positions[ind]
     && m_positions[actInd+1] == m_positions[ind+1]
     && m_positions[actInd+2] == m_positions[ind+2]
     )
     {
     vertCoordType::Count = k;
     found = true;
     }
     }
     }

     glm::vec3 normal;

     // if there was a connecting point
     if (found)
     {
     actInd2 = ( (i+1) *9 + ((j+1) %3) ) % m_positions.size();
     // calc two normals corresponding to the two triangles
     normal = glm::cross(glm::vec3(m_positions[actInd*9],
     m_positions[actInd*9+1], m_positions[actInd*9+2]),
     glm::vec3(m_positions[actInd2*9], m_positions[actInd2*9+1],
     m_positions[actInd2*9+2])); actInd = ((triCoordType::Count *9) +
     vertCoordType::Count) % m_positions.size(); actInd2 = (
     (triCoordType::Count *9) + (vertCoordType::Count+1 % 3) ) %
     m_positions.size(); glm::vec3 normal2 =
     glm::cross(glm::vec3(m_positions[actInd*9], m_positions[actInd*9+1],
     m_positions[actInd*9+2]), glm::vec3(m_positions[actInd2*9],
     m_positions[actInd2*9+1], m_positions[actInd2*9+2])); normal =
     glm::vec3((normal.x + normal2.x) * 0.5, (normal.y + normal2.y) * 0.5,
     (normal.z + normal2.z) * 0.5);
     normal = glm::normalize(normal);

     } else
     {
     actInd = ((i+1)*9 +j) % m_positions.size();
     actInd2 = ((i+1)*9 + ((j+1) %3)) % m_positions.size();

     // if there was no connecting point, take the normal relative to the actual
     triangle normal = glm::cross(glm::vec3(m_positions[actInd*9],
     m_positions[actInd*9+1], m_positions[actInd*9+2]),
     glm::vec3(m_positions[actInd2*9], m_positions[actInd2*9+1],
     m_positions[actInd2*9+2])); normal = glm::normalize(normal);
     }

     // add or replace them into the normals array
     actInd = ((i+1)*9 +j) % m_positions.size();
     for (int k=0;k<3;k++)
     {
     if ( normals.size() < actInd )
     {
     normals.push_back(normal.x);
     normals.push_back(normal.y);
     normals.push_back(normal.z);
     } else {
     normals[actInd] = normal.x;
     normals[actInd+1] = normal.y;
     normals[actInd+2] = normal.z;
     }
     }
     }
     }
     }
     if (!m_bUsing[NORMAL])
     {
     m_bUsing[NORMAL] = true;
     usedCoordTypes.push_back(NORMAL);
     }
     */
}

// should happen before transforming
void Mesh::genTexCoord(texCordGenType _type) {
    // loop through the positions
    switch (_type) {
        case texCordGenType::PlaneProjection:

            // get the normal of the first triangle
            glm::vec3 normal = glm::cross(glm::vec3(m_positions[0], m_positions[1], m_positions[2]),
                                          glm::vec3(m_positions[3], m_positions[4], m_positions[5]));
            //                LOG << ("bfore:  " <<glm::to_string(
            //                glm::vec3(m_positions[0], m_positions[1],
            //                m_positions[2]) ) ); LOG << ("bfore:  "
            //                <<glm::to_string( glm::vec3(m_positions[3],
            //                m_positions[4], m_positions[5]) ) ); LOG <<
            //                ("bfore:  " << glm::to_string(normal) );
            normal = glm::normalize(normal);
            //                LOG << (glm::to_string(normal) );

            // normal of x,y plane
            glm::vec3 dstNormal = glm::vec3(0.0f, 0.0f, 1.0f);

            // get a rotation matrix to rotate the plane into the x,y plane
            glm::quat rotQuat = RotationBetweenVectors(normal, dstNormal);

            // convert it to a rotation matrix
            glm::mat4 rotMat = glm::mat4_cast(rotQuat);

            std::vector<glm::vec4> rotVert;

            // get the borders of the transformed plane
            // and save the transformed coordinates
            GLfloat minX = 10000.0f, maxX = -10000.0f, minY = 10000.0f, maxY = -10000.0f;
            for (uint i = 0; i < static_cast<uint>(m_positions.size()) / m_coordTypeSize[toType(CoordType::Position)];
                 i++) {
                glm::vec4 inV    = glm::vec4(m_positions[i * m_coordTypeSize[toType(CoordType::Position)]],
                                             m_positions[i * m_coordTypeSize[toType(CoordType::Position)] + 1],
                                             m_positions[i * m_coordTypeSize[toType(CoordType::Position)] + 2], 1.0);
                glm::vec4 transV = rotMat * inV;

                rotVert.push_back(transV);
                if (transV.x > maxX) maxX = transV.x;
                if (transV.y > maxY) maxY = transV.y;
                if (transV.x < minX) minX = transV.x;
                if (transV.y < minY) minY = transV.y;
            }

            // set texCoords
            for (uint i = 0; i < static_cast<uint>(m_positions.size()) / m_coordTypeSize[toType(CoordType::Position)];
                 i++) {
                GLfloat tex[] = {(rotVert[i].x - minX) / (maxX - minX), (rotVert[i].y - minY) / (maxY - minY)};

                if (static_cast<uint>(m_texCoords.size()) < ((i + 1) * m_coordTypeSize[toType(CoordType::TexCoord)])) {
                    push_back_any(CoordType::TexCoord, tex, m_coordTypeSize[toType(CoordType::TexCoord)]);
                } else {
                    for (int j = 0; j < m_coordTypeSize[toType(CoordType::TexCoord)]; j++)
                        m_texCoords[(i * m_coordTypeSize[toType(CoordType::TexCoord)]) + j] = tex[j];
                }
            }

            // get the convex hull
            //            std::vector<vector<cv::Point> > hull(
            //            std::vectors.size() );

            break;
    }
}

//===============================================================================================

void Mesh::push_back(GLfloat *coords, int count) {
    if ((int)m_positions.size() == 0 && (int)m_normals.size() == 0 && (int)m_colors.size() == 0 &&
        (int)m_texCoords.size() == 0 && ((int)m_indices.size() == 0 || (int)m_indicesUint.size() == 0)) {
        for (int i = 0; i < count; i++) m_interleaved.push_back(coords[i]);
        if (!m_useIntrl) m_useIntrl = true;
    } else {
        LOGE << "Mesh::push_back_positions Error: mixing with interleave mode, "
                "please reset the m_mesh before changing modes";
    }
}

void Mesh::push_back_any(CoordType t, GLfloat *positions, int count) {
    if (!m_useIntrl) {
        for (int i = 0; i < count; i++) {
            m_allCoords[toType(t)]->emplace_back(positions[i]);
        }

        if (!m_bUsing[toType(t)]) {
            m_bUsing[toType(t)] = true;
            m_usedCoordTypes.emplace_back(t);
        }
    } else {
        LOGE << "Mesh::push_back_positions Error: mixing with interleave mode, please reset the m_mesh before changing modes";
    }
}

glm::vec3 Mesh::getVec3(CoordType t, int ind) {
    if (m_bUsing[toType(t)]) {
        if (!m_useIntrl &&
            static_cast<uint>(m_allCoords[toType(t)]->size()) > static_cast<uint>(ind * m_coordTypeSize[toType(t)])) {
            return {m_allCoords[toType(t)]->at(m_coordTypeSize[toType(t)] * ind),
                    m_allCoords[toType(t)]->at(m_coordTypeSize[toType(t)] * ind + 1),
                    m_allCoords[toType(t)]->at(m_coordTypeSize[toType(t)] * ind + 2)};

        } else
            LOGE << "Mesh::getVec3 Error: using interleave mode";

    } else
        LOGE << "Mesh::getVec3 Error: CoordType not found";

    return glm::vec3(0.f);
}

void Mesh::resize(CoordType t, uint size) {
    if (!m_useIntrl) {
        if (!m_bUsing[toType(t)]) {
            m_bUsing[toType(t)] = true;
            m_usedCoordTypes.push_back(t);
        }

        // uint actSize = m_allCoords[t]->size() / coordTypeSize[t];
        m_allCoords[toType(t)]->resize(size * m_coordTypeSize[toType(t)], 0.f);

        // if new size bigger than old size init with zeros
        //        	if(size > actSize)
        //        		std::fill(m_allCoords[t]->begin() + actSize *
        //        m_coordTypeSize[t], m_allCoords[t]->end(), 0.f);

    } else
        LOGE << "Mesh::resize Error: using interleave mode";
}

void *Mesh::getPtrInterleaved() {
    if (!m_useIntrl) {
        // printf("build interleaved \n");
        m_interleaved.clear();
        for (int i = 0; i < getNrPositions(); i++)
            for (int j = 0; j < toType(CoordType::Count); j++)
                if (!m_allCoords[j]->empty())
                    for (int k = 0; k < m_coordTypeSize[j]; k++)
                        m_interleaved.push_back(m_allCoords[j]->at(i * m_coordTypeSize[j] + k));
    }
    return &m_interleaved.front();
}

int Mesh::getTotalByteSize() {
    if (!m_useIntrl) {
        int size = 0;
        for (auto &m_usedCoordType : m_usedCoordTypes)
            size += (int)m_allCoords[toType(m_usedCoordType)]->size() * sizeof(GL_FLOAT);
        return size;
    } else
        return static_cast<int>(m_interleaved.size()) * sizeof(GL_FLOAT);
}

void Mesh::setVec3(CoordType t, uint ind, glm::vec3 _vec) {
    int idx = toType(t);
    if (m_bUsing[idx]) {
        if (!m_useIntrl && static_cast<uint>(m_allCoords[idx]->size()) >= ind * 3) {
            m_allCoords[idx]->at(m_coordTypeSize[idx] * ind)     = _vec.x;
            m_allCoords[idx]->at(m_coordTypeSize[idx] * ind + 1) = _vec.x;
            m_allCoords[idx]->at(m_coordTypeSize[idx] * ind + 2) = _vec.x;

        } else
            LOGE << "Mesh::getVec3 Error: using interleave mode";
    } else
        LOGE << "Mesh::getVec3 Error: CoordType not found";
}

void Mesh::setStaticColor(float _r, float _g, float _b, float _a) {
    if (static_cast<int>(m_statColor.size()) == 0) {
        m_statColor.push_back(_r);
        m_statColor.push_back(_g);
        m_statColor.push_back(_b);
        m_statColor.push_back(_a);
    } else {
        m_statColor[0] = _r;
        m_statColor[1] = _g;
        m_statColor[2] = _b;
        m_statColor[3] = _a;
    }
}

void Mesh::setStaticNormal(glm::vec3 norm) {
    if (static_cast<int>(m_statNormal.size()) == 0) {
        m_statNormal.push_back(norm.x);
        m_statNormal.push_back(norm.y);
        m_statNormal.push_back(norm.z);
    } else {
        m_statNormal[0] = norm.x;
        m_statNormal[1] = norm.y;
        m_statNormal[2] = norm.z;
    }
}

void Mesh::dumpInterleaved() {
    int count = 0;
    for (float &it : m_interleaved) {
        LOGE << count << " " << it;
        count++;
    }
}

}  // namespace ara
