#pragma once

#include <UIApplication.h>

namespace ara {

class UIPropertyExample : public UIApplication {
public:
    void init(const std::function<void()>& initCb);

private:
    Property<float> prop {0.f, 0.f, 1.f, 0.1f};   // create a property {default-value, min, max, step}
};

}
