#include <JsonEditorDemo.h>

using namespace ara;

int main(int, char**) {
    JsonEditorDemo app;
	app.init(nullptr);
	app.startEventLoop(); //blocking
	return 0;
}

