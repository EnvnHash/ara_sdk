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

#include "Asset/AssetFont.h"
#include "Asset/FontList.h"

using namespace std;

namespace ara {

void AssetFont::onProcess() {
    if ((m_size = value<int32_t>("size", 0)) <= 0) {
        throw runtime_error("ResFont::OnProcess Error: Invalid size ("+std::to_string(m_size)+")");
    }

    if (getFlag("bold")) {
        m_flags |= bold;
    }

    if (getFlag("italic")) {
        m_flags |= italic;
    }

    m_fontPath = getValue("font");
    if (m_fontPath.empty()) {
        m_fontPath = getValue("src");
    }

    if (m_fontPath.empty()) {
        throw runtime_error("ResFont::OnProcess Error: No font path");
    }
}

bool AssetFont::onResourceChange(bool deleted, const string &res_fpath) {
    if (deleted){
        return false;
    }
    return true;
}

}  // namespace ara
