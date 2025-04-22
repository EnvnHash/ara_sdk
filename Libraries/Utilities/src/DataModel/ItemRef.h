
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <DataModel/Item.h>
#include <Property.h>

namespace ara {

class ItemRef {
public:
    explicit ItemRef(ItemUi *item = nullptr) : m_item(item) {}

    virtual ~ItemRef() {
        for (auto &it : m_onPreChangedCb) it.second.reset();
    }

    virtual void setItem(ItemUi *item) { m_item = item; }

    // utility function for connecting to properties. stores a local callback
    // function add passes it as to the property which will store a reference to
    // it as a weak pointer on dtor the referring pointer is freed and by
    // weak_ptr.lock() checking it's weak_ptr reference inside the Property is
    // deleted implicitly
    template <typename T>
    void onPreChange(Property<T> *p, std::function<void(std::any)> f) {
        m_onPreChangedCb[p] = std::make_shared<std::function<void(std::any)>>(f);
        if (p) p->onPreChange(m_onPreChangedCb[p]);
    }

    template <typename T>
    void onPreChange(PropertyPtr<T> &p, std::function<void(std::any)> f) {
        m_onPreChangedCb[p.ptr] = std::make_shared<std::function<void(std::any)>>(f);
        if (p.ptr) p.onPreChange(m_onPreChangedCb[p.ptr]);
    }

    template <typename T>
    void onPostChange(PropertyPtr<T> &p, std::function<void(std::any)> f) {
        m_onPostChangedCb[p.ptr] = std::make_shared<std::function<void(std::any)>>(f);
        if (p.ptr) p.onPostChange(m_onPostChangedCb[p.ptr]);
    }

protected:
    ItemUi                                                                    *m_item = nullptr;
    std::unordered_map<void *, std::shared_ptr<std::function<void(std::any)>>> m_onPreChangedCb;
    std::unordered_map<void *, std::shared_ptr<std::function<void(std::any)>>> m_onPostChangedCb;
};

}  // namespace ara