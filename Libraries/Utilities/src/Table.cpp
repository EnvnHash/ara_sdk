
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

#include "Table.h"
#include "Log.h"

namespace ara {

bool TableRC::add(int32_t count) {
    Table_rc tb{};
    return ins(-1, count, tb);
}

bool TableRC::addPix(int32_t count, int32_t pix) {
    Table_rc tb{static_cast<float>(pix), dTableType::Pix};
    return ins(-1, count, tb);
}

bool TableRC::addPercent(int32_t count, float percent) {
    Table_rc tb{percent, dTableType::Percent};
    return ins(-1, count, tb);
}

bool TableRC::ins(int32_t at, int32_t count) {
    Table_rc tb{};
    return ins(-1, count, tb);
}

bool TableRC::insPix(int32_t at, int32_t count, int32_t pix) {
    Table_rc tb{static_cast<float>(pix), dTableType::Pix};
    return ins(-1, count, tb);
}

bool TableRC::insPercent(int32_t at, int32_t count, float percent) {
    Table_rc tb{percent, dTableType::Percent};
    return ins(-1, count, tb);
}

bool TableRC::setPix(int32_t index, int32_t pix) {
    Table_rc tb{static_cast<float>(pix), dTableType::Pix};
    return set(index, tb);
}

bool TableRC::setPercent(int32_t index, float percent) {
    Table_rc tb{percent, dTableType::Percent};
    return set(index, tb);
}

bool TableRC::ins(int32_t at, int32_t count, const Table_rc &rc) {
    if (at == -1) {
        at = getCount();
    }

    if (at < 0 || at > getCount() || count <= 0) {
        return false;
    }

    const auto it = iVector.begin() + at;
    iVector.insert(it, count, rc);

    return true;
}

bool TableRC::del(int32_t at, int32_t count) {
    if (count == 0) {
        count = 1;
    }

    if (count < 0) {
        count = getCount();
    }

    if (at < 0 || at >= getCount() || count <= 0) {
        return false;
    }

    auto it = iVector.begin() + at;
    iVector.erase(it, it + std::min<int32_t>(count, getCount() - at));
    return true;
}

bool TableRC::set(int32_t index, const Table_rc &rc) {
    if (index < 0 || index >= getCount()) {
        return false;
    }

    iVector[index] = rc;
    return true;
}

Table_rc TableRC::get(int32_t index) {
    if (index < 0 || index >= getCount()) {
        return {};
    }
    return iVector[index];
}

bool TableRC::updateGeo(float pix_size, float pix_margin_lo, float pix_margin_hi, float pix_padding) {
    int32_t n;

    if ((n = getCount()) <= 0) {
        return false;
    }

    float eff_size = pix_size - pix_margin_lo - pix_padding * static_cast<float>(n - 1) - pix_margin_hi;  // effective size
    float t_size   = 0;
    float cpix = 0, cper = 0;
    int32_t   none_count = 0;

    for (Table_rc &rc : iVector) {
        switch (rc.type) {
            case dTableType::Pix:
                cpix += rc.value;
                rc.size = rc.value;
                break;
            case dTableType::Percent: break;
            default: none_count++;
        }
    }

    t_size = eff_size - cpix;

    for (Table_rc &rc : iVector) {
        if (rc.type == dTableType::Percent) {
            rc.size = rc.value * t_size / 100.f;
            cper += rc.size;
        }
    }

    t_size -= cper;

    if (none_count) {
        for (Table_rc &rc : iVector) {
            if (rc.type == dTableType::Undef) {
                rc.size = t_size / static_cast<float>(none_count);
            }
        }
    }

    float ta = eff_size, i = 0;

    for (Table_rc &rc : iVector) {
        if (static_cast<int32_t>(i) == n - 1 && !rc.fixed) {
            rc.size = ta;
        }
        ta -= rc.size;
        i++;
    }

    float p = pix_margin_lo;

    for (Table_rc &rc : iVector) {
        rc.pos = p;
        p += rc.size;
        p += pix_padding;
    }

    return true;
}

float TableRC::calculatePixGeo(float pix_margin_lo, float pix_margin_hi, float pix_padding) {
    int32_t   n;
    float sum = 0;

    if ((n = getCount()) <= 0) {
        return 0;
    }

    float eff_size = pix_margin_lo + pix_padding * static_cast<float>(n - 1) + pix_margin_hi;  // effective size
    float cpix = 0, cper = 0, cnone = 0;
    int32_t   none_count = 0;

    for (Table_rc &rc : iVector) {
        switch (rc.type) {
            case dTableType::Pix:
                cpix += rc.value;
                rc.size = rc.value;
                break;
            case dTableType::Percent:
                cper += rc.value;
                rc.size = rc.value;
                break;
            default:
                cnone += 20.f;
                rc.size = 20.f;
                none_count++;
                break;
        }
    }

    sum = cpix;
    sum += eff_size;
    return sum;
}

dTableType TableRC::evalByPix(int32_t &index, float pix) {
    int32_t i = 0;

    for (RCV::const_iterator it = iVector.begin(); it < iVector.end(); ++it) {
        if (pix >= it->pos && pix < it->pos + it->size) {
            index = i;
            return dTableType::Cell;
        }

        if (pix >= it->pos + it->size && i < getCount() - 1 && pix < it[1].pos) {
            index = i;

            if (it[0].fixed) {
                return dTableType::Undef;
            }

            return dTableType::Separator;
        }
        i++;
    }
    index = -1;
    return dTableType::Undef;
}

bool TableRC::startSepInt(Table_sepInt &si, float pix) {
    if ((si.type = evalByPix(si.idx, pix)) != dTableType::Separator) {
        return false;
    }

    si.src_rc[0] = get(si.idx);
    si.src_rc[1] = get(si.idx + 1);

    if (si.src_rc[0].fixed) {
        return false;
    }

    setSepPix(si.idx, si.src_rc[0].size);
    setSepPix(si.idx + 1, si.src_rc[1].size);
    si.pix = pix;
    return true;
}

bool TableRC::updateSepInt(const Table_sepInt &si, float pix) {
    if (si.type != dTableType::Separator) {
        return false;
    }

    auto dx = pix - si.pix;
    auto tl = si.src_rc[0].size + si.src_rc[1].size;
    auto np = si.src_rc[0].size + dx;

    if (np < 0) {
        np = 0;
    }

    if (si.src_rc[0].fixed) {
        return false;
    }

    np = setSepPix(si.idx, np);

    if (!getDynamicSize()) {
        if (tl - np < 0) {
            np = tl;
        }

        if (!si.src_rc[1].fixed) {
            setSepPix(si.idx + 1, tl - np);
        }
    }

    return true;
}

float TableRC::setSepPix(int32_t index, float pix) {
    if (index < 0 || index >= getCount()) {
        return 0;
    }

    auto rc = iVector[index];
    rc.type = dTableType::Pix;

    if (rc.sizeRange[0] != -1 && pix < rc.sizeRange[0]) {
        pix = rc.sizeRange[0];
    }

    if (rc.sizeRange[1] != -1 && pix > rc.sizeRange[1]) {
        pix = rc.sizeRange[1];
    }

    rc.value = pix;
    iVector[index] = rc;
    return rc.value;
}

bool TableRC::stopSepInt(Table_sepInt &si, float pix) {
    si.type = dTableType::Undef;
    return true;
}

bool Table::updateGeo(float w, float h, float left_margin, float top_margin, float right_margin, float bottom_margin,
                      float h_padding, float v_padding) {
    m_mat[0].updateGeo(h, top_margin, bottom_margin, v_padding);
    m_mat[1].updateGeo(w, left_margin, right_margin, h_padding);
    return true;
}

int32_t Table::getCellCount() {
    return m_mat[0].getCount() * m_mat[1].getCount();
}

bool Table::startSepInt(const glm::vec2& p) {
    if (static_cast<int32_t>(m_mat[0].evalByPix(m_sepInt[0].idx, p.y)) == 0 &&
        static_cast<int32_t>(m_mat[1].evalByPix(m_sepInt[1].idx, p.x)) == 0) {
        return false;
    }

    bool r = m_mat[0].startSepInt(m_sepInt[0], p[1]);
    r |= m_mat[1].startSepInt(m_sepInt[1], p[0]);
    return r;
}

bool Table::updateSepInt(const glm::vec2& p) {
    m_mat[0].updateSepInt(m_sepInt[0], p.y);
    m_mat[1].updateSepInt(m_sepInt[1], p.x);
    return true;
}

bool Table::stopSepInt(const glm::vec2& p) {
    TableRC::stopSepInt(m_sepInt[0], p.y);
    TableRC::stopSepInt(m_sepInt[1], p.x);
    return true;
}

bool Table::getCellGeo(Table_CellGeo &cg, int32_t row, int32_t col) {
    if (row < 0 || col < 0) {
        return false;
    }

    if (row >= m_mat[0].getCount() || col >= m_mat[1].getCount()) {
        return false;
    }

    cg.idx        = row * m_mat[1].getCount() + row;
    cg.row        = row;
    cg.column     = col;
    cg.pixPos[0]  = m_mat[1](col).pos;
    cg.pixPos[1]  = m_mat[0](row).pos;
    cg.pixSize[0] = m_mat[1](col).size;
    cg.pixSize[1] = m_mat[0](row).size;

    return true;
}

bool Table::getCellGeo(Table_CellGeo &cg, int32_t index) {
    return index < 0 || index >= getCellCount()
               ? false
               : getCellGeo(cg, index / m_mat[1].getCount(), index % m_mat[1].getCount());
}

int32_t Table::rowColumnToIndex(int32_t row, int32_t column, bool validate) {
    int32_t idx = row * m_mat[1].getCount() + column;
    if (!validate) {
        return idx;
    }
    return (row < 0 || row >= m_mat[0].getCount()) || (column < 0 || column >= m_mat[1].getCount()) ? -1 : idx;
}

bool Table::indexToRowColumn(int32_t &row, int32_t &column, int32_t index, bool validate) {
    int32_t nc;

    if ((nc = m_mat[1].getCount()) <= 0) {
        return false;
    }

    if (validate && (index < 0 || index >= getCellCount())) {
        return false;
    }

    row    = index / nc;
    column = index % nc;
    return true;
}

}