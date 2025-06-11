#include "DemoView.h"
#include <UIElements/Spinner.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Spinner::DemoView_Spinner() : DemoView("Spinner demo",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName("DemoView_Spinner");
}

void DemoView_Spinner::init() {
    addChild<Spinner>("Spinner/spinner-1.png", 100, 100, 40, 40);
	addChild<Spinner>("Spinner/spinner-1.png", 100, 240, 80, 80);

	addChild<Spinner>("Spinner/spinner-2.png", 300, 100, 40, 40);
	addChild<Spinner>("Spinner/spinner-2.png", 300, 240, 80, 80);

	addChild<Spinner>("Spinner/spinner-3.png", 500, 100, 40, 40);
	addChild<Spinner>("Spinner/spinner-3.png", 500, 240, 80, 80);

	addChild<Spinner>("Spinner/spinner-4.png", 700, 100, 40, 40);
	addChild<Spinner>("Spinner/spinner-4.png", 700, 240, 80, 80);
}
