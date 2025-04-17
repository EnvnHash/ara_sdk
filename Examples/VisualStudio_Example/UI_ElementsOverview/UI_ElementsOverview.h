#pragma once

#include <UIApplication.h>

namespace ara {

    class UI_ElementsOverview : public UIApplication {
    public:
        void init(std::function<void()> initCb);

    private:
    };
}
