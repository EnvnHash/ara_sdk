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

#include <Asset/AssetImageBase.h>

using namespace std;

namespace ara {

std::array<int, 2> AssetImageBase::getVerPos(int ver) {
    std::array<int, 2> pos{ getSectionPos()[0], getSectionPos()[1] };

    if (m_dist == Dist::vert) {
        pos[1] += m_distPixOffset * getVer(ver);
    } else if (m_dist == Dist::horz) {
        pos[0] += m_distPixOffset * getVer(ver);
    }

    return pos;
}

}