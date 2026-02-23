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

#include <UIElements/Button/Button.h>
#include <UIElements/ScrollView.h>
#include <UISharedRes.h>
#include <UIElements/Image.h>
#include "UIElements/Menu/TreeCollapsible.h"

#include "clip.h"
#include "UIElements/Button/ImageButton.h"

using namespace glm;
using namespace std;

namespace ara {

TreeCollapsible::TreeCollapsible() {
    setTypeName<TreeCollapsible>();
    setName(getTypeName<TreeCollapsible>());
    setScissorChildren(false);
}

void TreeCollapsible::setNode(Node* node) {
    m_tree = node;

    addGlCb("rebuildTreeCollapsible", [this] {
        rebuild();
        return true;
    });

    m_reqRebuild = true;
    setDrawFlag();
}

void TreeCollapsible::rebuild() {
    rebuildUiElements();

    m_collapseState.clear();
    rebuildCollapseState(m_tree);

    m_reqRebuild = false;
}

void TreeCollapsible::rebuildUiElements() {
    clearChildren();
    int32_t yOffs = 0;
    rebuildIt(m_tree, 0, yOffs);
}

void TreeCollapsible::rebuildIt(Node* nd, size_t tabIdx, int32_t& yOffs) {
    if (nd) {
        if (!nd->children().empty()) {
            auto& arrowButt = push<ImageButton>(UINodePars{
                .pos = ivec2{ static_cast<int32_t>(11 + tabIdx * 20), yOffs },
                .size = ivec2{ 12, m_fontHeight },
                .align = align::left,
                .valign = valign::top,
            });

            arrowButt.setIsToggle(true);
            arrowButt.setImg("Icons/icon-arrow-right.png", 1);
            arrowButt.setOnStateImg("Icons/icon-arrow-down.png", 1);

            arrowButt.setToggleState(m_collapseState[nd->uuid()]);
            arrowButt.setClickedCb([this, nd] {
                addGlCb("TreeCollapsible::rebuildTree", [this, nd] {
                    m_collapseState[nd->uuid()] = !m_collapseState[nd->uuid()];
                    rebuildUiElements();
                    return true;
                });
            });
        }

        push<Label>(LabelPars{
            .pos = ivec2{ 30 + tabIdx * 20, yOffs },
            .size = ivec2{ 200, m_fontHeight },
            .align = align::left,
            .valign = valign::top,
            .text_color = m_color,
            .text = nd->name(),
            .text_align_x = align::left,
            .font_type = "regular",
            .font_height = m_fontHeight
        });

        yOffs += m_fontHeight + m_ySpacing;

        if (m_collapseState[nd->uuid()]) {
            for (auto& c : nd->children()) {
                rebuildIt(c.get(), tabIdx +1, yOffs);
            }
        }
    }
}

void TreeCollapsible::rebuildCollapseState(Node* nd) {
    if (nd && !nd->children().empty()) {
        m_collapseState.emplace(nd->uuid(), false);
        for (auto& c : nd->children()) {
            rebuildCollapseState(c.get());
        }
    }
}

}