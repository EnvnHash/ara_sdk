#include "DemoView.h"
#include <UIWindow.h>
#include <UIApplication.h>
#include "UI/Button/Button.h"

using namespace glm;
using namespace std;
using namespace ara;
using namespace ara;

#ifndef __ANDROID__

DemoView_FloatingMenu::DemoView_FloatingMenu() : DemoView("FloatingMenu",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_FloatingMenu>());
}

void DemoView_FloatingMenu::init()
{
    auto div = addChild<Div>();
    div->setAlign(align::center, valign::center);
    div->setSize(0.8f, 0.8f);
    div->setBorderWidth(10);
    div->setBorderColor(0.7f, 0.7f, 0.7f, 1.f);
    div->setBackgroundColor(0.3f, 0.3f, 0.3f, 1.f);

    div->addMouseClickRightCb([this](hidData* data)
    {
        runOnMainThread([this, data] {
           auto rightClickMen = getApp()->addWindow<FloatingMenuDialog>(200, 96, data->screenMousePos.x, data->screenMousePos.y, false);

           rightClickMen->addGlCb(this, "add", [rightClickMen]{
               auto but = rightClickMen->addItem<Button>("Value1");
               but->setText("Value 1");

               auto but2 = rightClickMen->addItem<Button>("Value2");
               but2->setText("Value 2");

               auto but3 = rightClickMen->addItem<Button>("Value3");
               but3->setText("Value 3");

               // should be done by stylesheet
               vector<Button*> buts = {but, but2, but3};
               for (int i=0; i<buts.size(); i++){
                   buts[i]->setHeight( 30);
                   buts[i]->setY(i * 32);
                   buts[i]->setBackgroundColor(0.4f, 0.4f, 0.4f, 1.f);
                   buts[i]->setColor(0.0f, 0.0f, 0.7f, 1.f, state::highlighted);
               }

               return true;
           });

           rightClickMen->setOnClose([](string returnValue){
               LOG << "returnValue: " << returnValue;
           });

           return true;
       });
  });

}


#endif

