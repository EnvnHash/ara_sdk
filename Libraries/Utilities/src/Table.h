
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

#include <array>
#include <vector>

#include "glm/vec2.hpp"

namespace ara {

enum class dTableType : int32_t { Undef = 0, Pix, Percent, Cell = 100, Separator = 101 };

struct Table_rc {
    float                   value        = 0;
    dTableType              type         = dTableType::Undef;
    bool                    fixed        = false;
    std::array<float, 2>    sizeRange = {-1, -1};  // min/max size in Pixels

    float pos  = 0;  // position in pixels
    float size = 0;  // size in pixels
};

struct Table_sepInt {
    int32_t                 idx{};
    float                   pix{};
    dTableType              type{};
    std::array<Table_rc, 2> src_rc{};
};

struct Table_CellGeo {
    int32_t     idx{};
    int32_t     row{};
    int32_t     column{};
    glm::vec2   pixPos{};
    glm::vec2   pixSize{};
};

class TableRC {
public:
    typedef std::vector<Table_rc> RCV;

    int  getCount() { return static_cast<int32_t>(V().size()); }
    bool add(int32_t count);                                // adds content with defaults
    bool addPix(int32_t count, int32_t pix);                    // adds content with int_val=pix
    bool addPercent(int32_t count, float percent);          // adds content with fp_val=percent [0..100]
    bool ins(int32_t at, int32_t count);                        // inserts content with defaults // for all :
                                                        // if at==-1 at=getCount()
    bool insPix(int32_t at, int32_t count, int32_t pix);            // inserts content with int_val=pix
    bool insPercent(int32_t at, int32_t count, float percent);  // inserts content with fp_val=percent [0..100]
    bool setPix(int32_t index, int32_t pix);
    bool setPercent(int32_t index, float percent);
    bool ins(int32_t at, int32_t count, const Table_rc &rc);         // at==-1 : at=getCount()
    bool del(int32_t at, int32_t count);                        // erase count elements at 'at' if count==-1 count=getCount()
    bool      set(int32_t index, const Table_rc &rc);
    Table_rc get(int32_t index);

    friend TableRC &operator<<(TableRC &t, const Table_rc &rc) {
        t.ins(-1, 1, rc);
        return t;
    }

    bool        operator<<=(const Table_rc &rc) { return ins(-1, 1, rc); }
    Table_rc   &operator()(int32_t index) { return iVector[index]; }
    bool        updateGeo(float pix_size, float pix_margin_lo, float pix_margin_hi, float pix_padding);
    float       calculatePixGeo(float pix_margin_lo, float pix_margin_hi,
                                float pix_padding);  // calculates de extent of the content, percentage
                                                    // items are used as pixels
    RCV         &V() { return iVector; }
    dTableType  evalByPix(int32_t  &index, float pix);  // returns Type (None,Cell,Separator) index has the item's index
    bool        startSepInt(Table_sepInt &si, float pix);
    bool        updateSepInt(const Table_sepInt &si, float pix);

    static bool stopSepInt(Table_sepInt &si, float pix);
    float       setSepPix(int32_t index, float pix);  // returns the value in pixels that is accepted
    bool        setDynamicSize(bool on_off) { return opt_DynamicSize = on_off; }
    [[nodiscard]] bool  getDynamicSize() const { return opt_DynamicSize; }

private:
    RCV  iVector;
    bool opt_DynamicSize = false;
};

class Table {
public:
    virtual ~Table() = default;

    bool     updateGeo(float w, float h, float left_margin, float top_margin, float right_margin, float bottom_margin,
                       float h_padding, float v_padding);
    int      getCellCount();
    bool     startSepInt(const glm::vec2& p);
    bool     updateSepInt(const glm::vec2& p);
    bool     stopSepInt(const glm::vec2& p);
    TableRC &operator()(int32_t index) { return m_mat[index]; }
    bool     getCellGeo(Table_CellGeo &cg, int32_t row, int32_t col);
    bool     getCellGeo(Table_CellGeo &cg, int32_t index);

    int  rowColumnToIndex(int32_t row, int32_t column, bool validate);  // if validate : will check if row and column are in range
    bool indexToRowColumn(int32_t &row, int32_t &column, int32_t index, bool validate);

    virtual void reset() {}

protected:
    std::array<TableRC, 2>       m_mat;   // [0]=rows, [1]=columns
    std::array<Table_sepInt, 2>  m_sepInt;  // [0]=rows, [1]=columns
};

template <class T>
class CellTable : public Table, public std::vector<T> {
public:
    virtual bool updateCells() {
        std::vector<T>::resize(getCellCount());
        return true;
    }

    void reset() override {
        for (auto & i : m_mat) {
            i.del(0, i.getCount());
        }

        std::vector<T>::clear();
    }
};

}  // namespace ara