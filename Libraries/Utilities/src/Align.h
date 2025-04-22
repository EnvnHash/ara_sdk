#pragma once

#include <util_common.h>

// sh: please use the functionality of UINode / UIMat instead to keep everything
// in one place
namespace ara {

struct e_align_item {
    int size[2]   = {0, 0};
    int offset[2] = {0, 0};    // this value is optional, if a particular item
                               // should have an extra offset added
    int position[2] = {0, 0};  // the result will be written here
};

struct e_align_attr {
    int  size[2]      = {0, 0};            // [x/y] sizes for the box
    int  margin[2][2] = {{0, 0}, {0, 0}};  // [min/max][x/y]
    int  padding[2]   = {0, 0};            // [x/y]
    bool constrain    = false;             // Elements won't go off the margins (PENDING)
};

class Align : public std::vector<e_align_item> {
public:
    enum { horizontal = 0, vertical = 1, left = 0, center = 1, right = 2, top = 0, vcenter = 1, bottom = 2 };

    e_align_item &Feed(e_align_item &item) {
        emplace_back(item);
        return back();
    }

    e_align_item &Feed(int w, int h) {
        emplace_back(e_align_item{w, h});
        return back();
    }

    int Calc(int orientation, int align, const e_align_attr &attr);  // returns the total size for the orientation axis
};

}  // namespace ara