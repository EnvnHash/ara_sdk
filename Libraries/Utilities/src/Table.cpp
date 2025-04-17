#include "Table.h"

namespace ara {

bool TableRC::add(int count) {
    eTable_rc tb{};
    return ins(-1, count, tb);
}

bool TableRC::addPix(int count, int pix) {
    eTable_rc tb{float(pix), dTableType::Pix};
    return ins(-1, count, tb);
}

bool TableRC::addPercent(int count, float percent) {
    eTable_rc tb{percent, dTableType::Percent};
    return ins(-1, count, tb);
}

bool TableRC::ins(int at, int count) {
    eTable_rc tb{};
    return ins(-1, count, tb);
}

bool TableRC::insPix(int at, int count, int pix) {
    eTable_rc tb{float(pix), dTableType::Pix};
    return ins(-1, count, tb);
}

bool TableRC::insPercent(int at, int count, float percent) {
    eTable_rc tb{percent, dTableType::Percent};
    return ins(-1, count, tb);
}

bool TableRC::setPix(int index, int pix) {
    eTable_rc tb{float(pix), dTableType::Pix};
    return set(index, tb);
}

bool TableRC::setPercent(int index, float percent) {
    eTable_rc tb{percent, dTableType::Percent};
    return set(index, tb);
}

bool TableRC::ins(int at, int count, eTable_rc &rc) {
    if (at == -1) at = getCount();

    if (at < 0 || at > getCount() || count <= 0) {
        return false;
    }

    auto it = iVector.begin() + at;
    it               = iVector.insert(it, count, rc);

    return true;
}

bool TableRC::del(int at, int count) {
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
    iVector.erase(it, it + std::min<int>(count, getCount() - at));
    return true;
}

bool TableRC::set(int index, eTable_rc &rc) {
    if (index < 0 || index >= getCount()) {
        return false;
    }

    iVector[index] = rc;

    return true;
}

eTable_rc TableRC::get(int index) {
    if (index < 0 || index >= getCount()) {
        return {};
    }
    return iVector[index];
}

bool TableRC::updateGeo(float pix_size, float pix_margin_lo, float pix_margin_hi, float pix_padding) {
    int n;

    if ((n = getCount()) <= 0) {
        return false;
    }

    float eff_size = pix_size - pix_margin_lo - pix_padding * (float)(n - 1) - pix_margin_hi;  // effective size
    float t_size   = 0;
    float cpix = 0, cper = 0;
    int   none_count = 0;

    for (eTable_rc &rc : iVector) {
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

    for (eTable_rc &rc : iVector) {
        if (rc.type == dTableType::Percent) {
            rc.size = rc.value * t_size / 100.f;
            cper += rc.size;
        }
    }

    t_size -= cper;

    if (none_count) {
        for (eTable_rc &rc : iVector) {
            if (rc.type == dTableType::Undef) {
                rc.size = t_size / (float)none_count;
            }
        }
    }

    float ta = eff_size, i = 0;

    for (eTable_rc &rc : iVector) {
        if (i == n - 1 && !rc.fixed) {
            rc.size = ta;
        }
        ta -= rc.size;
        i++;
    }

    float p = pix_margin_lo;

    for (eTable_rc &rc : iVector) {
        rc.pos = p;
        p += rc.size;
        p += pix_padding;
    }

    return true;
}

float TableRC::calculatePixGeo(float pix_margin_lo, float pix_margin_hi, float pix_padding) {
    int   n;
    float sum = 0;

    if ((n = getCount()) <= 0) {
        return 0;
    }

    float eff_size = pix_margin_lo + pix_padding * static_cast<float>(n - 1) + pix_margin_hi;  // effective size
    float t_size   = 0;
    float cpix = 0, cper = 0, cnone = 0;
    int   none_count = 0;

    for (eTable_rc &rc : iVector) {
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

dTableType TableRC::evalByPix(int &index, float pix) {
    int                 i = 0;
    RCV::const_iterator it;

    for (it = iVector.begin(); it < iVector.end(); it++) {
        if ((pix >= it->pos) && (pix < it->pos + it->size)) {
            index = i;
            return dTableType::Cell;
        }

        if ((pix >= it->pos + it->size) && (i < getCount() - 1) && (pix < it[1].pos)) {
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

bool TableRC::startSepInt(eTable_sepInt &si, float pix) {
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

bool TableRC::updateSepInt(eTable_sepInt &si, float pix) {
    if (si.type != dTableType::Separator) {
        return false;
    }

    float dx = pix - si.pix;
    float tl = si.src_rc[0].size + si.src_rc[1].size;
    float np = si.src_rc[0].size + dx;

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

float TableRC::setSepPix(int index, float pix) {
    if (index < 0 || index >= getCount()) {
        return 0;
    }

    eTable_rc rc = iVector[index];
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

bool TableRC::stopSepInt(eTable_sepInt &si, float pix) {
    si.type = dTableType::Undef;
    return true;
}

bool Table::updateGeo(float w, float h, float left_margin, float top_margin, float right_margin, float bottom_margin,
                      float h_padding, float v_padding) {
    m_Mat[0].updateGeo(h, top_margin, bottom_margin, v_padding);
    m_Mat[1].updateGeo(w, left_margin, right_margin, h_padding);
    // updateCells();
    return true;
}

int Table::getCellCount() {
    // return ((!m_Mat[0].getCount() && !m_Mat[1].getCount()) ? 0 :
    // std::max<int>(m_Mat[0].getCount(), 1) *
    // std::max<int>(m_Mat[1].getCount(), 1));
    return m_Mat[0].getCount() * m_Mat[1].getCount();
}

bool Table::startSepInt(float p[2]) {
    if (static_cast<int>(m_Mat[0].evalByPix(sepInt[0].idx, p[1])) != 0 &&
        static_cast<int>(m_Mat[1].evalByPix(sepInt[1].idx, p[0])) != 0) {
        return false;
    }

    bool r = m_Mat[0].startSepInt(sepInt[0], p[1]);
    r |= m_Mat[1].startSepInt(sepInt[1], p[0]);
    return r;
}

bool Table::updateSepInt(float p[2]) {
    m_Mat[0].updateSepInt(sepInt[0], p[1]);
    m_Mat[1].updateSepInt(sepInt[1], p[0]);
    return true;
}

bool Table::stopSepInt(float p[2]) {
    m_Mat[0].stopSepInt(sepInt[0], p[1]);
    m_Mat[1].stopSepInt(sepInt[1], p[0]);
    return true;
}

bool Table::getCellGeo(eTable_CellGeo &cg, int row, int col) {
    if (row < 0 || col < 0) {
        return false;
    }

    if (row >= m_Mat[0].getCount() || col >= m_Mat[1].getCount()) {
        return false;
    }

    cg.idx        = row * m_Mat[1].getCount() + row;
    cg.row        = row;
    cg.column     = col;
    cg.pixPos[0]  = m_Mat[1](col).pos;
    cg.pixPos[1]  = m_Mat[0](row).pos;
    cg.pixSize[0] = m_Mat[1](col).size;
    cg.pixSize[1] = m_Mat[0](row).size;

    return true;
}

bool Table::getCellGeo(eTable_CellGeo &cg, int index) {
    return index < 0 || index >= getCellCount()
               ? false
               : getCellGeo(cg, index / m_Mat[1].getCount(), index % m_Mat[1].getCount());
}

int Table::rc2index(int row, int column, bool validate) {
    int idx = row * m_Mat[1].getCount() + column;
    if (!validate) {
        return idx;
    }
    return (row < 0 || row >= m_Mat[0].getCount()) || (column < 0 || column >= m_Mat[1].getCount()) ? -1 : idx;
}

bool Table::index2rc(int &row, int &column, int index, bool validate) {
    int nc;

    if ((nc = m_Mat[1].getCount()) <= 0) {
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