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
        auto vertOffset = static_cast<uint>(geo.m_vertices.size());

        // Create vertices
        std::vector<Vertex> vertices( (ydim + 1) * (xdim + 1)); // Pre-allocate for efficiency
        for (int y = 0; y < ydim + 1; ++y) {
            for (int x = 0; x < xdim + 1; ++x) {
                float xpos = ((float)x * xmove);
                float ypos = ((float)y * ymove);

                vec3 pos((xpos - 0.5f) * 2.0f, (ypos - 0.5f) * 2.0f, 0);
                vec2 uv(xpos, ypos);
                vec3 normal(0.0f, 0.0f, 1.0f);
                vec4 col(1.f);

                Vertex vert = Vertex(pos, normal, uv, col);
                vert.position = mat * vert.position;
                vert.normal   = mat * vert.normal;

                vertices[x + y * width] = TVertex(vert); // Store in pre-allocated vector
            }
        }
        geo.m_vertices.insert(geo.m_vertices.end(), vertices.begin(), vertices.end());


        // Create triangles
        std::vector<glm::uvec3> triangleIndices;
        for (int y = 0; y < ydim; ++y) {
            for (int x = 0; x < xdim; ++x) {
                int topLeft     = (x) + (y + 1) * width + vertOffset;
                int bottomLeft  = (x) + (y)*width + vertOffset;
                int topRight    = (x + 1) + (y + 1) * width + vertOffset;
                int bottomRight = (x + 1) + (y)*width + vertOffset;

                triangleIndices.emplace_back(topLeft, bottomLeft, topRight);
                triangleIndices.emplace_back(topRight, bottomLeft, bottomRight);
            }
        }
        geo.m_indicesTriangles.insert(geo.m_indicesTriangles.end(), triangleIndices.begin(), triangleIndices.end());

        // Create outline indices
        std::vector<glm::uvec2> outlineIndices;
        for (int y = 0; y < ydim; ++y) {
            outlineIndices.emplace_back((y)*width + vertOffset, (y + 1) * width + vertOffset);
        }
        for (int y = 0; y < ydim; ++y) {
            outlineIndices.emplace_back((y)*width + xdim + vertOffset, (y + 1) * width + xdim + vertOffset);
        }
        for (int x = 0; x < xdim; ++x) {
            outlineIndices.emplace_back((x) + vertOffset, (x + 1) + vertOffset);
        }
        for (int x = 0; x < xdim; ++x) {
            outlineIndices.emplace_back((x) + ydim * width + vertOffset, (x + 1) + ydim * width + vertOffset);
        }

        geo.m_indicesOutline.insert(geo.m_indicesOutline.end(), outlineIndices.begin(), outlineIndices.end());
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

            auto matrixMove = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, 1.0f));
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

        auto vertOffset = static_cast<uint>(geo.m_vertices.size());

        float xyshift = 1.0f / static_cast<float>(xydim);
        float zshift  = 1.0f / static_cast<float>(zdim);
        int   width   = xydim + 1;

        // Create vertices
        std::vector<Vertex> vertices((xydim + 1) * (zdim + 1));
        for (int z = 0; z < zdim + 1; ++z) {
            for (int xy = 0; xy < xydim + 1; ++xy) {
                float curxy   = xyshift * static_cast<float>(xy);
                float curz    = zshift * static_cast<float>(z);
                float anglexy = curxy * M_PI * 2.0f;
                float anglez  = (1.0f - curz) * M_PI;

                vec3 pos(cosf(anglexy) * sinf(anglez), sinf(anglexy) * sinf(anglez), cosf(anglez));
                vec3 normal = pos;
                vec2 uv(curxy, curz);

                Vertex vert = Vertex(pos, normal, uv, vec4(1.f));
                vert.position = mat * vert.position;
                vert.normal   = mat * vert.normal;

                vertices[xy + z * width] = TVertex(vert);
            }
        }
        geo.m_vertices.insert(geo.m_vertices.end(), vertices.begin(), vertices.end());


        // Create triangles
        std::vector<glm::uvec3> triangleIndices;
        for (int z = 0; z < zdim; ++z) {
            for (int xy = 0; xy < xydim; ++xy) {
                int vertex = xy + z * width + vertOffset;

                // Calculate indices for the two triangles forming a quad.
                int nextX  = (xy + 1) % xydim; // Wrap around to handle last column
                int nextZ  = (z + 1) % zdim;   // Wrap around to handle last row

                int v0 = vertex;
                int v1 = xy + (z + 1) * width + vertOffset;
                int v2 = nextX + z * width + vertOffset;
                int v3 = nextX + (z + 1) * width + vertOffset;

                triangleIndices.emplace_back(v0, v1, v2);
                triangleIndices.emplace_back(v1, v3, v2);
            }
        }
        geo.m_indicesTriangles = triangleIndices;

        // Create outline indices (simplified)
        std::vector<glm::uvec2> outlineIndices;

        // Horizontal lines
        for (int z = 0; z < zdim + 1; ++z) {
            for (int x = 0; x < xydim; ++x) {
                outlineIndices.emplace_back(x + z * width + vertOffset, (x + 1) + z * width + vertOffset);
            }
        }

        // Vertical lines
        for (int x = 0; x < xydim + 1; ++x) {
            for (int z = 0; z < zdim; ++z) {
                outlineIndices.emplace_back(x + z * width + vertOffset, x + (z + 1) * width + vertOffset);
            }
        }

        geo.m_indicesOutline = outlineIndices;
    }

    explicit Sphere(int w = 16, int h = 8) { add(*this, glm::mat4(1), w, h); }
};

}  // namespace ara::geometry
