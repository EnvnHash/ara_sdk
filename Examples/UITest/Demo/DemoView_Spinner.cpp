#include "DemoView.h"
#include <UIElements/Spinner.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Spinner::DemoView_Spinner() : DemoView("Spinner demo",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName("DemoView_Spinner");
}

void DemoView_Spinner::init() {
    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-1.png", .pos = {100, 100}, .size = {40, 40} });
    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-1.png", .pos = {100, 240}, .size = {80, 80} });

    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-2.png", .pos = {300, 100}, .size = {40, 40} });
    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-2.png", .pos = {300, 240}, .size = {80, 80} });

    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-3.png", .pos = {500, 100}, .size = {40, 40} });
    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-3.png", .pos = {500, 240}, .size = {80, 80} });

    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-4.png", .pos = {700, 100}, .size = {40, 40} });
    addChild<Spinner>(SpinnerPars{ .filepath = "Spinner/spinner-4.png", .pos = {700, 240}, .size = {80, 80} });
}
