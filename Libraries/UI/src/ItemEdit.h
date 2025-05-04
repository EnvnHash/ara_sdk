//
// Created by user on 13.08.2021.
//

#pragma once

#include <DataModel/ItemRef.h>
#include <UITable.h>

namespace ara {

class ItemEdit : public UITable, public ItemRef {
public:
    ItemEdit();
    ItemEdit(const std::string& styleClass);
    virtual ~ItemEdit() = default;
    void init();

protected:
    float m_rowHeight=0.f;
};

}  // namespace ara
