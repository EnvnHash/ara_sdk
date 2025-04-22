
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

#include <util_common.h>

namespace ara {

template <typename T>
class ListProperty {
public:
    ListProperty() = default;

    ListProperty &operator=(std::initializer_list<T> &l) {
        m_list = l;
        procCb();
        return *this;
    }

    ListProperty &operator=(std::list<T> &l) {
        m_list = l;
        procCb();
        return *this;
    }

    ListProperty &operator=(std::list<T> &&l) {
        m_list = l;
        procCb();
        return *this;
    }

    void push_back(const T &i, bool doProcCb = true) {
        m_list.emplace_back(i);
        if (doProcCb) {
            procCb();
        }
    }

    void pop_back(bool doProcCb = true) {
        m_list.pop_back();
        if (doProcCb) {
            procCb();
        }
    }

    void emplace_back(const T &i, bool doProcCb = true) {
        m_list.emplace_back(i);
        if (doProcCb) {
            procCb();
        }
    }

    void procCb() {
        // call the onChange callbacks (m_valChgChecking and m_valPreChange)
        for (auto it = m_valChgChecking.begin(); it != m_valChgChecking.end();)
            if (auto p = it->lock()) {
                p->operator()(std::any(&m_list));
                ++it;
            } else {
                it = m_valChgChecking.erase(it);
            }

        for (auto &it : m_valChanged) {
            it.second(&m_list);
        }
    }

    void                            clear() { m_list.clear(); }
    T                              &front() { return m_list.front(); }
    T                              &back() { return m_list.back(); }
    typename std::list<T>::iterator begin() { return m_list.begin(); }
    typename std::list<T>::iterator end() { return m_list.end(); }
    size_t                          size() { return m_list.size(); }
    bool                            empty() { return m_list.empty(); }
    std::list<T>                   &operator()() { return m_list; }
    std::list<T>                   &get() { return m_list; }
    // operator T const& () const               { return m_list; }
    typename std::list<T>::iterator insert(typename std::list<T>::iterator pos, const T &value) {
        return m_list.insert(pos, value);
    }
    // std::ostream& operator << (std::ostream& out) { out << m_list; return
    // out; }

    /** add a callback which will be called when the property changes it's
     * value. The shared_ptr is store as a weak_ptr and remove from the list,
     * when it can't be locked, which means that it's owner was deleted */
    void onChanged(std::shared_ptr<std::function<void(std::any)>> funcPtr) {
        m_valChgChecking.emplace_back(funcPtr);
    }

    /** add a callback which will be called when the property changes it's
     * value. no checking is done if the class instance passing this callback
     * still exists. It has to be removed when the caller is destroyed with
     * removeOnPreChange() This is meant to be used when the property is part of
     * the same class as the callback */
    void onChanged(std::function<void(std::list<T> *)> func, void *ptr) {
        m_valChanged[ptr] = func;
    }

    void removeOnValueChanged(void *ptr) {
        if (ptr) {
            auto it = m_valChanged.find(ptr);
            if (it != m_valChanged.end()) {
                m_valChanged.erase(it);
            }
        }
    }

private:
    std::list<T>                                                    m_list;
    std::list<std::weak_ptr<std::function<void(std::any)>>>         m_valChgChecking;
    std::unordered_map<void *, std::function<void(std::list<T> *)>> m_valChanged;
};
}  // namespace ara