#pragma once

#include <vector>

namespace ara {

enum class dTableType : int { Undef = 0, Pix, Percent, Cell = 100, Separator = 101 };

struct eTable_rc {
    float      value        = 0;
    dTableType type         = dTableType::Undef;
    bool       fixed        = false;
    float      sizeRange[2] = {-1, -1};  // min/max size in Pixels

    float pos  = 0;  // position in pixels
    float size = 0;  // size in pixels
};

struct eTable_sepInt {
    int        idx{};
    float      pix{};
    dTableType type{};
    eTable_rc  src_rc[2]{};
};

struct eTable_CellGeo {
    int   idx{};
    int   row{};
    int   column{};
    float pixPos[2]{};
    float pixSize[2]{};
};

class TableRC {
public:
    typedef std::vector<eTable_rc> RCV;

    int  getCount() { return (int)V().size(); }
    bool add(int count);                        // adds content with defaults
    bool addPix(int count, int pix);            // adds content with int_val=pix
    bool addPercent(int count, float percent);  // adds content with fp_val=percent [0..100]
    bool ins(int at, int count);                // inserts content with defaults // for all :
                                                // if at==-1 at=getCount()
    bool insPix(int at, int count,
                int pix);  // inserts content with int_val=pix
    bool insPercent(int at, int count,
                    float percent);  // inserts content with fp_val=percent [0..100]
    bool setPix(int index, int pix);
    bool setPercent(int index, float percent);
    bool ins(int at, int count, eTable_rc &rc);  // at==-1 : at=getCount()
    bool del(int at, int count);                 // erase count elements at 'at' if count==-1 count=getCount()
    bool      set(int index, eTable_rc &rc);
    eTable_rc get(int index);

    friend TableRC &operator<<(TableRC &t, eTable_rc &rc) {
        t.ins(-1, 1, rc);
        return t;
    }

    bool       operator<<=(eTable_rc &rc) { return ins(-1, 1, rc); }
    eTable_rc &operator()(int index) { return iVector[index]; }
    bool       updateGeo(float pix_size, float pix_margin_lo, float pix_margin_hi, float pix_padding);
    float      calculatePixGeo(float pix_margin_lo, float pix_margin_hi,
                               float pix_padding);  // calculates de extent of the content, percentage
                                                    // items are used as pixels
    RCV       &V() { return iVector; }
    dTableType evalByPix(int  &index, float pix);  // returns Type (None,Cell,Separator) index has the item's index
    bool  startSepInt(eTable_sepInt &si, float pix);
    bool  updateSepInt(eTable_sepInt &si, float pix);
    bool  stopSepInt(eTable_sepInt &si, float pix);
    float setSepPix(int index, float pix);  // returns the value in pixels that is accepted
    bool  setDynamicSize(bool on_off) { return (opt_DynamicSize = on_off); }
    bool  getDynamicSize() { return opt_DynamicSize; }

private:
    RCV  iVector;
    bool opt_DynamicSize = false;
};

class Table {
public:
    bool     updateGeo(float w, float h, float left_margin, float top_margin, float right_margin, float bottom_margin,
                       float h_padding, float v_padding);
    int      getCellCount();
    bool     startSepInt(float p[2]);
    bool     updateSepInt(float p[2]);
    bool     stopSepInt(float p[2]);
    TableRC &operator()(int index) { return m_Mat[index]; }
    bool     getCellGeo(eTable_CellGeo &cg, int row, int col);
    bool     getCellGeo(eTable_CellGeo &cg, int index);

    int rc2index(int row, int column, bool validate);  // if validate : will check if row and column are in range
    bool index2rc(int &row, int &column, int index, bool validate);

    virtual void reset() {}

protected:
    TableRC       m_Mat[2];   // [0]=rows, [1]=columns
    eTable_sepInt sepInt[2];  // [0]=rows, [1]=columns
};

template <class T>
class CellTable : public Table, public std::vector<T> {
public:
    virtual bool updateCells() {
        std::vector<T>::resize(getCellCount());
        return true;
    }

    void reset() {
        for (int i = 0; i < 2; i++) {
            m_Mat[i].del(0, m_Mat[i].getCount());
        }

        std::vector<T>::clear();
    }
};

}  // namespace ara