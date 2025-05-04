
#include "UI_ElementsOverview.h"
#include <PropSlider.h>
#include <UI/UITable.h>
#include <UI/ComboBox.h>
#include <UI/Spinner.h>
#include <UI/TabView.h>

using namespace std;
using namespace ara;
using namespace glm;

void UI_ElementsOverview::init(std::function<void()> initCb) {
    m_mainWindow = addWindow(1600, 1000, 100, 100, false);
    auto root = m_mainWindow->getRootNode();
    root->setPadding(30);

    int heightStep = 35;

    addDiv(root, heightStep);
    addLabel(root, heightStep);
    addButton(root, heightStep);
    addSlider(root, heightStep);
    addPropSlider(root, heightStep);
    addComboBox(root, heightStep);
    addImage(root, heightStep);
    addImageButton(root, heightStep);
    addSpinner(root, heightStep);
    addEditor(root, heightStep);
    addInfoDialogButton(root, heightStep);
    addTable(root);
    addTabView(root);

    startRenderLoop();  // main UIApplication renderloop (for all windows) -> blocking
}

void UI_ElementsOverview::addDiv(UINode* root, int heightStep) {
    auto div = root->addChild<Div>(0, 0, 100, 25);
    div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
}

void UI_ElementsOverview::addLabel(UINode* root, int heightStep) {
    auto label = root->addChild<Label>(0, heightStep, 100, 30);
    label->setText("some Text");
}

void UI_ElementsOverview::addButton(UINode* root, int heightStep) {
    auto butt = root->addChild<Button>(0, heightStep * 2, 100, 30);
    butt->setText("Button");
    butt->setBackgroundColor(0.6f, 0.6f, 0.6f, 1.f);
    butt->addMouseClickCb([this](hidData* data) { LOG << "Button clicked!"; });
}

void UI_ElementsOverview::addSlider(UINode* root, int heightStep) {
    auto slider = root->addChild<Slider>(0, heightStep * 3, 300, 30);
    slider->setValue(0.f);
}

void UI_ElementsOverview::addPropSlider(UINode* root, int heightStep) {
    Property<float> fprop = { 0.5f };
    fprop.setMinMax(0.f, 1.f);

    auto propSlider = root->addChild<PropSlider>(0, heightStep * 4, 300, 50);
    propSlider->setLabel("Slider");
    propSlider->setProp(&fprop);
}

void UI_ElementsOverview::addComboBox(UINode* root, int heightStep) {
    auto combo = root->addChild<ComboBox>(0, heightStep * 6, 300, 50);
    combo->setMenuName("Combo");
    combo->addEntry("entry 1", [this] { LOG << "entry 1 clicked"; });
    combo->addEntry("entry 2", [this] { LOG << "entry 2 clicked"; });
    combo->addEntry("entry 3", [this] { LOG << "entry 3 clicked"; });
}

void UI_ElementsOverview::addImage(UINode* root, int heightStep) {
    auto image = root->addChild<Image>(0, heightStep * 16, 300, 200);
    image->setImg("test/test-tex.png", 4);
}

void UI_ElementsOverview::addImageButton(UINode* root, int heightStep) {
    auto imageButt = root->addChild<ImageButton>(0, heightStep * 8, 25, 25);
    imageButt->setIsToggle(true);
    imageButt->setImg("Icons/nvi_check_butt.png");
    imageButt->setOnStateImg("Icons/nvi_check_butt_active.png");
}

void UI_ElementsOverview::addSpinner(UINode* root, int heightStep) {
    auto spinner = root->addChild<Spinner>(0, heightStep * 9, 80, 80);
    spinner->setImage("test/spinner-1.png", 2);
}

void UI_ElementsOverview::addEditor(UINode* root, int heightStep) {
    auto edit = root->addChild<UIEdit>(0, heightStep * 12, 300, 30);
    edit->setText("Edit me!");
    edit->setBorderWidth(1);
    edit->setBorderColor(1.f, 1.f, 1.f, 1.f);
}

void UI_ElementsOverview::addInfoDialogButton(UINode* root, int heightStep) {
    auto butt2 = root->addChild<Button>(0, heightStep * 14, 100, 30);
    butt2->setText("Open Dialog");
    butt2->setBackgroundColor(0.6f, 0.6f, 0.6f, 1.f);
    butt2->setMouseClickCb([this](mouseClickData* data) {
        openModalDiag(infoDiagType::error, "Hello", [this] { LOG << "confirmed"; });
    });
}

void UI_ElementsOverview::addTable(UINode* root) {
    auto table = (UITable*)root->addChild(make_unique<UITable>(350.f, 0.f, 800.f, 300.f, 4, 3));
    table->t_setSpacing(8, 8);
    table->t_setMargins(5, 5);
    vec4 m_textColor = vec4(1.f, 1.f, 1.f, 1.f);

    for (int i = 0; i < table->getRowCount(); i++)
        for (int j = 0; j < table->getColumnCount(); j++) {
            auto l = table->setCell<Label>(i, j);
            l->setFont("regular", 22, align::left, valign::top, m_textColor);
            l->setBackgroundColor(.3f, .3f, .4f, 1.0f);
            l->setText("(" + std::to_string(i) + "," + std::to_string(j) + ")");
            l->setPadding(10);
        }
}

void UI_ElementsOverview::addTabView(UINode* root) {
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
}