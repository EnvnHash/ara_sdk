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

#include "Asset/ResNode.h"

namespace ara {

class AssetManager;
class GLBase;

class AssetFont : public ResNode {
public:
    enum { none = 0, bold = 1, italic = 2 };

    int         m_Size  = 0;
    unsigned    m_Flags = none;
    std::string m_FontPath;

    AssetFont(std::string name, GLBase *glbase) : ResNode(name, glbase) {}

    void onProcess() override;
    bool onResourceChange(bool deleted, const std::string &res_fpath) override;

    bool        onLoad() override { return true; }
    bool        isOK() override { return true; }
    static bool isClass(ResNode *snode) { return snode->getFlag("font") != nullptr; }
};

}  // namespace ara