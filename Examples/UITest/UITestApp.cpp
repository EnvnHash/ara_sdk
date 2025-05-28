//
// Created by user on 24.11.2020.
//

#include "UITestApp.h"
#include <UIElements/Label.h>
#include <UIElements/TabView.h>

#include "Demo/DemoView.h"

using namespace std;
using namespace ara;
using namespace ara;
using namespace ara;

void UITestApp::init(std::function<void(UINode*)>) {
    UIApplication::init([](UINode* rootNode){
        auto ui_TabView = rootNode->addChild<TabView>(10, 10, 1200, 700, glm::vec4(.3f, .3f, .3f, 1.f), glm::vec4(.1f, .1f, .1f, 1.f));
        ui_TabView->setPos(0,0);
        ui_TabView->setSize(1.f,1.f);
        ui_TabView->setPadding(10.f);

        ui_TabView->addTab<DemoView_Table>("Table");
        ui_TabView->addTab<DemoView_Table_2>("Table 2");
        ui_TabView->addTab<DemoView_Spinner>("Spinner");
        ui_TabView->addTab<DemoView_ScrollView>("Scroll View");
        ui_TabView->addTab<DemoView_ScrollView_2>("Scroll View 2");
        ui_TabView->addTab<DemoView_ScrollView_3>("Scroll View 3");
        ui_TabView->addTab<DemoView_ComboBox>("ComboBoxes");
        ui_TabView->addTab<DemoView_Resources>("Resources");
#ifndef __ANDROID__
        ui_TabView->addTab<DemoView_FloatingMenu>("Floating Menu");
#endif
        ui_TabView->addTab<DemoView_Edit>("Edit");

        ui_TabView->setActivateTab(0);
    });
}
