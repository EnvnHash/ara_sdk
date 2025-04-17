/*
 * geometry.h
 *
 *  Created on: 19.09.2016
 *      Author: sven
 */

#pragma once

#include "Vertex.h"

namespace ara::geometry {

template <class TVertex>
class Mesh {
public:
    std::vector<TVertex>    m_vertices;
    std::vector<glm::uvec3> m_indicesTriangles;
    std::vector<glm::uvec2> m_indicesOutline;

    void append(Mesh<TVertex> &geo) {
        m_vertices.reserve(geo.m_vertices.size() + m_vertices.size());
        m_indicesTriangles.reserve(geo.m_indicesTriangles.size() + m_indicesTriangles.size());
        m_indicesOutline.reserve(geo.m_indicesOutline.size() + m_indicesOutline.size());

        uint offset = glm::uint(m_vertices.size());

        for (size_t i = 0; i < geo.m_vertices.size(); i++) {
            m_vertices.push_back(geo.m_vertices[i]);
        }

        for (auto m_indicesTriangle : geo.m_indicesTriangles) {
            m_indicesTriangles.push_back(m_indicesTriangle + glm::uvec3(offset));
        }

        for (auto i : geo.m_indicesOutline) {
            m_indicesOutline.push_back(i + glm::uvec2(offset));
        }
    }

    void flipWinding() {
        for (auto &m_indicesTriangle : m_indicesTriangles) {
            std::swap(m_indicesTriangle.x, m_indicesTriangle.z);
        }
    }

    [[nodiscard]] size_t    getTriangleIndicesSize() const { return m_indicesTriangles.size() * sizeof(glm::uvec3); }
    [[nodiscard]] glm::uint getTriangleIndicesCount() const { return (glm::uint)m_indicesTriangles.size() * 3; }
    [[nodiscard]] size_t    getOutlineIndicesSize() const { return m_indicesOutline.size() * sizeof(glm::uvec2); }
    [[nodiscard]] glm::uint getOutlineIndicesCount() const { return (glm::uint)m_indicesOutline.size() * 2; }
    [[nodiscard]] size_t    getVerticesSize() const { return m_vertices.size() * sizeof(TVertex); }
    [[nodiscard]] glm::uint getVerticesCount() const { return (glm::uint)m_vertices.size(); }
};

template <class TVertex>
class Plane : public Mesh<TVertex> {
public:
    static void add(Mesh<TVertex> &geo, const glm::mat4 &mat, int w, int h) {
        int xdim = w;
        int ydim = h;

        float xmove = 1.0f / (float)xdim;
        float ymove = 1.0f / (float)ydim;

        int width = (xdim + 1);

        auto vertOffset = (glm::uint)geo.m_vertices.size();

        int x, y;
        for (y = 0; y < ydim + 1; y++) {
            for (x = 0; x < xdim + 1; x++) {
                float     xpos = ((float)x * xmove);
                float     ypos = ((float)y * ymove);
                glm::vec3 pos;
                glm::vec2 uv;
                glm::vec3 normal;
                glm::vec4 col(1.f);

                pos[0] = (xpos - 0.5f) * 2.0f;
                pos[1] = (ypos - 0.5f) * 2.0f;
                pos[2] = 0;

                uv[0] = xpos;
                uv[1] = ypos;

                normal[0] = 0.0f;
                normal[1] = 0.0f;
                normal[2] = 1.0f;

                Vertex vert = Vertex(pos, normal, uv, col);

                vert.position = mat * vert.position;
                vert.normal   = mat * vert.normal;
                geo.m_vertices.push_back(TVertex(vert));
            }
        }

        for (y = 0; y < ydim; y++) {
            for (x = 0; x < xdim; x++) {
                // upper tris
                geo.m_indicesTriangles.push_back(glm::uvec3((x) + (y + 1) * width + vertOffset,
                                                            (x) + (y)*width + vertOffset,
                                                            (x + 1) + (y + 1) * width + vertOffset));
                // lower tris
                geo.m_indicesTriangles.push_back(glm::uvec3((x + 1) + (y + 1) * width + vertOffset,
                                                            (x) + (y)*width + vertOffset,
                                                            (x + 1) + (y)*width + vertOffset));
            }
        }

        for (y = 0; y < ydim; y++) {
            geo.m_indicesOutline.push_back(glm::uvec2((y)*width + vertOffset, (y + 1) * width + vertOffset));
        }
        for (y = 0; y < ydim; y++) {
            geo.m_indicesOutline.push_back(
                glm::uvec2((y)*width + xdim + vertOffset, (y + 1) * width + xdim + vertOffset));
        }
        for (x = 0; x < xdim; x++) {
            geo.m_indicesOutline.push_back(glm::uvec2((x) + vertOffset, (x + 1) + vertOffset));
        }
        for (x = 0; x < xdim; x++) {
            geo.m_indicesOutline.push_back(
                glm::uvec2((x) + ydim * width + vertOffset, (x + 1) + ydim * width + vertOffset));
        }
    }

    explicit Plane(int segments = 1) { add(*this, glm::mat4(1), segments, segments); }
};

template <class TVertex>
class Box : public Mesh<TVertex> {
public:
    static void add(Mesh<TVertex> &geo, const glm::mat4 &mat, int w, int h, int d) {
        int configs[6][2] = {
            {w, h}, {w, h}, {d, h}, {d, h}, {w, d}, {w, d},
        };

        for (int side = 0; side < 6; side++) {
            glm::mat4 matrixRot(1);

            switch (side) {
                case 0: break;
                case 1: matrixRot = glm::rotate(glm::mat4(1.f), float(M_PI), glm::vec3(0.f, 1.f, 0.f)); break;
                case 2:
                    matrixRot = glm::rotate(glm::mat4(1.f), float(M_PI * 0.5f), glm::vec3(0.f, 1.f, 0.f));
                    ;
                    break;
                case 3: matrixRot = glm::rotate(glm::mat4(1.f), float(M_PI * 1.5f), glm::vec3(0.f, 1.f, 0.f)); break;
                case 4: matrixRot = glm::rotate(glm::mat4(1.f), float(M_PI * 0.5f), glm::vec3(1.f, 0.f, 0.f)); break;
                case 5: matrixRot = glm::rotate(glm::mat4(1.f), float(M_PI * 1.5f), glm::vec3(1.f, 0.f, 0.f)); break;
            }

            glm::mat4 matrixMove = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, 1.0f));
            //          glm::mat4 matrixMove =
            //          nv_math::translation_mat4(0.0f,0.0f,1.0f);

            Plane<TVertex>::add(geo, mat * matrixRot * matrixMove, configs[side][0], configs[side][1]);
        }
    }

    explicit Box(int segments = 1) { add(*this, glm::mat4(1), segments, segments, segments); }
};

template <class TVertex>
class Sphere : public Mesh<TVertex> {
public:
    static void add(Mesh<TVertex> &geo, const glm::mat4 &mat, int w, int h) {
        int xydim = w;
        int zdim  = h;

        auto vertOffset = (glm::uint)geo.m_vertices.size();

        float xyshift = 1.0f / (float)xydim;
        float zshift  = 1.0f / (float)zdim;
        int   width   = xydim + 1;

        int xy, z;
        for (z = 0; z < zdim + 1; z++) {
            for (xy = 0; xy < xydim + 1; xy++) {
                glm::vec3 pos;
                glm::vec3 normal;
                glm::vec2 uv;
                glm::vec4 col = glm::vec4(1.f);

                float curxy   = xyshift * (float)xy;
                float curz    = zshift * (float)z;
                float anglexy = curxy * M_PI * 2.0f;
                float anglez  = (1.0f - curz) * M_PI;
                pos[0]        = cosf(anglexy) * sinf(anglez);
                pos[1]        = sinf(anglexy) * sinf(anglez);
                pos[2]        = cosf(anglez);
                normal        = pos;
                uv[0]         = curxy;
                uv[1]         = curz;

                Vertex vert   = Vertex(pos, normal, uv, col);
                vert.position = mat * vert.position;
                vert.normal   = mat * vert.normal;

                geo.m_vertices.push_back(TVertex(vert));
            }
        }

        int vertex = 0;
        for (z = 0; z < zdim; z++) {
            for (xy = 0; xy < xydim; xy++, vertex++) {
                glm::uvec3 indices;
                if (z != zdim - 1) {
                    indices[2] = vertex + vertOffset;
                    indices[1] = vertex + width + vertOffset;
                    indices[0] = vertex + width + 1 + vertOffset;
                    geo.m_indicesTriangles.push_back(indices);
                }

                if (z != 0) {
                    indices[2] = vertex + width + 1 + vertOffset;
                    indices[1] = vertex + 1 + vertOffset;
                    indices[0] = vertex + vertOffset;
                    geo.m_indicesTriangles.push_back(indices);
                }
            }
            vertex++;
        }

        int middlez = zdim / 2;

        for (xy = 0; xy < xydim; xy++) {
            glm::uvec2 indices;
            indices[0] = middlez * width + xy + vertOffset;
            indices[1] = middlez * width + xy + 1 + vertOffset;
            geo.m_indicesOutline.push_back(indices);
        }

        for (int i = 0; i < 4; i++) {
            int x = (xydim * i) / 4;
            for (z = 0; z < zdim; z++) {
                glm::uvec2 indices;
                indices[0] = x + width * (z) + vertOffset;
                indices[1] = x + width * (z + 1) + vertOffset;
                geo.m_indicesOutline.push_back(indices);
            }
        }
    }

    explicit Sphere(int w = 16, int h = 8) { add(*this, glm::mat4(1), w, h); }
};
}  // namespace ara::geometry
