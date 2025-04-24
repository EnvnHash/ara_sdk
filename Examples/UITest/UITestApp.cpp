//
// Created by user on 24.11.2020.
//

#include "UITestApp.h"
#include <Label.h>
#include <TabView.h>
#include <ScrollBar.h>

using namespace std;
using namespace ara;
using namespace ara;
using namespace ara;

void UI_Test_App::init(std::function<void()> initCb) {
    // create the main UI-Window must be on the same thread on that glbase.init happened to have context-sharing work
    m_mainWindow = addWindow(1280, 700, 50, 50, false);
    m_mainWindow->addGlobalKeyUpCb(this, [this](const hidData* data){
        // dump the sceneTree
        if (data->key == GLSG_KEY_D) {
            if (m_mainWindow->getWinHandle()) {
                m_mainWindow->getRootNode()->dump();
            }
            return;
        }
    });

    createBaseUIElements();
    startRenderLoop();  // main UIApplication render loop (for all windows) -> blocking
}

void UI_Test_App::createBaseUIElements() {
    auto rootNode = m_mainWindow->getRootNode();

    ui_TabView = rootNode->addChild<TabView>(10, 10, 1200, 700, glm::vec4(.3f, .3f, .3f, 1.f), glm::vec4(.1f, .1f, .1f, 1.f));
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
    ui_TabView->addTab<DemoView_FloatingMenu>("Floating Menu");
    ui_TabView->addTab<DemoView_Edit>("Edit");

    ui_TabView->setActivateTab(4);
}
