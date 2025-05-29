//
// Created by user on 24.11.2020.
//

#pragma once

#include <UIApplication.h>

namespace ara {

class UITestApp : public UIApplication {
public:
    void init(std::function<void(UINode&)> initCb) override;
};

}
