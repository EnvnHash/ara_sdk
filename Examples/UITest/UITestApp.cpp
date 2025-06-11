//
// Created by user on 24.11.2020.
//

#include "UITestApp.h"
#include <UIElements/Label.h>
#include <UIElements/TabView.h>

#include "Demo/DemoView.h"

using namespace std;
using namespace ara;
using namespace glm;

void UITestApp::init(std::function<void(UINode&)>) {
    UIApplication::init([](UINode& rootNode){
        auto tabView = rootNode.addChild<TabView>(UINodePars{
            .size = vec2{1.f, 1.f},
            .fgColor = glm::vec4(.3f, .3f, .3f, 1.f),
            .bgColor = glm::vec4(.1f, .1f, .1f, 1.f)
        });
        tabView->setPadding(10.f);

        tabView->addTab<DemoView_Table>("Table");
        tabView->addTab<DemoView_Table_2>("Table 2");
        tabView->addTab<DemoView_Spinner>("Spinner");
        tabView->addTab<DemoView_ScrollView>("Scroll View");
        tabView->addTab<DemoView_ScrollView_2>("Scroll View 2");
        tabView->addTab<DemoView_ScrollView_3>("Scroll View 3");
        tabView->addTab<DemoView_ComboBox>("ComboBoxes");
        tabView->addTab<DemoView_Resources>("Resources");
#ifndef __ANDROID__
        tabView->addTab<DemoView_FloatingMenu>("Floating Menu");
#endif
        tabView->addTab<DemoView_Edit>("Edit");

        tabView->setActivateTab(0);
    });
}
