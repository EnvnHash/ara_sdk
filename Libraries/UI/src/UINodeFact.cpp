//
// Created by sven on 22-07-22.
//

#include <UIElements/Button/Button.h>
#include <UIElements/ComboBox.h>
#include <UIElements/CrossHair.h>
#include <UIElements/DropDownMenu.h>
#include <UIElements/DropDownMenuBar.h>
#include <UIElements/Gizmo.h>
#include <UIElements/Grid.h>
#include <UIElements/Image.h>
#include <UIElements/Button/ImageButton.h>
#include <UIElements/Label.h>
#include <UIElements/WindowElements/MenuBar.h>
#include <UIElements/PropSlider.h>
#include <UIElements/ScrollBar.h>
#include <UIElements/ScrollView.h>
#include <UIElements/Slider.h>
#include <UIElements/Spinner.h>
#include <UIElements/TabView.h>
#include <UIElements/UIEdit.h>
#include <UINodeFact.h>
#include <UIElements/UIPolygon.h>
#include <UIElements/UITable.h>
#include <UIElements/WindowElements/WindowResizeArea.h>
#include <UIElements/WindowElements/WindowResizeAreas.h>
#include <Utils/PingPongFbo.h>
#include <DrawManagers/DrawManagerGizmo.h>

namespace ara {

std::unique_ptr<UINode> UINodeFact::Create(const std::string &className) {
    static std::unordered_map<std::string, std::function<std::unique_ptr<UINode>()> > objMap = {
        {"button", [] { return std::make_unique<ImageButton>(); }},
        {"comboBox", [] { return std::make_unique<ComboBox>(); }},
        {"crosshair", [] { return std::make_unique<CrossHair>(); }},
        {"grid", [] { return std::make_unique<Grid>(); }},
        {"div", [] { return std::make_unique<Div>(); }},
        {"dropDownMenu", [] { return std::make_unique<DropDownMenu>(); }},
        {"dropDownMenuBar", [] { return std::make_unique<DropDownMenuBar>(); }},
        {"gizmo", [] { return std::make_unique<Gizmo>(); }},
        {"img", [] { return std::make_unique<Image>(); }},
        {"label", [] { return std::make_unique<Label>(); }},
        {"menuBar", [] { return std::make_unique<MenuBar>(); }},
        {"num_input", [] { return std::make_unique<UIEdit>(); }},
        {"polygon", [] { return std::make_unique<UIPolygon>(); }},
        {"slider", [] { return std::make_unique<Slider>(); }},
        {"propSlider", [] { return std::make_unique<PropSlider>(); }},
        {"edit", [] { return std::make_unique<UIEdit>(); }},
        {"node", [] { return std::make_unique<UINode>(); }},
        {"scrollView", [] { return std::make_unique<ScrollView>(); }},
        {"spinner", [] { return std::make_unique<Spinner>(); }},
        {"tabView", [] { return std::make_unique<TabView>(); }},
        {"table", [] { return std::make_unique<UITable>(); }},
    };

    return objMap[className]();
}

}  // namespace ara
