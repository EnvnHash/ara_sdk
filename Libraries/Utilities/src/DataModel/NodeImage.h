
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

#include "DataModel/Node.h"
#include "DataModel/NodeDataPath.h"

namespace ara::node {

class Image : public Node, public DataPath {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(m_imgPath, m_mipMaps)

    Image();

    void load(bool fromAssets) override;
    void setWatch(bool val) override;

    const std::string&  imgPath() { return m_imgPath; }
    int                 getMipMap() const { return m_mipMaps; }

protected:
    std::string m_imgPath;
    int         m_mipMaps = 1;
};

}