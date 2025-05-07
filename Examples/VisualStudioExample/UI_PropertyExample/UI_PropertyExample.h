#pragma once

#include <UIApplication.h>
#include <UIElements/TabView.h>

namespace ara {

    class UI_PropertyExample : public UIApplication {
    public:
        void init(std::function<void()> initCb);

    private:
    };
}
