
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

#include "Align.h"

namespace ara {

int Align::Calc(int orientation, int align, const e_align_attr &attr) {
    int  sum    = 0;
    int  n      = static_cast<int>(size());
    auto it     = begin();
    int  pos[2] = {0, 0};
    int  sleft  = 0;

    if (n <= 0) {
        return 0;
    }

    pos[0] = attr.margin[0][0];
    pos[1] = attr.margin[0][1];

    if (orientation == horizontal) {
        sum   = attr.margin[0][0] + attr.padding[0] * (n - 1) + attr.margin[1][0];
        sleft = attr.size[1] - attr.margin[0][1] - attr.margin[1][1];

        for (it = begin(); it < end(); ++it) {
            sum += it->size[0];
            it->position[0] = pos[0] + it->offset[0];

            if (align == top) {
                it->position[1] = pos[1];
            } else if (align == vcenter) {
                it->position[1] = pos[1] + (sleft / 2) - (it->size[1] / 2);
            } else if (align == bottom) {
                it->position[1] = pos[1] + sleft - it->size[1];
            }

            it->position[1] += it->offset[1];
            pos[0] += it->size[0] + attr.padding[0];
        }

        return sum;
    } else {
        sum   = attr.margin[0][1] + attr.padding[1] * (n - 1) + attr.margin[1][1];
        sleft = attr.size[0] - attr.margin[0][0] - attr.margin[1][0];

        for (it = begin(); it < end(); ++it) {
            sum += it->size[1];
            it->position[1] = pos[1] + it->offset[1];

            if (align == left) {
                it->position[0] = pos[0];
            } else if (align == center) {
                it->position[0] = pos[0] + (sleft / 2) - (it->size[0] / 2);
            } else if (align == right) {
                it->position[0] = pos[0] + sleft - it->size[0];
            }

            it->position[0] += it->offset[0];
            pos[1] += it->size[1] + attr.padding[1];
        }

        return sum;
    }
}

}