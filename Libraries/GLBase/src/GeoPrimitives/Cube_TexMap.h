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

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {
class Cube_TexMap : public GeoPrimitive {
public:
    explicit Cube_TexMap(float _width = 2.f, float _height = 2.f, float _depth = 2.f);
    virtual ~Cube_TexMap() = default;

    void draw(TFO *tfo = nullptr) override;
    void init() override;

private:
    float width  = 0;
    float height = 0;
    float depth  = 0;
};
}  // namespace ara
