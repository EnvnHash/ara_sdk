#include "PageView.h"

using namespace glm;
using namespace std;

namespace ara {

PageView::PageView() : Div() {
    setName(getTypeName<PageView>());
    setFocusAllowed(false);

    m_content = addChild<Div>();
    m_content->setName("PageView_Content");
}

UINode* PageView::addPage(std::unique_ptr<UINode> child) {
    auto newNode = m_content->addChild(std::move(child));
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
