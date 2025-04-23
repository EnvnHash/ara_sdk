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


#include "Asset/NodeImageBase.h"

namespace ara::node {

ImageBase::ImageBase()  {
    setTypeName<node::ImageBase>();
}

int *ImageBase::getVerPos(int *pos, int ver) {
    for (int i = 0; i < 2; i++) {
        pos[i] = getSectionPos()[i];
    }

    if (m_Dist == Dist::horz) {
        pos[0] += m_DistPixOffset * getVer(ver);
    } else if (m_Dist == Dist::vert) {
        pos[1] += m_DistPixOffset * getVer(ver);
    }

    return pos;
}

}
