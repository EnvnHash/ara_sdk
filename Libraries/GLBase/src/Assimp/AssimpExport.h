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