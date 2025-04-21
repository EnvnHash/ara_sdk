//
// Created by user on 16.02.2021.
//

#pragma once

#include <Canvas.h>
#include <Computer.h>
#include <ComboBox.h>
#include <Workspace.h>

#include "DataModel/ItemRef.h"

namespace ara::proj {

/** type of ConnectionCombo is always meant to be the type of the source object.
 * The type of the destination object is meant to be set as "connectToType" */

template <class T>
class ConnectionCombo : public ComboBox, public ItemRef {
public:
    ConnectionCombo() : ComboBox(), ItemRef() {}

    ConnectionCombo(T *item, std::string connectToType)
        : ComboBox(), ItemRef(), m_item(item), m_connectToType(connectToType) {}

    ConnectionCombo(T *item, std::string connectToType, std::string styleClass)
        : ComboBox(styleClass), ItemRef(), m_item(item), m_connectToType(connectToType) {}

    void init() {
        ComboBox::init();

        if (m_connectToType == "Canvas")
            initForType<proj::Canvas>();
        else if (m_connectToType == "Camera")
            initForType<proj::Camera>();
        else if (m_connectToType == "Computer")
            initForType<proj::Computer>();
        else if (m_connectToType == "Eyepoint")
            initForType<proj::Eyepoint>();
        else if (m_connectToType == "Projector")
            initForType<proj::Projector>();
        else if (m_connectToType == "Surface")
            initForType<proj::Surface>();
    }

    template <class Tp>
    void initForType() {
        if (m_item) {
            auto itCon = (ItemConnectable *)m_item;
            auto con   = static_cast<Property<Tp *> *>(itCon->m_propList[m_connectToType]);
            if (!con) return;

            if ((*con)()) {
                Item *conItem = (Item *)((*con)());
                setMenuName(conItem->name());  // update immediately if there is
                                               // a valid connection
                ItemRef::onChanged<std::string>(&conItem->name,
                                                [this](std::any val) { setMenuName(std::any_cast<std::string>(val)); });
            }

            // update the active Entry on changes
            ItemRef::onChanged<Tp *>(con, [this](std::any val) {
                try {
                    auto ptr = std::any_cast<Tp *>(val);
                    if (ptr) {
                        setMenuName(ptr->name());
                        ItemRef::onChanged<std::string>(
                            &ptr->name, [this](std::any val) { setMenuName(std::any_cast<std::string>(val)); });
                    } else {
                        // connection deleted
                        setMenuName(std::string(""));
                    }
                } catch (const std::bad_any_cast &e) {
                    // happens when the value passed is a nullptr
                }
            });
        }
    }

    void setMenuName(std::string str) {
        m_menuEntryName = str;
        m_menuEntryButt->setText(str);
    }

    void addEntry(Property<std::string> name, std::function<void()> f) { m_entries.push_back(std::make_pair(name, f)); }

    void open() {
        if (m_open) {
            close();
            return;
        }

        // create on top of all nodes
        m_entryList = getRoot()->addChild<ScrollView>(getStyleClass() + ".list");
        rebuildEntryList();
        m_entryList->setPos((int)getWinPos().x, (int)(getWinPos().y + m_sharedRes->gridSize.y));
        m_open = true;
    }

    void rebuildEntryList() {
        if (!m_entryList) return;

        m_entries.clear();

        if (m_connectToType == "Canvas")
            rebuildEntriesForType<proj::Canvas>();
        else if (m_connectToType == "Camera")
            rebuildEntriesForType<proj::Camera>();
        else if (m_connectToType == "Computer")
            rebuildEntriesForType<proj::Computer>();
        else if (m_connectToType == "Eyepoint")
            rebuildEntriesForType<proj::Eyepoint>();
        else if (m_connectToType == "Surface")
            rebuildEntriesForType<proj::Surface>();
        else if (m_connectToType == "Projector")
            rebuildEntriesForType<proj::Projector>();

        // rebuild list
        m_entryList->setHeight((int)m_entries.size() * m_listEntryHeight);

        int i = 0;
        for (auto &entry : m_entries) {
            auto butt = m_entryList->addChild<Button>(getStyleClass() + ".entries");
            m_entryButts.push_back(butt);  // maintain a separate list of entry button since
                                           // m_entries may contain other elements

            butt->setName("Button_" + entry.first());
            butt->setText(entry.first());
            butt->setSize(1.f, m_listEntryHeight);
            butt->setPos(0, m_listEntryHeight * i);

            butt->setMouseClickCb([this, entry](mouseClickData *data) {
                entry.second();
                close();
            });

            i++;
        }

        updateStyle();
    }

    template <class Tp>
    void rebuildEntriesForType() {
        // add an empty entry to the front to delete the connection
        m_entries.push_back(std::make_pair(std::string(""), [this]() {
            if (m_item && !m_connectToType.empty()) {
                // remove connection on m_item
                auto con = static_cast<Property<Tp *> *>(m_item->m_propList[m_connectToType]);
                if (con) {
                    if (typeid(T) == typeid(proj::Projector) && typeid(Tp) == typeid(proj::Canvas))
                        // disconnect projector from old connectedCanvas
                        if ((*con)()) ((proj::Canvas *)(*con)())->removeProjector((proj::Projector *)m_item);

                    // get the destination item and from it the connection
                    // leading to this item
                    if ((*con)()) {
                        auto dCon = static_cast<Property<T *> *>((*con)()->m_propList[getName<T>()]);
                        if (dCon) (*dCon) = nullptr;
                    }

                    (*con) = nullptr;
                }

                setMenuName(std::string(""));
            }
        }));

        // get all items of m_connectToType == template type
        for (auto &it : proj::Workspace::activeProject()->children<Tp>())
            addEntry(it->name(), [this, it]() {
                if (it && m_item && !m_connectToType.empty()) {
                    // set connection on m_item
                    auto con = static_cast<Property<Tp *> *>(m_item->m_propList[m_connectToType]);

                    // also set the connected property on the destination item
                    // projector and canvas is a special case, since there can
                    // be various projectors connected to a single canvas
                    if (typeid(T) == typeid(proj::Projector) && typeid(Tp) == typeid(proj::Canvas)) {
                        // disconnect projector from old connectedCanvas
                        if (con && (*con)()) ((proj::Canvas *)(*con)())->removeProjector((proj::Projector *)m_item);
                        // add to new connectedCanvas
                        ((proj::Canvas *)it)->addProjector((proj::Projector *)m_item);
                    } else {
                        // delete old connection from destination item to this
                        // item
                        if (con && (*con)()) {
                            auto oldBackCon = static_cast<Property<Tp *> *>((*con)()->m_propList[getName<T>()]);
                            if (oldBackCon) (*oldBackCon) = nullptr;
                        }
                        auto dCon = static_cast<Property<T *> *>(it->m_propList[getName<T>()]);
                        (*dCon)   = m_item;
                    }

                    (*con) = it;

                    if (con && (*con)()) setMenuName((*con)()->name());
                }
            });
    }

    void setConnectToType(std::string tp) { m_connectToType = tp; }

private:
    T                                                                 *m_item = nullptr;
    std::string                                                        m_connectToType;
    std::list<std::pair<Property<std::string>, std::function<void()>>> m_entries;
};

}  // namespace ara::proj
