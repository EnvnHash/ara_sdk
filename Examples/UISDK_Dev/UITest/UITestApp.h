//
// Created by user on 24.11.2020.
//

#pragma once

#include <UIApplication.h>
#include <UI/Spinner.h>
#include <UI/UITable.h>
#include <UI/ScrollBar.h>
#include <UI/ScrollView.h>
#include <UI/TabView.h>

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
