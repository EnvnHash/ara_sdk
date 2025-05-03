//
// Created by user on 13.08.2021.
//

#include <DataModel/PropertyItemUi.h>
#include "ItemEdit.h"
#include "UIEdit.h"
#include "PropSlider.h"
#include "Button/Button.h"

using namespace std;

namespace ara {

ItemEdit::ItemEdit() : UITable(), m_rowHeight(30.f) {
    setName(getTypeName<ItemEdit>());
    setFocusAllowed(false);
}

ItemEdit::ItemEdit(const std::string& styleClass) : UITable(std::move(styleClass)) {
    setName(getTypeName<ItemEdit>());
    setFocusAllowed(false);
}

void ItemEdit::init() {
    // get all Item's children
    if (m_item) {
        clearCells();

        auto& ui_children = reinterpret_cast<std::list<std::unique_ptr<ItemUi>>&>(m_item->children());
        auto nrPropItems = std::count_if(ui_children.begin(), ui_children.end(),
                          [](const unique_ptr<ItemUi>& it) { return it->isPropertyItem && it->visible(); });

        insertColumn(-1, 1, 25.f, true);
        insertColumn(-1, 1, 75.f, true);
        insertRow(-1, (int)nrPropItems + 1, m_rowHeight);

        // insert Headline
        auto head = setCell<Label>(0, 0);
        head->setText(!m_item->displayName().empty() ? m_item->displayName() : m_item->name());
        head->addStyleClass(getStyleClass() + ".head");

        int row = 1;
        for (const auto& it: ui_children) {
            if (it->isPropertyItem && it->visible()) {
                auto lbl = setCell<Label>(row, 0);
                lbl->setText(!it->displayName().empty() ? it->displayName() : it->name());
                lbl->addStyleClass(getStyleClass() + ".entry");

                if (it->m_typeId == tpi::tp_char || it->m_typeId == tpi::tp_string) {
                    auto edit = setCell<UIEdit>(row, 1);
                    edit->addStyleClass(getStyleClass() + ".edit");
                    edit->setSingleLine();
                    edit->setTextAlignX(align::left);
                    edit->setPropItem(it.get());

                } else if (it->m_typeId == tpi::tp_int32 || it->m_typeId == tpi::tp_uint32 ||
                           it->m_typeId == tpi::tp_float || it->m_typeId == tpi::tp_double ||
                           it->m_typeId == tpi::tp_int64 || it->m_typeId == tpi::tp_uint64) {
                    auto pSlid = setCell<PropSlider>(row, 1);

                    if (it->m_typeId == tpi::tp_float) {
                        auto pit = static_cast<PropertyItemUi<float>*>(it.get());
                        pSlid->setProp(pit->getPtr());
                    } else if (it->m_typeId == tpi::tp_double) {
                        auto pit = static_cast<PropertyItemUi<double>*>(it.get());
                        pSlid->setProp(pit->getPtr());
                    } else if (it->m_typeId == tpi::tp_int32) {
                        auto pit = static_cast<PropertyItemUi<int>*>(it.get());
                        pSlid->setProp(pit->getPtr());
                    }
                } else if (it->m_typeId == tpi::tp_bool) {
                    auto cb = setCell<Button>(row, 1);
                    cb->setIsToggle(true);
                    cb->addStyleClass(getStyleClass() + ".checkBox");
                    auto pit = static_cast<PropertyItemUi<bool>*>(it.get());
                    cb->setProp(pit->getPtr());
                }

                row++;
            }
        }
    }
}
}  // namespace ara
