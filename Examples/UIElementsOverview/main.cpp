#include "UIElementsOverview.h"

using namespace ara;

int main(int, char**) {
    UIElementsOverview app;
    app.init(nullptr);
    app.startEventLoop(); //blocking
    return 0;
}

