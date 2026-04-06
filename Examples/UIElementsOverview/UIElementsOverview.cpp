
#include "UIElementsOverview.h"
#include <UIElements/DataBinding/PropSlider.h>
#include <UIElements/UITable.h>
#include <UIElements/Menu/ComboBox.h>
#include <UIElements/Spinner.h>
#include <UIElements/TabView.h>
#include <UIElements/Button/ImageButton.h>

using namespace std;
using namespace ara;
using namespace glm;

void UIElementsOverview::init(const std::function<void()>&) {
    UIApplication::init([this](UINode& root) {
        root.setPadding(30, 30, 30, 30);

        constexpr int heightStep = 35;

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
    });
}

void UIElementsOverview::addDiv(UINode& root, int) {
    root.push<Div>({
        .pos = ivec2{0, 0},
        .size = ivec2{100, 25},
        .bgColor = vec4{1.f, 0.f, 0.f, 1.f}
    });
}

void UIElementsOverview::addLabel(UINode& root, const int heightStep) {
    root.push<Label>(LabelPars{
        .pos = ivec2{0, heightStep},
        .size = ivec2{100, 30},
        .color = vec4(1.f, 1.f,1.f, 1.f),
        .text = "some Text",
    });
}

void UIElementsOverview::addButton(UINode& root, const int heightStep) const {
    auto& butt = root.push<Button>(LabelPars{
        .pos = ivec2{0,  heightStep * 2},
        .size = ivec2{100, 30},
        .color = vec4{0.9f, 0.9f, 0.9f, 1.f},
        .bgColor = vec4{ 0.3f, 0.3f, 0.3f, 1.f },
        .borderWidth = 2,
        .borderRadius = 7,
        .borderColor = vec4{ 0.8f, 0.8f, 0.8f, 1.f },
        .text = "click me",
    });
    butt.addMouseClickCb([this](hidData& data) { LOG << "Button clicked!"; });
}

void UIElementsOverview::addSlider(UINode& root, const int heightStep) {
    auto& slider = root.push<Slider>({
        .pos = ivec2{0, heightStep * 3},
        .size = ivec2{ 300, 30},
    });
    slider.setValue(0.f);
    slider.setValueChangeCb([&](float v) {
        LOG << "slider value: " << v;
    });
}

void UIElementsOverview::addPropSlider(UINode& root, const int heightStep) {
    auto& propSlider = root.push<PropSlider>({
        .pos = ivec2{0, heightStep * 4},
        .size = ivec2{ 300, 30 }
    });
    propSlider.setLabel("Val");
    propSlider.setProp(floatProp);
    floatProp.onPostChange([this] {
        LOG << "PropSlider changed: " << floatProp();
    }, this);
}

void UIElementsOverview::addComboBox(UINode& root, const int heightStep) const {
    auto& combo = root.push<ComboBox>({
        .pos = ivec2{0, heightStep * 6},
        .size = ivec2{ 300, 50 }
    });
    combo.setMenuName("Combo");
    combo.addEntry("entry 1", [this] { LOG << "entry 1 clicked"; });
    combo.addEntry("entry 2", [this] { LOG << "entry 2 clicked"; });
    combo.addEntry("entry 3", [this] { LOG << "entry 3 clicked"; });
}

void UIElementsOverview::addImage(UINode& root, const int heightStep) {
    auto& image = root.push<Image>({
        .pos = ivec2{0, heightStep * 16},
        .size = ivec2{ 300, 200 }
    });
    image.setImg("test/test-tex.png", 4);
}

void UIElementsOverview::addImageButton(UINode& root, const int heightStep) {
    auto& imageButt = root.push<ImageButton>({
        .pos = ivec2{0, heightStep * 7.5f},
        .size = ivec2{35, 35},
        .fgColor = vec4{0.f, 0.f, 0.4f, 1.f},
    });
    imageButt.setIsToggle(true);
    imageButt.setImg("Icons/invert_icon.png");
    imageButt.setOnStateImg("Icons/invert_icon_selected.png", 1);
}

void UIElementsOverview::addSpinner(UINode& root, const int heightStep) {
    auto& spinner = root.push<Spinner>({
        .pos = ivec2{0, heightStep * 9},
        .size = ivec2{80, 80}
    });
    spinner.setImage("Spinner/spinner-1.png", 2);
}

void UIElementsOverview::addEditor(UINode& root, const int heightStep) {
    auto &edit = root.push<UIEdit>(UINodePars{
        .pos = ivec2{0, heightStep * 12},
        .size = ivec2{ 300, 30 },
        .borderWidth = 1,
        .borderColor = vec4{1.f, 1.f, 1.f, 1.f},
    });
    edit.setText("Edit me!");
}

void UIElementsOverview::addInfoDialogButton(UINode& root, const int heightStep) {
    auto& butt2 = root.push<Button>(LabelPars{
        .pos = ivec2{0, heightStep * 14},
        .size = ivec2{ 100, 30},
        .color = vec4{0.9f, 0.9f, 0.9f, 1.f},
        .bgColor = vec4{ 0.3f, 0.3f, 0.3f, 1.f },
        .borderWidth = 2,
        .borderRadius = 7,
        .borderColor = vec4{ 0.8f, 0.8f, 0.8f, 1.f },
        .text = "Open Dialog",
    });
    butt2.setClickedCb([this] {
        openInfoDiag(infoDiagType::error, "Hello", [this] {
            LOG << "confirmed"; return true;
        });
    });
}

void UIElementsOverview::addTable(UINode& root) {
    auto& table = root.push<UITable>(UITableParameters{
        .pos = ivec2{350, 0},
        .size = ivec2{800, 300},
        .topology = ivec2{4, 3}
    });

    table.setSpacing(8, 8);
    table.setMargins(5, 5);
    constexpr auto textColor = vec4(1.f, 1.f, 1.f, 1.f);

    for (int i = 0; i < table.getRowCount(); i++)
        for (int j = 0; j < table.getColumnCount(); j++) {
            const auto l = table.setCell<Label>(i, j);
            l->setFont("regular", 22, align::left, valign::top, textColor);
            l->setBackgroundColor(.3f, .3f, .4f, 1.0f);
            l->setText("(" + std::to_string(i) + "," + std::to_string(j) + ")");
            l->setPadding(10, 10, 10, 10);
        }
}

void UIElementsOverview::addTabView(UINode& root) {
    auto fgCol = vec4(.3f, .3f, .3f, 1.f);
    auto bgCol = vec4(.1f, .1f, .1f, 1.f);
    auto& tabView = root.push<TabView>({
        .pos = ivec2{350, 400},
        .size = ivec2{800, 300},
        .fgColor = fgCol,
        .bgColor = bgCol,
        .padding = vec4{10.f, 10.f, 10.f, 10.f}
    });

    const auto l1 = tabView.addTab<Label>("Tab 1");
    l1->setText("TAB 1");

    const auto l2 = tabView.addTab<Label>("Tab 2");
    l2->setText("TAB 2");

    const auto l3 = tabView.addTab<Label>("Tab 3");
    l3->setText("TAB 3");

    tabView.setActivateTab(0);
}