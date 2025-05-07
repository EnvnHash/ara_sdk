//
// Created by user on 24.11.2020.
//

#pragma once

#include <UIApplication.h>
#include <UIElements/Spinner.h>
#include <UIElements/UITable.h>
#include <UIElements/ScrollBar.h>
#include <UIElements/ScrollView.h>
#include <UIElements/TabView.h>

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
