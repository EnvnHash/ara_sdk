#include "PageView.h"

using namespace glm;
using namespace std;

namespace ara {

PageView::PageView() {
    setName(getTypeName<PageView>());
    setFocusAllowed(false);

    auto& n = push<Div>();
    m_content = &n;
    m_content->setName("PageView_Content");
}

UINode* PageView::addPage(std::shared_ptr<UINode> child) {
    auto newNode = &m_content->push(std::move(child));
    m_pages.emplace_back(newNode);
    return newNode;
}

void PageView::showPage(int idx) {
    if (idx >= m_pages.size()) {
        return;
    }

    for (const auto& it : m_pages) {
        it->setVisibility(false);
    }

    m_pages[idx]->setVisibility(true);
}

}
