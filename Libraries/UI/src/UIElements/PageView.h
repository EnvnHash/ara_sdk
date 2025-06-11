#pragma once

#include "Button/Button.h"

namespace ara {

class PageView : public Div {
public:
    PageView();
    ~PageView() override = default;

    virtual UINode* addPage(std::unique_ptr<UINode> child);
    virtual void    showPage(int idx);

    [[nodiscard]] size_t getNrPages() const { return m_pages.size(); }

    template <class T>
    T* addPage(const std::string& styleClass) {
        for (const auto& it : m_pages) {
            it->setVisibility(false);
        }

        auto newNode = m_content->addChild(std::make_unique<T>());
        newNode->addStyleClass(styleClass);
        m_pages.emplace_back(newNode);
        m_pages.back()->setVisibility(true);
        return static_cast<T*>(newNode);
    }

protected:
    std::vector<UINode*> m_pages;
    Div*                 m_content = nullptr;
    Div*                 m_controls = nullptr;
};

}  // namespace ara
