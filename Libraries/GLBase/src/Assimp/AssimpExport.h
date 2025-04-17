#pragma once

#ifdef ARA_USE_ASSIMP

#include <assimp/Exporter.hpp>

#include "glb_common/glb_common.h"

namespace ara {

template <class T>
class ShaderBuffer;

class AssimpExport {
public:
    void singleMeshExport(uint32_t nrVert, uint32_t nrIndices, ShaderBuffer<custVec3> *pos,
                          ShaderBuffer<custVec3> *norm, ShaderBuffer<custVec3> *texc, ShaderBuffer<uint32_t> *indices,
                          std::string &path, std::string &formatId);

private:
    Assimp::Exporter exp;
};
}  // namespace ara
#endif