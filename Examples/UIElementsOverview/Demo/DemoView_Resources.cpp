#include "DemoView.h"
#include <UIElements/Image.h>
#include <Log.h>
#include <Asset/AssetManager.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Resources::DemoView_Resources() : DemoView("Resources Demo",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_Resources>());
    UINodeStyle::addStyleClass("chtest");
}

void DemoView_Resources::init() {
    setPadding(10.f);
    fontList.setGlbase(m_glbase);

    push<Label>({ .style = "text_test" });

    auto& dp = push<Div>({ .style = "styles.rdiv" });
    dp.push<Div>({ .style = "styles.irdiv" });

    push<Image>({ .style = "chtest.sample-image" }).setLod(3);

    push<Image>({ .style = "chtest.sample-image-ref" }).setLod(3);

    push<Image>({
        .pos = ivec2{150,100},
        .size = ivec2{300,200},
        .style = "chtest.icon"
    });

    auto& image = push<Image>({
        .pos = ivec2{150,320},
        .size = ivec2{300,200},
        .style = "chtest.icon"
    });
    image.setSelected(isSelected());

    push<Image>({
        .pos = ivec2{150,540},
        .size = ivec2{300,200},
        .style = "one.but1"
    });
    image.selectSection(1);

    auto& imgBut = push<ImageButton>(UINodePars{
        .style = "chtest.sample-imageButton"
    });
    imgBut.setIsToggle(true);
    imgBut.setToggleCb([](const bool val){
        LOG << "val " << val;
    });
}
