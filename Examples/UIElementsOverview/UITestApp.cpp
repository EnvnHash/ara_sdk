//
// Created by user on 24.11.2020.
//

#include "UITestApp.h"
#include <UIElements/Text/Label.h>
#include <UIElements/TabView.h>

#include "Demo/DemoView.h"

using namespace std;
using namespace ara;
using namespace glm;

void UITestApp::init(std::function<void(UINode&)>) {
    UIApplication::init([](UINode& rootNode){
        auto& tabView = rootNode.push<TabView>(UINodePars{
            .size = vec2{1.f, 1.f},
            .fgColor = vec4(.3f, .3f, .3f, 1.f),
            .bgColor = vec4(.1f, .1f, .1f, 1.f)
        });
        tabView.setPadding(10.f);

        tabView.addTab<DemoView_Carrousel>("Carrousel");
        tabView.addTab<DemoView_Collapsibles>("Collapsibles");
        tabView.addTab<DemoView_DataBinding>("Data Binding");
        tabView.addTab<DemoView_Edit>("Edit");
#ifndef __ANDROID__
        tabView.addTab<DemoView_FloatingMenu>("Floating Menu");
#endif
        tabView.addTab<DemoView_Resources>("Resources");
        tabView.addTab<DemoView_Resizable>("Resizable");
        tabView.addTab<DemoView_ScrollView>("Scroll View");
        tabView.addTab<DemoView_ScrollView_2>("Scroll View 2");
        tabView.addTab<DemoView_ScrollView_3>("Scroll View 3");
        tabView.addTab<DemoView_ScrollViewList>("Scroll View List");
        tabView.addTab<DemoView_Spinner>("Spinner");
        tabView.addTab<DemoView_Table>("Table");
        tabView.addTab<DemoView_Table_2>("Table 2");
        tabView.addTab<DemoView_ZoomView>("ZoomView");

        tabView.setActivateTab(0);
    });
}
