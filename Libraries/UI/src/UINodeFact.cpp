//
// Created by sven on 22-07-22.
//

#include "Button/Button.h"
#include <ComboBox.h>
#include <CrossHair.h>
#include <DropDownMenu.h>
#include <DropDownMenuBar.h>
#include <Gizmo.h>
#include <Grid.h>
#include <Image.h>
#include "Button/ImageButton.h"
#include <Label.h>
#include "WindowElements/MenuBar.h"
#include <PropSlider.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Slider.h>
#include <Spinner.h>
#include <TabView.h>
#include <UIEdit.h>
#include <UINodeFact.h>
#include <UIPolygon.h>
#include <UITable.h>
#include "WindowElements/WindowResizeArea.h"
#include "WindowElements/WindowResizeAreas.h"
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
