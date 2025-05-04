#include "DemoView.h"
#include <Spinner.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Spinner::DemoView_Spinner() : DemoView("Spinner demo",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName("DemoView_Spinner");
}

void DemoView_Spinner::init() {
    addChild<Spinner>(100, 100, 40, 40, "Spinner/spinner-1.png");
	addChild<Spinner>(100, 240, 80, 80, "Spinner/spinner-1.png");

	addChild<Spinner>(300, 100, 40, 40, "Spinner/spinner-2.png");
	addChild<Spinner>(300, 240, 80, 80, "Spinner/spinner-2.png");

	addChild<Spinner>(500, 100, 40, 40, "Spinner/spinner-3.png");
	addChild<Spinner>(500, 240, 80, 80, "Spinner/spinner-3.png");

	addChild<Spinner>(700, 100, 40, 40, "Spinner/spinner-4.png");
	addChild<Spinner>(700, 240, 80, 80, "Spinner/spinner-4.png");
}
