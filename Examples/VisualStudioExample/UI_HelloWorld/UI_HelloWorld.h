#pragma once

#include <UIApplication.h>
#include <Div.h>
#include <Label.h>
#include <Image.h>

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
