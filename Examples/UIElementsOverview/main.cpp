#include <UITestApp.h>
#include "Demo/DemoView.h"

using namespace ara;

int main(int, char**) {
    UITestApp app;
	app.init(nullptr);
	app.startEventLoop(); //blocking
	return 0;
}

