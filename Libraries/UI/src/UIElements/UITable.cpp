#include "UITable.h"



#include "UIApplication.h"

using namespace glm;
using namespace std;

namespace ara {
UITable::UITable() {
    setTypeName<UITable>();
    setName(getTypeName<UITable>());
    setFocusAllowed(false);
    m_canReceiveDrag = true;
}

UITable::UITable(const UITableParameters& par) {
    setTypeName<UITable>();
    setName(getTypeName<UITable>());
    setFocusAllowed(false);
    m_canReceiveDrag = true;

    std::visit([this](auto&& arg) {
        if (arg.x != 0 || arg.y != 0) {
            setPos(arg);
        }
    }, par.pos);

    std::visit([this](auto&& arg) {
        if (arg.x != 0 || arg.y != 0) {
            setSize(arg);
        }
    }, par.size);

    bool updateCells = false;
    std::array<std::function<void()>, 8> funcMap {
            [&] { auto c = par.fgColor.value(); UITable::setColor(c.r, c.g, c.b, c.a); },
            [&] { UITable::setBackgroundColor(par.bgColor.value()); },
            [&] { setAlignX(par.alignX.value()); },
            [&] { setAlignY(par.alignY.value()); },
            [&] { addStyleClass(par.style.value()); },
            [&] {
                if (par.topology.value().y) {
                    m_cells(0).add(par.topology.value().y);
                    updateCells = true;
                }
                if (par.topology.value().x) {
                    m_cells(1).add(par.topology.value().x);
                    updateCells = true;
                } },
            [&] { UITable::setMargins(par.margin.value().x, par.margin.value().y); },
            [&] { UITable::setSpacing(par.spacing.value().x, par.spacing.value().y); },
    };
    iterateOptionals(par.getTiedOptionals(), arrayToTuple(funcMap), std::make_index_sequence<funcMap.size()>{});

    if (updateCells) {
        UITable::initNewCellNode();
    }

    UITable::geo_Update();
}

int UITable::geo_Update() {
    m_geoUpdating = true;

    if (getDynamicWidth()) {
        float v = m_cells(1).calculatePixGeo(m_margin[0].x, m_margin[1].x, m_spacing.x);
        if (v != getContentSize().x) {
            setWidth(static_cast<int>(v));
        }
    }

    if (getDynamicHeight()) {
        float v = m_cells(0).calculatePixGeo(m_margin[0].y, m_margin[1].y, m_spacing.y);
        if (v != getContentSize().y) {
            setHeight(static_cast<int>(v));
        }
    }

    if (!m_cells.updateGeo(getContentSize().x, getContentSize().y, m_margin[0].x, m_margin[0].y, m_margin[1].x,
                           m_margin[1].y, m_spacing.x, m_spacing.y)) {
        return 0;
    }

    UINode*        uinode = nullptr;
    Table_CellGeo cg;
    int            i = 0;

    for (e_cell& cell : m_cells) {
        if (m_cells.getCellGeo(cg, i)) {
            if ((uinode = cell.ui_node) != nullptr) {
                if (uinode->getPos().x != cg.pixPos[0] || uinode->getPos().y != cg.pixPos[1]) {
                    uinode->setPos(static_cast<int>(cg.pixPos[0]), static_cast<int>(cg.pixPos[1]));
                }

                if (uinode->getSize().x != cg.pixSize[0] || uinode->getSize().y != cg.pixSize[1]) {
                    uinode->setSize(static_cast<int>(cg.pixSize[0]), static_cast<int>(cg.pixSize[1]));
                }
            }
        }

        ++i;
    }

    for (const auto& it : m_children) {
        std::dynamic_pointer_cast<UINode>(it)->setChanged(true);  // recursively force matrix update
    }

    m_geoUpdating = false;
    return m_cells.getCellCount();
}

void UITable::setRowHeight(int32_t idx, int32_t heightInPix) {
    m_cells(0).setPix(idx, heightInPix);
}

/** to be used for moving UINodes from existing parents to the table */
UINode* UITable::setCell(int row, int column, const vector<shared_ptr<UINode> >::iterator& nodeIt) {
    if (int idx; (idx = m_cells.rowColumnToIndex(row, column, true)) >= 0) {
        // get the cell's ui_node
        auto cell = m_cells[idx].ui_node;

        // move from another parent to the cell
        cell->children().insert(cell->children().end(), std::make_move_iterator(nodeIt),
                                   std::make_move_iterator(nodeIt + 1));

        m_cells[idx].content = dynamic_cast<UINode *>(cell->children().back().get());
    }

    geo_Update();
    return nodeIt->get();
}

void UITable::updateMatrix() {
    if (m_geoChanged && !m_geoUpdating) {
        geo_Update();
    }
    UINode::updateMatrix();
}

void UITable::mouseDrag(hidData& data) {
    if (mouseEvent & 1) {
        m_cells.updateSepInt(data.mousePosNodeRel);
        setDrawFlag();
        m_geoChanged = true;
        data.consumed = true;
    }
}

void UITable::mouseMove(hidData& data) {
    std::array<dTableType, 2>   pp{};
    ivec2                       ii {};

    pp[0] = m_cells(1).evalByPix(ii.x, data.mousePosNodeRel.x);
    pp[1] = m_cells(0).evalByPix(ii.y, data.mousePosNodeRel.y);

#ifdef ARA_USE_GLFW
    if (pp[0] == dTableType::Separator && pp[1] == dTableType::Separator) {
        m_sharedRes->winHandle->setMouseCursor(WinMouseIcon::crossHair);
    } else if (pp[0] == dTableType::Separator && pp[1] != dTableType::Separator) {
        m_sharedRes->winHandle->setMouseCursor(WinMouseIcon::hresize);
    } else if (pp[0] != dTableType::Separator && pp[1] == dTableType::Separator) {
        m_sharedRes->winHandle->setMouseCursor(WinMouseIcon::vresize);
    } else {
        m_sharedRes->winHandle->setMouseCursor(WinMouseIcon::arrow);
    }
#endif
    data.consumed = true;
}

void UITable::mouseOut(hidData& data) {
#ifdef ARA_USE_GLFW
    m_sharedRes->winHandle->setMouseCursor(WinMouseIcon::arrow);
#endif
}

void UITable::mouseDown(hidData& data) {
    if (!data.hit) {
        return;
    }

    if (m_cells.startSepInt(data.mousePosNodeRel)) {
        mouseEvent |= 1;
        data.consumed = true;
    }
}

void UITable::mouseUp(hidData& data) {
    if (!data.hit) {
        return;
    }

    if (mouseEvent & 1) {
        m_cells.stopSepInt(data.mousePosNodeRel);
        mouseEvent &= ~1;
    }

    data.consumed = true;
}

bool UITable::insertRow(int at, int count, float size, bool percent, bool fixed, float min_pix_size,
                        float max_pix_size) {
    Table_rc tb{size,
                 size <= 0 ? dTableType::Undef
                 : percent ? dTableType::Percent
                           : dTableType::Pix,
                 fixed, {min_pix_size, max_pix_size}};
    m_cells(0).ins(at, count, tb);
    initNewCellNode();
    return true;
}

bool UITable::insertColumn(int at, int count, float size, bool percent, bool fixed, float min_pix_size,
                           float max_pix_size) {
    Table_rc tb{size,
                 size <= 0 ? dTableType::Undef
                 : percent ? dTableType::Percent
                           : dTableType::Pix,
                 fixed, {min_pix_size, max_pix_size}};
    m_cells(1).ins(at, count, tb);
    initNewCellNode();
    return true;
}

void UITable::initNewCellNode() {
    m_cells.updateCells();

    for (auto & m_Cell : m_cells) {
        if (!m_Cell.ui_node) {
            m_Cell.ui_node = &push<Div>();
            m_Cell.ui_node->setName("TableCell");
            m_Cell.ui_node->setBackgroundColor(getColor());
        }
    }
}

bool UITable::clearCell(int row, int column, bool updateGeo) {
    int idx;
    if ((idx = m_cells.rowColumnToIndex(row, column, true)) < 0) {
        return false;
    }

    if (m_cells[idx].ui_node) {
        m_cells[idx].ui_node->clearChildren();
    }

    m_cells[idx].content = nullptr;

    if (updateGeo) {
        geo_Update();
    }

    return true;
}

bool UITable::removeRow(int row) {
    if (m_cells(0).getCount() > row) {
        // iterate through all cells of this row and remove all content
        vector<vector<e_cell>::iterator> cellsToDelete;
        for (int col = 0; col < m_cells(1).getCount(); col++) {
            int idx;
            if ((idx = m_cells.rowColumnToIndex(row, col, true)) < 0) {
                return false;
            }

            m_cells[idx].content = nullptr;

            if (m_cells[idx].ui_node) {
                m_cells[idx].ui_node->clearChildren();
                remove(m_cells[idx].ui_node);
            }

            cellsToDelete.emplace_back(m_cells.begin() + idx);
        }

        for (const auto& it : cellsToDelete) {
            m_cells.erase(it);
        }

        // remove the row
        m_cells(0).del(row, 1);
        geo_Update();
    }

    return true;
}

void UITable::clearCells() {
    for (auto& it : m_cells) {
        it.content = nullptr;
        if (it.ui_node) {
            it.ui_node->clearChildren();
            remove(it.ui_node);
        }
    }

    m_cells.reset();
    geo_Update();
}

void UITable::setFixedCellSize(bool val) {
    for (int i=0; i<2; ++i) {
        for (auto& rc : m_cells(1).V()) {
            rc.fixed = val;
        }
    }
}

}
