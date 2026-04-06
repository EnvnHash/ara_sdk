#include "UIPropertyExample.h"

using namespace ara;

int main(int, char*) {
    UIPropertyExample app;
    app.init(nullptr);
    app.startEventLoop(); //blocking

    return 0;
}

