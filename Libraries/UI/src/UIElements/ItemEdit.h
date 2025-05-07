//
// Created by user on 13.08.2021.
//

#pragma once

#include <DataModel/ItemRef.h>
#include <UIElements/UITable.h>

namespace ara {

class ItemEdit : public UITable, public ItemRef {
public:
    ItemEdit();
    explicit ItemEdit(const std::string& styleClass);
    ~ItemEdit() override = default;
    void init() override;

protected:
    float m_rowHeight=0.f;
};

}  // namespace ara
