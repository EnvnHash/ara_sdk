//
// Created by user on 08.08.2021.
//

#pragma once

#include <ListProperty.h>
#include <Label.h>
#include <ScrollView.h>
#include <UITable.h>

namespace ara {

class ListItemBase : public Div {
public:
    ListItemBase() : Div() {
#ifdef __ANDROID__
        setCanReceiveDrag(true);
#endif
        setName(getTypeName<ListItemBase>());
    }

    ListItemBase(std::string &&styleClass) : Div(std::move(styleClass)) {
#ifdef __ANDROID__
        setCanReceiveDrag(true);
#endif
        setName(getTypeName<ListItemBase>());
    }
};

template <typename T>
class ListItem : public ListItemBase {
public:
    ListItem() : ListItemBase() { setName(getTypeName<ListItem>()); }
    ListItem(std::string &&styleClass) : ListItemBase(std::move(styleClass)) { setName(getTypeName<ListItem>()); }

    void init() override {
        Div::init();
        label = addChild<Label>();
        if (val) setData(val, m_idx);
    }

    virtual void setData(T *data, int idx) {
        if (!data) return;

        val   = data;
        m_idx = idx;

        if (!label) return;

        if (typeid(T) == typeid(std::string)) {
            label->setText(*reinterpret_cast<std::string *>(data));
        } else if (typeid(T) == typeid(const char *)) {
            //            label->setText(std::string(*reinterpret_cast<const
            //            char**>(data)));
        } else if (typeid(T) == typeid(int)) {
            label->setText(std::to_string(*reinterpret_cast<int *>(data)));
        } else if (typeid(T) == typeid(int32_t)) {
            label->setText(std::to_string(*reinterpret_cast<int32_t *>(data)));
        } else if (typeid(T) == typeid(unsigned int)) {
            label->setText(std::to_string(*reinterpret_cast<unsigned int *>(data)));
        } else if (typeid(T) == typeid(float)) {
            label->setText(std::to_string(*reinterpret_cast<float *>(data)));
        } else if (typeid(T) == typeid(double)) {
            label->setText(std::to_string(*reinterpret_cast<double *>(data)));
        } else if (typeid(T) == typeid(long long)) {
            label->setText(std::to_string(*reinterpret_cast<long long *>(data)));
        } else if (typeid(T) == typeid(unsigned long long)) {
            label->setText(std::to_string(*reinterpret_cast<unsigned long long *>(data)));
        }
    }

    T     *val   = nullptr;
    int    m_idx = 0;
    Label *label = nullptr;
};

class ListBase : public ScrollView {
public:
    ListBase();

    explicit ListBase(std::string &&styleClass);

    virtual ~ListBase() = default;

    void init() override;

    virtual void rebuild() = 0;

    void loadStyleDefaults() override;

    void updateStyleIt(ResNode *node, state st, std::string &styleClass) override;

    void setRowHeight(float val) { m_rowHeight = val; }

    void setSpacing(float spX, float spY) {
        m_space.x = spX;
        m_space.y = spY;
    }

protected:
    UITable  *m_table     = nullptr;
    float     m_rowHeight = 30.f;
    glm::vec2 m_space     = glm::vec2{0.f};
};

template <typename DataTyp, class LiTyp = ListItem<DataTyp>>
class List : public ListBase {
public:
    List() : ListBase() {}
    explicit List(std::string &&styleClass) : ListBase(std::move(styleClass)) {}

    void rebuild() override {
        if (!m_table || (!m_items && !m_itemsVec && !m_listProp)) return;

        m_table->clearCells();
        m_table->insertColumn(-1, 1, 1.f, false);
        m_table->insertRow(-1,
                           (int)(m_items      ? m_items->size()
                                 : m_itemsVec ? m_itemsVec->size()
                                 : m_listProp ? m_listProp->size()
                                              : 1),
                           m_rowHeight);
        m_table->t_setSpacing(m_space.x, m_space.y);

        m_uiItems.clear();

        int i = 0;
        if (m_items) {
            for (auto li = m_items->begin(); li != m_items->end(); li++) {
                m_uiItems.emplace_back(
                    m_table->setCell<LiTyp>(i, 0, std::make_unique<LiTyp>(getStyleClass() + ".item")));
                if (!m_uiItems.back()) continue;
                m_uiItems.back()->setData(&(*li), i);
                m_uiItems.back()->addMouseClickCb([this, i, li](hidData *data) {
                    if (!m_clickCb || !m_items) return;
                    m_clickCb(&(*li), i, data);
                });
                i++;
            }
        } else if (m_itemsVec) {
            for (auto li = m_itemsVec->begin(); li != m_itemsVec->end(); li++) {
                m_uiItems.emplace_back(
                    m_table->setCell<LiTyp>(i, 0, std::make_unique<LiTyp>(getStyleClass() + ".item")));
                if (!m_uiItems.back()) continue;
                m_uiItems.back()->setData(&(*li), i);
                m_uiItems.back()->addMouseClickCb([this, i, li](hidData *data) {
                    if (!m_clickCb || !m_itemsVec) return;
                    m_clickCb(&(*li), i, data);
                });
                i++;
            }
        } else if (m_listProp) {
            for (auto li = m_listProp->begin(); li != m_listProp->end(); li++) {
                m_uiItems.emplace_back(
                    m_table->setCell<LiTyp>(i, 0, std::make_unique<LiTyp>(getStyleClass() + ".item")));
                if (!m_uiItems.back()) continue;
                m_uiItems.back()->setData(&(*li), i);
                m_uiItems.back()->addMouseClickCb([this, i, li](hidData *data) {
                    if (!m_clickCb) return;
                    m_clickCb(&(*li), i, data);
                });
                i++;
            }
        }

        setDrawFlag();
    }

    void set(std::list<DataTyp> *data, bool doRebuild = true) {
        if (!data) return;
        m_items = data;

        if (doRebuild)
            addGlCb(std::to_string((int64_t)this), [this] {
                rebuild();
                return true;
            });
    }

    void set(std::vector<DataTyp> *data, bool doRebuild = true) {
        if (!data) return;
        m_itemsVec = data;

        if (doRebuild)
            addGlCb(std::to_string((int64_t)this), [this] {
                rebuild();
                return true;
            });
    }

    void set(ListProperty<DataTyp> *dataProp, bool doRebuild = true) {
        if (!dataProp) return;
        m_listProp = dataProp;
        onChanged<DataTyp>(dataProp, [this, doRebuild](std::any v) {
            auto sess = std::any_cast<std::list<DataTyp> *>(v);
            if (sess && doRebuild)
                addGlCb(std::to_string((int64_t)this), [this] {
                    rebuild();
                    return true;
                });
        });
    }

    void setClickCb(std::function<void(DataTyp *, int, hidData *data)> cb) { m_clickCb = std::move(cb); }

protected:
    std::vector<DataTyp>                              *m_itemsVec = nullptr;
    std::list<DataTyp>                                *m_items    = nullptr;
    ListProperty<DataTyp>                             *m_listProp = nullptr;
    std::vector<LiTyp *>                               m_uiItems;
    std::function<void(DataTyp *, int, hidData *data)> m_clickCb;
};

}  // namespace ara