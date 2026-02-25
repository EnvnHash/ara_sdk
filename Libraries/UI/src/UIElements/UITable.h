#pragma once

#include "Table.h"
#include "UIElements/Div.h"

namespace ara {

class UITableParameters  {
public:
    std::variant<glm::ivec2, glm::vec2> pos{};
    std::variant<glm::ivec2, glm::vec2> size{};
    std::optional<glm::vec4> fgColor{};
    std::optional<glm::vec4> bgColor{};
    std::optional<align> alignX{};
    std::optional<valign> alignY{};
    std::optional<std::string> style{};
    std::optional<glm::ivec2> topology{};
    std::optional<glm::vec2> margin{};
    std::optional<glm::vec2> spacing{};
    auto getTiedOptionals() const {
        return std::tie(fgColor, bgColor, alignX, alignY, style, topology, margin, spacing);
    }
};

class UITable : public Div {
public:
    typedef struct e_cell {
        Div*    ui_node = nullptr;
        UINode* content = nullptr;
    } e_cell;

    UITable();
    UITable(const UITableParameters& par);
    ~UITable() override = default;

    virtual int getRowCount() { return m_cells(0).getCount(); }
    virtual int getColumnCount() { return m_cells(1).getCount(); }

    virtual void setSpacing(float px, float py) {
        m_spacing.x = px;
        m_spacing.y = py;
    }  // Table cell padding

    virtual void setTLMargin(float dx, float dy) {
        m_margin[0].x = dx;
        m_margin[0].y = dy;
    }  // Table Top-Left margins

    virtual void setBRMargin(float dx, float dy) {
        m_margin[1].x = dx;
        m_margin[1].y = dy;
    }  // Table Bottom-Right margins

    virtual void setMargins(float dx, float dy) {
        setTLMargin(dx, dy);
        setBRMargin(dx, dy);
    }  // Table margins

    virtual UINode* setCell(int row, int column, const std::vector<std::shared_ptr<UINode>>::iterator& nodeIt);
    virtual void setFixedCellSize(bool val);

    virtual bool clearCell(int row, int column, bool updateGeo = true);
    virtual void clearCells();
    virtual bool removeRow(int row);

    // UITables' calculations depend on the matrices of its children, so we have to modify the udpateMatrix and
    // updtMatrIt method
    void updateMatrix() override;

    // set color has to be overloaded since it corresponds to the children's
    // background color
    virtual void setColor(float r, float g, float b, float a) {
        m_color = glm::vec4(r, g, b, a);
        for (const auto& it : m_cells) {
            it.ui_node->setBackgroundColor(getColor());
        }
    }
    virtual void setColor(glm::vec4& col) {
        m_color = col;
        for (const auto& it : m_cells) {
            it.ui_node->setBackgroundColor(getColor());
        }
    }
    virtual void setRowHeight(int32_t idx, int32_t heightInPix);

    void mouseDrag(hidData& data) override;
    void mouseMove(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseUp(hidData& data) override;
    void mouseOut(hidData& data) override;

    virtual bool insertRow(int at, int count, float size, bool percent = false, bool fixed = false,
                           float min_pix_size = -1,
                           float max_pix_size = -1);  // at: -1 means at the end (append)
    virtual bool insertColumn(int at, int count, float size, bool percent = false, bool fixed = false,
                              float min_pix_size = -1, float max_pix_size = -1);
    virtual void initNewCellNode();

    virtual Table& getTable() { return m_cells; }

    virtual int geo_Update();  // returns number of cells

    virtual bool setDynamicWidth(bool on_off) { return m_cells(1).setDynamicSize(on_off); }
    virtual bool getDynamicWidth() { return m_cells(1).getDynamicSize(); }
    virtual bool setDynamicHeight(bool on_off) { return m_cells(0).setDynamicSize(on_off); }
    virtual bool getDynamicHeight() { return m_cells(0).getDynamicSize(); }
    virtual Table_rc getRow(int idx) { return (m_cells(0).getCount() > idx ? m_cells(0).get(idx) : Table_rc{}); }
    virtual CellTable<e_cell>& getCells() { return m_cells; }

    /** add a new UINode to the Table */
    template <typename T>
    T* setCell(int row, int column, std::shared_ptr<T> node = nullptr) {
        if (int idx; (idx = m_cells.rowColumnToIndex(row, column, true)) >= 0) {
            T* newNode;
            if (node) {
                newNode = static_cast<T*>(&m_cells[idx].ui_node->push(std::move(node)));
            } else {
                newNode = &m_cells[idx].ui_node->push<T>();
            }

            m_cells[idx].content = newNode;
            geo_Update();
            return newNode;
        }
        return nullptr;
    }

private:
    CellTable<e_cell> m_cells;

    glm::vec2 m_spacing{};
    std::array<glm::vec2, 2> m_margin{};

    unsigned mouseEvent = 0;

    bool opt_DrawEmptyCells = true;
    bool m_geoUpdating      = false;

};

}  // namespace ara
