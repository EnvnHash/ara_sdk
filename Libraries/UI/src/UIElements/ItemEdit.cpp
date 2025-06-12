//
// Created by user on 13.08.2021.
//

#include <DataModel/PropertyItemUi.h>
#include <UIElements/UIEdit.h>
#include <UIElements/ItemEdit.h>
#include <UIElements/PropSlider.h>
#include <UIElements/Button/Button.h>

using namespace std;

namespace ara {

ItemEdit::ItemEdit() : m_rowHeight(30.f) {
    setName(getTypeName<ItemEdit>());
    setFocusAllowed(false);
}

void ItemEdit::init() {
    // get all Item's children
    if (m_item) {
        clearCells();

        auto& ui_children = reinterpret_cast<std::list<std::unique_ptr<ItemUi>>&>(m_item->children());
        const auto nrPropItems = ranges::count_if(ui_children,
                                                  [](const unique_ptr<ItemUi>& it) { return it->isPropertyItem && it->visible(); });

        insertColumn(-1, 1, 25.f, true);
        insertColumn(-1, 1, 75.f, true);
        insertRow(-1, static_cast<int>(nrPropItems) + 1, m_rowHeight);

        // insert Headline
        const auto head = setCell<Label>(0, 0);
        head->setText(!m_item->displayName().empty() ? m_item->displayName() : m_item->name());
        head->addStyleClass(getStyleClass() + ".head");

        int row = 1;
        for (const auto& it: ui_children) {
            if (it->isPropertyItem && it->visible()) {
                const auto lbl = setCell<Label>(row, 0);
                lbl->setText(!it->displayName().empty() ? it->displayName() : it->name());
                lbl->addStyleClass(getStyleClass() + ".entry");

                if (it->m_typeId == tpi::tp_char || it->m_typeId == tpi::tp_string) {
                    const auto edit = setCell<UIEdit>(row, 1);
                    edit->addStyleClass(getStyleClass() + ".edit");
                    edit->setSingleLine();
                    edit->setTextAlignX(align::left);
                    edit->setPropItem(it.get());

                } else if (it->m_typeId == tpi::tp_int32 || it->m_typeId == tpi::tp_uint32 ||
                           it->m_typeId == tpi::tp_float || it->m_typeId == tpi::tp_double ||
                           it->m_typeId == tpi::tp_int64 || it->m_typeId == tpi::tp_uint64) {
                    const auto pSlid = setCell<PropSlider>(row, 1);

                    if (it->m_typeId == tpi::tp_float) {
                        const auto pit = dynamic_cast<PropertyItemUi<float>*>(it.get());
                        pSlid->setProp(*pit->getPtr());
                    } else if (it->m_typeId == tpi::tp_double) {
                        const auto pit = dynamic_cast<PropertyItemUi<double>*>(it.get());
                        pSlid->setProp(*pit->getPtr());
                    } else if (it->m_typeId == tpi::tp_int32) {
                        const auto pit = dynamic_cast<PropertyItemUi<int>*>(it.get());
                        pSlid->setProp(*pit->getPtr());
                    }
                } else if (it->m_typeId == tpi::tp_bool) {
                    const auto cb = setCell<Button>(row, 1);
                    cb->setIsToggle(true);
                    cb->addStyleClass(getStyleClass() + ".checkBox");
                    const auto pit = dynamic_cast<PropertyItemUi<bool>*>(it.get());
                    cb->setProp(*pit->getPtr());
                }
                ++row;
            }
        }
    }
}
}  // namespace ara
