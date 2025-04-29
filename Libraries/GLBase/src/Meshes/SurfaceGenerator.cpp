#include "Meshes/SurfaceGenerator.h"

#include <GLBase.h>
#include <Shaders/ShaderCollector.h>
#include <Shaders/ShaderUtils/ShaderBuffer.h>
#include <Utils/VAO.h>

using namespace std;

namespace ara {

SurfaceGenerator::SurfaceGenerator()
    : m_work_group_size(128), m_meshVertsPerMM(0.1f)  // 0.1f
{
    // template for vertex generating compute shaders
    // for hardware implementation reasons there is no such thing as a vec3 as a
    // shader storage buffer type for this reason we have to do this dirty trick
    // with a custom vec3, defined as a struct
    m_vertShdrSrc = STRINGIFY(
        struct custVec3 {
            float x;
            float y;
            float z;
        };
        layout(std430, binding = 1) buffer Pos{custVec3 pos[];}; \n
        layout(std430, binding = 2) buffer Norm { custVec3 norm[]; }; \n
        layout(std430, binding = 3) buffer TexC { custVec3 tex_coord[]; }; \n
        uniform float radius; \n
        uniform float leftAng; \n
        uniform float rightAng; \n
        uniform float topAng; \n
        uniform float bottAng; \n
        uniform float width; \n
        uniform float height; \n
        uniform uint qHoriVerts; \n
        uniform uint qVertVerts; \n
        uniform uint qVertices; \n
        uniform uint type; \n // 0=Flat, 1=Cylinder, 2=Dome, 3=Panadome
        const float pi = 3.1415926535897932384626433832795;
        void main() { \n
            const uint i = gl_GlobalInvocationID.x; \n
            if (i < qVertices) {
                // get the relative x,y position within the grid
                vec2 gridPos = vec2(float(i % qHoriVerts) / float(qHoriVerts - 1),
                float(i / qHoriVerts) / float(qVertVerts - 1)); \n
                // this is the texture coordinate
                tex_coord[i].x = gridPos.x; \n
                tex_coord[i].y = gridPos.y; \n
                tex_coord[i].z = 0.0; \n
                \n
                // get the corresponding elevation and azimuth
                vec2 vAngle = vec2(type == 2 ? 2.0 * pi * gridPos.x : leftAng + (rightAng - leftAng) * gridPos.x, \n
                type == 2 ? gridPos.y * pi * 0.5 : bottAng + (topAng - bottAng) * gridPos.y); \n
                // imagine a base curve along the equator, scale it according to its elevation
                float scaleXZ = (type == 1 ? 1.0 : cos(vAngle.y)) * radius; \n
                pos[i].x = type == 0 ? (gridPos.x - 0.5) * width : sin(vAngle.x) * scaleXZ; \n
                pos[i].y = (type == 0 || type == 1) ? gridPos.y * height :
                sin(vAngle.y) * radius; \n
                pos[i].z = type == 0 ? 0.f : -cos(vAngle.x) * scaleXZ; \n
                \n
                // normal always points from the unit spheres surface to the surfaces center
                norm[i].x = type == 0 ? 0.0 :
                type == 1 ? -sin(vAngle.x) : -sin(vAngle.x) * cos(vAngle.y); \n
                norm[i].y = (type == 0 || type == 1) ? 0.0 : -sin(vAngle.y); \n
                norm[i].z = type == 0 ? 1.0 :
                type == 1 ? cos(vAngle.x) : cos(vAngle.x) * cos(vAngle.y); \n
            }
        });

    // template for element index buffer generators
    // (y+1) * qHoriVerts +x *-----* (y+1) * qHoriVerts +x +1
    //						 |\    |
    //						 | \   |
    //						 |  \  |
    //						 |   \ |
    //						 |    \|
    //	   y * qHoriVerts +x *-----* y * qHoriVerts +x +1
    //
    // Define 2 Tris CW		[0: y *qHoriVerts +x,		1: (y+1)
    // *qHoriVerts +x		2: y *qHoriVerts +x +1 ]
    //						[3: y *qHoriVerts +x +1
    // 4: (y+1) *qHoriVerts +x,	5: (y+1) *qHoriVerts +x +1 ]
    // Define 2 Tris CCW	[0: y *qHoriVerts +x,		1: y *qHoriVerts
    // +x +1,		2: (y+1) *qHoriVerts +x]
    //						[3: (y+1) *qHoriVerts +x,
    // 4: y *qHoriVerts +x +1,		5: (y+1) *qHoriVerts +x +1]
    m_elemIndShdrSrc = STRINGIFY(
        layout(std430, binding = 1) buffer Indices { uint index[]; }; \n uniform uint qHoriVerts; \n uniform uint qVertVerts; \n uniform uint qIndices; \n void
            main() {
                \n uint i = gl_GlobalInvocationID.x;
                \n if (i < qIndices) {
                    // get the quad relative index (1-5) at this buffer index
                    uint quadVertInd = i % 6;
                    \n
                        // at which quad are we ?
                        uint quadInd = i / 6;
                    \n
                        // get the relative x, y gridPosition of the left lower
                        // corner of this quad there is one quad less than
                        // vertices in each horizontal quad strip
                        ivec2 gridPos = ivec2(quadInd % (qHoriVerts - 1), quadInd / (qHoriVerts - 1));
                    \n

                        index[i] = quadVertInd == 0 ? gridPos.y * qHoriVerts + gridPos.x
                                   : \n quadVertInd == 1                      ? gridPos.y * qHoriVerts + gridPos.x + 1
                                   : \n(quadVertInd == 2 || quadVertInd == 3) ? (gridPos.y + 1) * qHoriVerts + gridPos.x
                                   : \n quadVertInd == 4                      ? gridPos.y * qHoriVerts + gridPos.x + 1
                                   : \n quadVertInd == 5 ? (gridPos.y + 1) * qHoriVerts + gridPos.x + 1
                                                         : 0;
                    \n
                }
            }\n);
}

void SurfaceGenerator::init(surfaceType type, ShaderCollector *shCol) {
    m_type = type;

    // create a placeholder vao - this will be only used for drawing
    if (!m_vao) {
        m_vao = make_unique<VAO>("position:3f,normal:3f,texCoord:3f", GL_STATIC_DRAW);
    }

    // init the Shaders of they haven't been already m_inited
    if (!m_genVertShdr) {
        m_shdr_Header = shCol->getShaderHeader() + "layout(local_size_x=" + std::to_string(m_work_group_size) +
                        ", local_size_y=1,local_size_z=1) in;\n";
        m_vertShdrSrc    = m_shdr_Header + m_vertShdrSrc;
        m_elemIndShdrSrc = m_shdr_Header + m_elemIndShdrSrc;

        m_genVertShdr    = shCol->add("SurfaceGenerator", m_vertShdrSrc);
        m_genElemIndShdr = shCol->add("SurfGenElemenInd", m_elemIndShdrSrc);
    }

    // calculate the grid resolution
    if (m_genVertShdr && ((type == FLAT && m_width > 0.f && m_height > 0.f) ||
                          ((type == CYLINDER || type == PANADOME || type == DOME) && m_radius > 1.f))) {
        float hCircum=0.f, vCircum=0.f;
        if (type == FLAT) {
            hCircum = m_width;
            vCircum = m_height;
        } else if (type == CYLINDER) {
            // calculate the horizontal circumference (in mm)
            hCircum = (-m_leftAng + m_rightAng) / 360.f * (2.f * static_cast<float>(M_PI) * m_radius);
            // calculate the vertical circumference
            vCircum = static_cast<float>(M_PI) * m_height;
        } else if (type == DOME) {
            hCircum = 2.f * static_cast<float>(M_PI) * m_radius;
            vCircum = 0.5f * static_cast<float>(M_PI) * m_radius;
        } else if (type == PANADOME) {
            hCircum = (-m_leftAng + m_rightAng) / 360.f * (2.f * static_cast<float>(M_PI) * m_radius);
            vCircum = (-m_bottAng + m_topAng) / 360.f * (2.f * static_cast<float>(M_PI) * m_radius);
        }

        m_qHoriVerts = static_cast<uint32_t>(m_meshVertsPerMM * hCircum);
        m_qVertVerts = static_cast<uint32_t>(m_meshVertsPerMM * vCircum);

        // resize the shader buffers
        resize();
        generateVert();
        generateElemInd();

        // set the shader storage buffers inside the VAO
        m_vao->addExtBuffer(CoordType::Position, m_pos->getBuffer());
        m_vao->addExtBuffer(CoordType::Normal, m_norm->getBuffer());
        m_vao->addExtBuffer(CoordType::TexCoord, m_texc->getBuffer());
        m_vao->setExtElemIndices(m_qIndices, m_ind->getBuffer());
    }
}

void SurfaceGenerator::resize() {
    m_qVertices = m_qHoriVerts * m_qVertVerts;
    m_qIndices  = (m_qHoriVerts - 1) * (m_qVertVerts - 1) * 6;

    // create Buffers if they don't exist
    if (!m_pos) {
        m_pos = make_unique<ShaderBuffer<custVec3>>(m_qVertices);
    }

    if (!m_norm) {
        m_norm = make_unique<ShaderBuffer<custVec3>>(m_qVertices);
    }

    if (!m_texc) {
        m_texc = make_unique<ShaderBuffer<custVec3>>(m_qVertices);
    }

    // two triangles per segment = 6 vertices per MeshCell
    if (!m_ind) {
        m_ind = make_unique<ShaderBuffer<uint32_t>>(m_qIndices);
    }

    if (m_qVertices > m_pos->getSize()) {
        m_pos->resize(m_qVertices);
        m_norm->resize(m_qVertices);
        m_texc->resize(m_qVertices);
    }

    if (m_qIndices > m_ind->getSize()) {
        m_ind->resize(m_qIndices);
    }
}

void SurfaceGenerator::generateVert() const {
    // run the compute shader, generate!!
    m_genVertShdr->begin();
    m_genVertShdr->bindProgramPipeline();

    m_genVertShdr->setUniform1ui("type", static_cast<uint32_t>(m_type));
    m_genVertShdr->setUniform1ui("qHoriVerts", m_qHoriVerts);
    m_genVertShdr->setUniform1ui("qVertVerts", m_qVertVerts);
    m_genVertShdr->setUniform1ui("qVertices", m_qVertices);

    if (m_type != FLAT) {
        m_genVertShdr->setUniform1f("radius", m_radius * 0.001f);
    }

    if (m_type == FLAT || m_type == CYLINDER) {
        m_genVertShdr->setUniform1f("height", m_height * 0.001f);
    }

    if (m_type == FLAT) {
        m_genVertShdr->setUniform1f("width", m_width * 0.001f);
    }

    if (m_type == CYLINDER || m_type == PANADOME) {
        m_genVertShdr->setUniform1f("leftAng", glm::radians(m_leftAng));
        m_genVertShdr->setUniform1f("rightAng", glm::radians(m_rightAng));
    }

    if (m_type == PANADOME) {
        m_genVertShdr->setUniform1f("topAng", glm::radians(m_topAng));
        m_genVertShdr->setUniform1f("bottAng", glm::radians(m_bottAng));
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pos->getBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_norm->getBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_texc->getBuffer());

    // round up
    glDispatchCompute(static_cast<GLuint>(std::ceil(static_cast<float>(m_pos->getSize()) / static_cast<float>(m_work_group_size))), 1, 1);

    // We need to block here on compute completion to ensure that the
    // computation is done before we render
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

    ara::Shaders::unbindProgramPipeline();
    ara::Shaders::end();
}

void SurfaceGenerator::generateElemInd() const {
    m_genElemIndShdr->begin();
    m_genElemIndShdr->bindProgramPipeline();

    m_genElemIndShdr->setUniform1ui("qIndices", m_qIndices);
    m_genElemIndShdr->setUniform1ui("qHoriVerts", m_qHoriVerts);
    m_genElemIndShdr->setUniform1ui("qVertVerts", m_qVertVerts);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ind->getBuffer());
    glDispatchCompute(static_cast<GLuint>(std::ceil(static_cast<float>(m_ind->getSize()) / static_cast<float>(m_work_group_size))), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

    Shaders::unbindProgramPipeline();
    Shaders::end();
}

void SurfaceGenerator::exportCollada(const std::string& path, const std::string &formatId) {
#ifdef ARA_USE_ASSIMP
    if (m_qVertices > 0) {
        m_exporter.singleMeshExport(m_qVertices, m_qIndices, m_pos.get(), m_norm.get(), m_texc.get(),
                                    m_ind.get(), path, formatId);
    }
#endif
}

void SurfaceGenerator::setTypeName(const std::string& typeName) {
    if (!strcmp(typeName.c_str(), "Flat")) {
        m_type = FLAT;
    } else if (!strcmp(typeName.c_str(), "Cylinder")) {
        m_type = CYLINDER;
    } else if (!strcmp(typeName.c_str(), "Dome")) {
        m_type = DOME;
    } else if (!strcmp(typeName.c_str(), "Panadome")) {
        m_type = PANADOME;
    }
}

}  // namespace ara