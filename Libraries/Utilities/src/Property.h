
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

/*!
 *\brief A general class reactive Properties. An arbitrary amount of functions can be bound for pre- and post value changes
 */

#pragma once

#include <algorithm>
#include <any>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

#include <json/json.hpp>

namespace ara {

template<typename T>
class Property {
public:
    Property() = default;

    Property(T val) {
        m_value = val;
    }

    Property(T val, T min, T max, T step) {
        m_value = val;
        m_min   = min;
        m_max   = max;
        m_step  = step;
    }

    T &operator=(const T &i) {
        return set(i);
    }

    T &set(const T &i, bool procCb = true) {
        if (m_value != i && procCb) {
            callOnPreChange(i);
            m_value = i;
            callOnPostChange();
        }

        return m_value;
    }

    T &setClamp(const T &i) {
        if (m_value != i) {
            T tempVal = std::clamp(i, m_min, m_max);
            return set(tempVal);
        }
        return m_value = i;
    }

    void setStep(T v) { m_step = v; }

    // Implicit conversion back to T
    operator T const &() const { return m_value; }

    T &operator()() { return m_value; }

    T &get() { return m_value; }

    void setMinMax(T min, T max) {
        m_min = min;
        m_max = max;
    }

    T &getMin() { return m_min; }

    T &getMax() { return m_max; }

    T &getStep() { return m_step; }

    void callOnPreChange(const T &i) {
        // call the onChange callbacks (m_valChgChecking and m_valPreChange)
        for (auto it = m_valPreChecking.begin(); it != m_valPreChecking.end();)
            if (auto p = it->lock()) {
                p->operator()(std::any(i));
                ++it;
            } else {
                it = m_valPreChecking.erase(it);
            }

        for (auto &it: m_valPreChange) {
            it.second(i);
        }
    }

    void callOnPostChange() {
        // call the onChange callbacks (m_valChgChecking and m_valPreChange)
        for (auto it = m_valPostChecking.begin(); it != m_valPostChecking.end();)
            if (auto p = it->lock()) {
                p->operator()(std::any(m_value));
                ++it;
            } else {
                it = m_valPostChecking.erase(it);
            }

        for (auto &it: m_valPostChange) {
            it.second();
        }
    }

    std::ostream &operator<<(std::ostream &out) {
        out << m_value;
        return out;
    }

    /** add a callback which will be called before the property changes it's
     * value. The shared_ptr is store as a weak_ptr and remove from the list,
     * when it can't be locked, which means that it's owner was deleted */
    void onPreChange(std::shared_ptr<std::function<void(std::any)>> funcPtr) {
        m_valPreChecking.emplace_back(funcPtr);
    }

    /** add a callback which will be called when the property value is changed,
     * before m_value is set. the new value is passed as arg. no checking is
     * done if the class instance passing this callback still exists. It has to
     * be removed when the caller is destroyed with removeOnPreChange() This is
     * meant to be used when the property is part of the same class as the
     * callback */
    void onPreChange(std::function<void(const T &)> func, void *ptr) {
        m_valPreChange[ptr] = func;
    }

    /** add a callback which will be called after the property changed its
     * value. The shared_ptr is store as a weak_ptr and remove from the list,
     * when it can't be locked, which means that it's owner was deleted */
    void onPostChange(std::shared_ptr<std::function<void(std::any)>> funcPtr) {
        m_valPostChecking.emplace_back(funcPtr);
    }

    /** add a callback which will be called when the property value is changed,
     * after m_value is set. no checking is done if the class instance passing
     * this callback still exists. It has to be removed when the caller is
     * destroyed with removeOnPostChange() This is meant to be used when the
     * property is part of the same class as the callback */
    void onPostChange(const std::function<void()>& func, void *ptr) {
        m_valPostChange[ptr] = func;
    }

    void removeOnPreChange(void *ptr) {
        if (ptr) {
            auto it = m_valPreChange.find(ptr);
            if (it != m_valPreChange.end()) {
                m_valPreChange.erase(it);
            }
        }
    }

    void removeOnPostChange(void *ptr) {
        if (ptr) {
            auto it = m_valPostChange.find(ptr);
            if (it != m_valPostChange.end()) {
                m_valPostChange.erase(it);
            }
        }
    }

    T m_value;
    T m_min;
    T m_max;
    T m_step;

    void setIsAttrib(bool b) { m_isAttrib = true; }
    bool isAttrib(bool b) const { return m_isAttrib; }

private:
    std::list<std::weak_ptr<std::function<void(std::any)>>> m_valPreChecking;
    std::list<std::weak_ptr<std::function<void(std::any)>>> m_valPostChecking;
    std::unordered_map<void *, std::function<void(const T &)>> m_valPreChange;
    std::unordered_map<void *, std::function<void()>> m_valPostChange;
    bool m_isAttrib = false;
};

template <typename T>
void to_json(nlohmann::json& j, const Property<T>& p) {
    j = nlohmann::json{{"value", p.m_value}, {"min", p.m_min}, {"max", p.m_max}, {"step", p.m_step}};
}

template <typename T>
void from_json(const nlohmann::json& j, Property<T>& p) {
    j.at("value").get_to(p.get());
    j.at("min").get_to(p.getMin());
    j.at("max").get_to(p.getMax());
    j.at("step").get_to(p.getStep());
}

template <typename T>
class PropertyPtr {
public:
    PropertyPtr() = default;
    PropertyPtr(Property<T> *p) { ptr = p; }
    PropertyPtr(const PropertyPtr<T> &p) { ptr = p.ptr; }
    void operator=(PropertyPtr<T> &p) { ptr = p.ptr; }
    void operator=(Property<T> *p) { ptr = p; }

    void operator=(const T &i) {
        if (ptr) {
            ptr->set(i);
        }
    }

    operator T const &() const {
        return ptr->get();
    }

    T &operator()() {
        return ptr->get();
    }

    T &get() {
        return reinterpret_cast<T&>(*ptr);
    }

    void setMinMax(T min, T max) {
        if (ptr) {
            ptr->setMinMax(min, max);
        }
    }

    void setStep(T v) {
        if (ptr) {
            ptr->setStep(v);
        }
    }

    void onPreChange(std::shared_ptr<std::function<void(std::any)>> funcPtr) {
        if (ptr) {
            ptr->onPreChange(funcPtr);
        }
    }

    void onPostChange(std::shared_ptr<std::function<void(std::any)>> funcPtr) {
        if (ptr) {
            ptr->onPostChange(funcPtr);
        }
    }

    void onPreChange(std::function<void(const T &)> func, void *p) {
        if (ptr) {
            ptr->onPreChange(func, p);
        }
    }

    bool valid() { return ptr != nullptr; }

    Property<T> *ptr = nullptr;
};

}  // namespace ara