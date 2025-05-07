#pragma once

#include <UIApplication.h>
#include <UIElements/Div.h>
#include <UIElements/Label.h>
#include <UIElements/Image.h>

namespace ara {

    class UI_HelloWorld : public UIApplication {
    public:

        void init(std::function<void()> initCb);
        void createContainerDiv();
        void createStyleSheetDiv();
        void createImages();

    private:
        Div* m_div = nullptr;
    };

}
