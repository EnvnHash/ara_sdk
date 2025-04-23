
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {

class UtfIterator {
public:
    std::string::const_iterator str_iter;
    size_t                      cplen{};

    explicit UtfIterator(std::string::const_iterator str_iter) : str_iter(std::move(str_iter)) {
        find_cplen();
    }

    std::string operator*() const {
        return {str_iter, str_iter + static_cast<int32_t>(cplen)};
    }

    UtfIterator &operator++() {
        str_iter += static_cast<int32_t>(cplen);
        find_cplen();
        return *this;
    }

    bool operator!=(const UtfIterator &o) const { return this->str_iter != o.str_iter; }

private:
    void find_cplen() {
        cplen = 1;
        if ((*str_iter & 0xf8) == 0xf0) {
            cplen = 4;
        } else if ((*str_iter & 0xf0) == 0xe0) {
            cplen = 3;
        } else if ((*str_iter & 0xe0) == 0xc0) {
            cplen = 2;
        }
    }
};

}

