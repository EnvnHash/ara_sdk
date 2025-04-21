//
// Created by user on 24.11.2020.
//

#pragma once

#include <UIApplication.h>
#include <Spinner.h>
#include <UITable.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <TabView.h>

#include "Demo/DemoView.h"

namespace ara {

class UI_Test_App : public UIApplication {
public:

    void init(std::function<void()> initCb);
    void createBaseUIElements();

private:

    TabView*		        ui_TabView=nullptr;
};

}
