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

#include "Assimp/AssimpExport.h"

namespace ara {

class Shaders;
class ShaderCollector;
class VAO;

class SurfaceGenerator {
public:
    enum surfaceType { FLAT = 0, CYLINDER, DOME, PANADOME };

    SurfaceGenerator();

    virtual void init(surfaceType type, ShaderCollector *shCol);

    void resize();
    void generateVert() const;
    void generateElemInd() const;
    void exportCollada(const std::string& path, const std::string& formatId);
    void setTypeName(const std::string& typeName);

    void setUseGL32Fallback(bool val) { m_useGL32Fallback = val; }

    float                m_radius{};
    float                m_width{};
    float                m_height{};
    float                m_leftAng{};
    float                m_topAng{};
    float                m_rightAng{};
    float                m_bottAng{};
    std::string          m_typeName;
    std::shared_ptr<VAO> m_vao;
    surfaceType          m_type{};

protected:
#ifdef ARA_USE_ASSIMP
    AssimpExport m_exporter;
#endif
    ShaderCollector                        *m_shCol          = nullptr;
    Shaders                                *m_genVertShdr    = nullptr;
    Shaders                                *m_genElemIndShdr = nullptr;
    std::unique_ptr<ShaderBuffer<custVec3>> m_pos;
    std::unique_ptr<ShaderBuffer<custVec3>> m_norm;
    std::unique_ptr<ShaderBuffer<custVec3>> m_texc;
    std::unique_ptr<ShaderBuffer<uint32_t>> m_ind;

    uint32_t    m_qHoriVerts = 0;
    uint32_t    m_qVertVerts = 0;
    uint32_t    m_qVertices  = 0;
    uint32_t    m_qIndices{};
    size_t      m_work_group_size{};
    std::string m_shdr_Header;
    std::string m_vertShdrSrc;
    std::string m_elemIndShdrSrc;
    float       m_meshVertsPerMM{};
    bool        m_useGL32Fallback = false;
};

}  // namespace ara