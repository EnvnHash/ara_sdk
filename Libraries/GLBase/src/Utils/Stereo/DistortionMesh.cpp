//
// Created by sven on 18-10-22.
//

#include "DistortionMesh.h"

using namespace glm;
using namespace std;

namespace ara {

// Units of the following parameters are tan-angle units.
DistortionMesh::DistortionMesh(const PolynomialRadialDistortion &distortion,
                               glm::vec2 screen_size, glm::vec2 eye_offset_screen,
                               glm::vec2 texture_size, glm::vec2 eye_offset_texture) {
    m_mesh.init("position:2f,texCoord:2f");

    std::array<float, 2> p_texture{};
    std::array<float, 2> p_screen{};

    m_min = vec2{numeric_limits<float>::max(), numeric_limits<float>::max()};
    m_max = vec2{numeric_limits<float>::min(), numeric_limits<float>::min()};

    for (int row = 0; row < kResolution; row++) {
        for (int col = 0; col < kResolution; col++) {
            // Note that we warp the mesh vertices using the inverse of
            // the distortion function instead of warping the texture
            // coordinates by the distortion function so that the mesh
            // exactly covers the screen area that gets rendered to.
            // Helps avoid visible aliasing in the vignette.
            float u_texture = (static_cast<float>(col) / (kResolution - 1));
            float v_texture = (static_cast<float>(row) / (kResolution - 1));

            // texture position & radius relative to eye center in meters - I
            // believe this is tanangle
            p_texture[0] = u_texture * texture_size.x - eye_offset_texture.x;
            p_texture[1] = v_texture * texture_size.y - eye_offset_texture.y;

            p_screen = distortion.DistortInverse(p_texture);

            float u_screen = (p_screen[0] + eye_offset_screen.x) / screen_size.x;
            float v_screen = (p_screen[1] + eye_offset_screen.y) / screen_size.y;

            m_vertex_data.emplace_back(2.f * u_screen - 1.f, 2.f * v_screen - 1.f);

            for (int i = 0; i < 2; i++) {
                if (m_min[i] > m_vertex_data.back()[i]) m_min[i] = m_vertex_data.back()[i];
                if (m_max[i] < m_vertex_data.back()[i]) m_max[i] = m_vertex_data.back()[i];
            }

            m_uvs_data.emplace_back(u_texture, v_texture);
        }
    }

    //  LOG << "max: " << glm::to_string(m_max) << " min: " <<
    //  glm::to_string(m_min);

    // Strip method described at:
    // http://dan.lecocq.us/wordpress/2009/12/25/triangle-strip-for-grids-a-construction/
    //
    // For a grid with 4 rows and 4 columns of vertices, the strip would
    // look like:
    //
    //     0  -  1  -  2  -  3
    //     ↓  ↗  ↓  ↗  ↓  ↗  ↓
    //     4  -  5  -  6  -  7 ↺
    //     ↓  ↖  ↓  ↖  ↓  ↖  ↓
    //   ↻ 8  -  9  - 10  - 11
    //     ↓  ↗  ↓  ↗  ↓  ↗  ↓
    //    12  - 13  - 14  - 15
    //
    // Note the little circular arrows next to 7 and 8 that indicate
    // repeating that vertex once so as to produce degenerate triangles.
    /*
        // Number of indices:
        //   1 vertex per triangle
        //   2 triangles per quad
        //   (rows - 1) * (cols - 1) quads
        //   2 vertices at the start of each row for the first triangle
        //   1 extra vertex per row (except first and last) for a
        //     degenerate triangle
        const int n_indices = 2 * (kResolution - 1) * kResolution + (kResolution
       - 2); m_index_data.resize(n_indices); int index_offset = 0; int
       vertex_offset = 0; for (int row = 0; row < kResolution - 1; row++)
        {
            if (row > 0)
            {
                m_index_data[index_offset] = m_index_data[index_offset - 1];
                index_offset++;
            }

            for (int col = 0; col < kResolution; col++)
            {
                if (col > 0)
                {
                    if (row % 2 == 0) {
                        // Move right on even rows.
                        vertex_offset++;
                    } else {
                        // Move left on odd rows.
                        vertex_offset--;
                    }
                }

                m_index_data[index_offset++] = vertex_offset;
                m_index_data[index_offset++] = vertex_offset + kResolution;
            }

            vertex_offset = vertex_offset + kResolution;
        }

        m_mesh.push_back_positions(&m_vertex_data[0][0],
       m_vertex_data.size()*2); m_mesh.push_back_texCoords(&m_uvs_data[0][0],
       m_uvs_data.size()*2); m_mesh.push_back_indices(&m_index_data[0],
       m_index_data.size());*/
}

}  // namespace ara