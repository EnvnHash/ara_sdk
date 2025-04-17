#pragma once

#include "Assimp/AssimpExport.h"

namespace ara {

class Shaders;
class VAO;

class SurfaceGenerator {
public:
    enum surfaceType { FLAT = 0, CYLINDER, DOME, PANADOME };

    SurfaceGenerator();

    virtual void init(surfaceType type, ShaderCollector *shCol);

    void resize();
    void generateVert();
    void generateElemInd();
    void exportCollada(std::string path, std::string formatId);
    void setTypeName(std::string typeName);

    void setUseGL32Fallback(bool val) { m_useGL32Fallback = val; }

    float                m_radius;
    float                m_width;
    float                m_height;
    float                m_leftAng;
    float                m_topAng;
    float                m_rightAng;
    float                m_bottAng;
    std::string          m_typeName;
    std::unique_ptr<VAO> m_vao;
    surfaceType          m_type;

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
    uint32_t    m_qIndices;
    size_t      m_work_group_size;
    std::string m_shdr_Header;
    std::string m_vertShdrSrc;
    std::string m_elemIndShdrSrc;
    float       m_meshVertsPerMM;
    bool        m_useGL32Fallback = false;
};

}  // namespace ara