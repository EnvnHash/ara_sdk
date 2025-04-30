#pragma once

#include "Table.h"
#include "Button/Button.h"

namespace ara {

class PageView : public Div {
public:
    PageView();
    virtual ~PageView() = default;

    virtual UINode* addPage(std::unique_ptr<UINode> child);
    virtual void    showPage(int idx);
    size_t          getNrPages() { return m_pages.size(); }

    template <class T>
    T* addPage(std::string* styleClass) {
        for (const auto& it : m_pages) it->setVisibility(false);

        auto newNode = m_content->addChild(std::make_unique<T>());
        newNode->addStyleClass(std::move(*styleClass));
        m_pages.emplace_back(newNode);
        m_pages.back()->setVisibility(true);
        return (T*)newNode;
    }

    template <class T>
    T* addPage(std::string&& styleClass) {
        for (const auto& it : m_pages) it->setVisibility(false);

        auto newNode = m_content->addChild(std::make_unique<T>());
        newNode->addStyleClass(std::move(styleClass));
        m_pages.emplace_back(newNode);
        m_pages.back()->setVisibility(true);
        return (T*)newNode;
    }

protected:
    std::vector<UINode*> m_pages;
    Div*                 m_content = nullptr;
    Div*                 m_controls = nullptr;
};

}  // namespace ara
