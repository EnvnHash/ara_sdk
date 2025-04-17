
#include "UI_ElementsOverview.h"
#include <PropSlider.h>
#include <UITable.h>

using namespace std;
using namespace ara;
using namespace ara;
using namespace ara;
using namespace ara;

void UI_ElementsOverview::init(std::function<void()> initCb)
{
    m_mainWindow = addWindow(1600, 1000, 100, 100, false);
    auto root = m_mainWindow->getRootNode();
    root->setPadding(30);

    int heightStep = 35;

    //---- Div (mostly used as a container for other elements) -----

    auto div = root->addChild<Div>(0, 0, 100, 25);
    div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);

    //---- Label ---------------------------------------------------

    auto label = root->addChild<Label>(0, heightStep, 100, 30);
    label->setText("some Text");

    //---- Button --------------------------------------------------

    auto butt = root->addChild<Button>(0, heightStep * 2, 100, 30);
    butt->setText("Button");
    butt->setBackgroundColor(0.6f, 0.6f, 0.6f, 1.f);
    butt->addMouseClickCb([this](hidData* data) { LOG << "Button clicked!"; });

    //---- Slider --------------------------------------------------

    auto slider = root->addChild<Slider>(0, heightStep * 3, 300, 30);   // add a slider at position (100|100), size (200|80)
    slider->setValue(0.f);

    //---- Slider with Label and numeric field ---------------------
    
    Property<float> fprop = { 0.5f };
    fprop.setMinMax(0.f, 1.f);

    auto propSlider = root->addChild<PropSlider>(0, heightStep *4, 300, 50);   // add a slider at position (100|100), size (200|80)
    propSlider->setLabel("Slider");
    propSlider->setProp(&fprop);

    //---- combo box -----------------------------------------------

    auto combo = root->addChild<ComboBox>(0, heightStep * 6, 300, 50);   // add a slider at position (100|100), size (200|80)
    combo->setMenuName("Combo");
    combo->addEntry("entry 1", [this] { LOG << "entry 1 clicked";  });
    combo->addEntry("entry 2", [this] { LOG << "entry 2 clicked";  });
    combo->addEntry("entry 3", [this] { LOG << "entry 3 clicked";  });

    //---- Image ---------------------------------------------------

    auto image = root->addChild<Image>(0, heightStep * 16, 300, 200);   // add a slider at position (100|100), size (200|80)
    image->setImg("test/test-tex.png", 4);

    //---- Image Button --------------------------------------------

    auto imageButt = root->addChild<ImageButton>(0, heightStep * 8, 25, 25);   // add a slider at position (100|100), size (200|80)
    imageButt->setIsToggle(true);
    imageButt->setImg("Icons/nvi_check_butt.png");
    imageButt->setOnStateImg("Icons/nvi_check_butt_active.png");

    //---- Spinner -------------------------------------------------

    auto spinner = root->addChild<Spinner>(0, heightStep * 9, 80, 80);
    spinner->setImage("test/spinner-1.png", 2);

    //---- Editor -------------------------------------------------

    auto edit = root->addChild<UIEdit>(0, heightStep * 12, 300, 30);
    edit->setText("Edit me!");
    edit->setBorderWidth(1);
    edit->setBorderColor(1.f, 1.f, 1.f, 1.f);

    //---- Info Dialog --------------------------------------------
    
    auto butt2 = root->addChild<Button>(0, heightStep * 14, 100, 30);
    butt2->setText("Open Dialog");
    butt2->setBackgroundColor(0.6f, 0.6f, 0.6f, 1.f);
    butt2->setMouseClickCb([this](mouseClickData* data) { 
        openModalDiag(infoDiagType::error, "Hello", [this] { LOG << "confirmed"; });
    });

    //---- Table ---------------------------------------------------

    auto table = (UITable*)root->addChild(make_unique<UITable>(350.f, 0.f, 800.f, 300.f, 4, 3));
    table->t_setSpacing(8, 8);
    table->t_setMargins(5, 5);
    vec4 m_textColor = vec4(1.f, 1.f, 1.f, 1.f);

    for (int i = 0; i < table->getRowCount(); i++)
        for (int j = 0; j < table->getColumnCount(); j++) 
        {
            auto l = table->setCell<Label>(i, j);
            l->setFont("regular", 22, align::left, valign::top, m_textColor);
            l->setBackgroundColor(.3f, .3f, .4f, 1.0f);
            l->setText("(" + std::to_string(i) + "," + std::to_string(j) + ")");
            l->setPadding(10);

        }
    
    //---- Tab View ------------------------------------------------
    
    vec4 fgCol = vec4(.3f, .3f, .3f, 1.f);
    vec4 bgCol = vec4(.1f, .1f, .1f, 1.f);
    auto tabView = root->addChild<TabView>(350, 400, 800, 300, fgCol, bgCol);
    tabView->setPadding(10.f);
    
    label = tabView->addTab<Label>("Tab 1");
    label->setText("TAB 1");

    label = tabView->addTab<Label>("Tab 2");
    label->setText("TAB 2");

    label = tabView->addTab<Label>("Tab 3");
    label->setText("TAB 3"); 

    startRenderLoop();  // main UIApplication renderloop (for all windows) -> blocking
}
