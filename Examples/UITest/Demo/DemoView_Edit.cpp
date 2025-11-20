#include "DemoView.h"
#include <UIElements/Image.h>
#include <Log.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetManager.h>
#include <Asset/ResNode.h>
#include <Asset/ResSrcFile.h>
#include <UIElements/UIEdit.h>
#include <UIElements/Label.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Edit::DemoView_Edit() : DemoView("Edit Demo",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_Edit>());
    UINodeStyle::addStyleClass("demos.edit");
    setScissorChildren(true);
}

void DemoView_Edit::init() {
    auto& ed = push<UIEdit>(UINodePars{
        .pos = ivec2{10,50},
        .size = ivec2{200,28},
        .bgColor = vec4{.15f, .15f, .15f, 1.f},
        .borderWidth = 2,
        .borderRadius = 4,
        .borderColor = vec4{.3f, .3f, .3f, 1.f},
        .padding = vec4{2.f, 2.f, 2.f, 2.f},
    });
    ed.setText("Test Single-Line Edit");
    ed.setFontType("regular");
    ed.setFontSize(19);
    ed.setSingleLine();

    push<Label>({ .style = getStyleClass()+".ed-sl-label" });

    auto& ed2 = push<UIEdit>();
    ed2.addStyleClass(getStyleClass()+".ed-sl");
    ed2.setOpt(UIEdit::single_line | UIEdit::num_int);
    ed2.setValue(100);
    ed2.addEnterCb([](const std::string& str){ LOG << "entercb " << str; }, this);
    ed2.setOnFocusedCb([]{ LOG << "focused"; });
    ed2.setOnLostFocusCb([]{ LOG << "lost focus";  });
    ed2.setUseWheel(true);

    push<Label>({ .style = getStyleClass()+".ed-sl-label-float" });

    auto& ed3 = push<UIEdit>(UINodePars{ .style = getStyleClass()+".ed-sl-float" });
    ed3.setOpt(UIEdit::single_line | UIEdit::num_fp);
    ed3.setValue(1.f);
    ed3.setUseWheel(true);

    auto& ed4 = push<UIEdit>(UINodePars{
        .pos = ivec2{10,200},
        .name = "testItem",
        .style = getStyleClass()+".ed-ml"
    });
    ed4.setText("Test Multi-Line Edit:\rLorem ipsum dolor sit amet, consectetur adipiscing elit. Sed neque ligula, tristique euismod scelerisque ut, finibus id libero. Praesent sagittis consectetur consequat. Integer et elit sed lorem finibus placerat in sit amet libero. Praesent sed nibh nec magna auctor aliquam quis ultrices sapien. Integer ullamcorper volutpat felis, condimentum facilisis mauris dignissim sed. Donec tempus non lorem vel facilisis. Sed pulvinar lorem in nulla facilisis sodales. Praesent commodo consectetur cursus. Sed tristique, nunc a suscipit hendrerit, nulla metus dapibus orci, non ultrices leo nulla eget ligula. Duis porta neque rutrum metus varius varius. Maecenas tincidunt varius leo. Duis risus leo, rutrum a ante sit amet, egestas cursus massa. Mauris aliquet lobortis ultrices.");
    ed4.setUseWheel(true);

    push<Label>({ .style = getStyleClass()+".label" });
}
