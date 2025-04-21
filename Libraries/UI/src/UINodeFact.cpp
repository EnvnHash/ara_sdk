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

namespace ara {

std::unique_ptr<UINode> UINodeFact::Create(const std::string &className) {
    if (className == "button") {
        return std::make_unique<ImageButton>();
    } else if (className == "comboBox") {
        return std::make_unique<ComboBox>();
    } else if (className == "crosshair") {
        return std::make_unique<CrossHair>();
    } else if (className == "grid") {
        return std::make_unique<Grid>();
    } else if (className == "div") {
        return std::make_unique<Div>();
    } else if (className == "dropDownMenu") {
        return std::make_unique<DropDownMenu>();
    } else if (className == "dropDownMenuBar") {
        return std::make_unique<DropDownMenuBar>();
    } else if (className == "gizmo") {
        return std::make_unique<Gizmo>();
    } else if (className == "img") {
        return std::make_unique<Image>();
    } else if (className == "label") {
        return std::make_unique<Label>();
    } else if (className == "menuBar") {
        return std::make_unique<MenuBar>();
    } else if (className == "num_input") {
        return std::make_unique<UIEdit>();
    } else if (className == "polygon") {
        return std::make_unique<UIPolygon>();
    } else if (className == "slider") {
        return std::make_unique<Slider>();
    } else if (className == "propSlider") {
        return std::make_unique<PropSlider>();
    } else if (className == "edit") {
        return std::make_unique<UIEdit>();
    } else if (className == "node") {
        return std::make_unique<UINode>();
    } else if (className == "scrollView") {
        return std::make_unique<ScrollView>();
    } else if (className == "spinner") {
        return std::make_unique<Spinner>();
    } else if (className == "spinner") {
        return std::make_unique<Spinner>();
    } else if (className == "tabView") {
        return std::make_unique<TabView>();
    } else if (className == "table") {
        return std::make_unique<UITable>();
    }
    return std::make_unique<UINode>();
}

}  // namespace ara
