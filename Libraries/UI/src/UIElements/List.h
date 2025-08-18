//
// Created by user on 08.08.2021.
//

#pragma once

#include <ListProperty.h>
#include <UIElements/Label.h>
#include <UIElements/ScrollView.h>
#include <UIElements/UITable.h>

namespace ara {

class ListItemBase : public Label {
public:
    ListItemBase() {
#ifdef __ANDROID__
        setCanReceiveDrag(true);
#endif
        setName(getTypeName<ListItemBase>());
    }
};

template<typename T>
concept AllowedContainer = requires(T t) {
    { t.begin() } -> std::same_as<decltype(t.end())>;
    { t.size() } -> std::integral;
};

template <typename T>
class ListItem : public ListItemBase {
public:
    ListItem() {
        setName(getTypeName<ListItem>());
    }

    void init() override {
        Label::init();
        setData(val, m_idx);
    }

    virtual void setData(const T& data, int idx) {
        val   = data;
        m_idx = idx;
        if constexpr (std::is_floating_point_v<T> || std::is_integral_v<T>) {
            setText(std::to_string(data));
        } else {
            setText(data);
        }
    }

    T      val{};
    int    m_idx = 0;
};

template <typename T>
class ListBase : public ScrollView {
public:
    ListBase() {
        setName(getTypeName<ListBase>());
        setFocusAllowed(false);
    }

    ~ListBase() override = default;

    void init() override {
        ScrollView::init();

        // table must always be on top of ScrollViews children, in order to have the children's bounding box being
        // calculated correctly before ScrollBar onChange callbacks are evaluated
        m_table = addChild<UITable>();
        m_table->setDynamicHeight(true);

        rebuild();
    };

    void loadStyleDefaults() override {
        UINode::loadStyleDefaults();
        m_setStyleFunc[state::none][styleInit::rowHeight] = [this] { m_rowHeight = 30.f; };
    }

    void updateStyleIt(ResNode* node, const state st, const std::string& styleClass) override {
        UINode::updateStyleIt(node, st, styleClass);

        if (const auto rh = node->findNumericNode("rowHeight"); get<ResNode*>(rh)) {
            if (get<unitType>(rh) == unitType::Percent) {
                float val                                = stof(get<std::string>(rh)) * 0.01f;
                m_setStyleFunc[st][styleInit::rowHeight] = [this, val] {
                    m_rowHeight = val;
                    addGlCb("rbList", [this] {
                        rebuild();
                        return true;
                    });
                };
            } else {
                int val                                  = stoi(get<std::string>(rh));
                m_setStyleFunc[st][styleInit::rowHeight] = [this, val] {
                    m_rowHeight = static_cast<float>(val);
                    addGlCb("rbList", [this] {
                        rebuild();
                        return true;
                    });
                };
            }
        }
    }

    virtual void rebuild() = 0;

    template <AllowedContainer C>
    void rebuild(const C& items, std::vector<ListItem<T>*>& uiItems) {
        if (!m_table) {
            return;
        }

        m_table->clearCells();
        m_table->insertColumn(-1, 1, 1.f, false);
        m_table->insertRow(-1, static_cast<int32_t>(items.size()), m_rowHeight, false, true);
        m_table->setSpacing(m_space.x, m_space.y);
        m_table->updateMatrix();

        uiItems.clear();
        int i = 0;
        for (auto li = items.begin(); li != items.end(); ++li) {
            uiItems.emplace_back(m_table->setCell<ListItem<T>>(i, 0));
            if (!uiItems.back()) {
                continue;
            }
            uiItems.back()->addStyleClass(getStyleClass() + ".item");
            uiItems.back()->setData(*li, i);
            uiItems.back()->addMouseClickCb([this, i, li](hidData& data) {
                if (m_clickCb){
                    m_clickCb(*li, i, data);
                }
            });
            ++i;
        }
    }

    void setRowHeight(float val) {
        m_rowHeight = val;
    }

    void setSpacing(float spX, float spY) {
        m_space.x = spX;
        m_space.y = spY;
    }

    void setClickCb(std::function<void(const T&, int, hidData& data)> cb) {
        m_clickCb = std::move(cb);
    }

protected:
    UITable*                                            m_table     = nullptr;
    float                                               m_rowHeight = 30.f;
    glm::vec2                                           m_space     = glm::vec2{0.f};
    std::function<void(const T&, int, hidData& data)>   m_clickCb;
};

template <AllowedContainer C>
class List : public ListBase<typename C::value_type> {
public:
    using T = typename C::value_type;

    void rebuild() override {
        ListBase<T>::rebuild(m_items, m_uiItems);
        UINode::setDrawFlag();
    }

    void set(C& data, bool doRebuild = true) {
        m_items = data;
        if (doRebuild) {
            rebuild();
        }
    }

protected:
    C                                                   m_items;
    std::vector<ListItem<T>*>                           m_uiItems;
};

template <typename T>
class PList : public ListBase<T> {
public:
    void rebuild() override {
        if (!m_listProp) {
            return;
        }
        ListBase<T>::rebuild(m_listProp->get(), m_uiItems);
        UINode::setDrawFlag();
    }

    void set(ListProperty<T> *dataProp, bool doRebuild = true) {
        if (!dataProp) {
            return;
        }
        m_listProp = dataProp;
        UINode::onChanged<T>(*dataProp, [this, doRebuild](const std::any &v) {
            auto sess = std::any_cast<std::list<T>*>(v);
            if (sess && doRebuild) {
                UINode::addGlCb(std::to_string(reinterpret_cast<int64_t>(this)), [this] {
                    rebuild();
                    return true;
                });
            }
        });
    }

protected:
    ListProperty<T>*                                    m_listProp = nullptr;
    std::vector<ListItem<T>*>                           m_uiItems;
};

}  // namespace ara