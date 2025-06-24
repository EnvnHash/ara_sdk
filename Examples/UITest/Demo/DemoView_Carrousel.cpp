#include "DemoView.h"
#include "Transitions/Carrousel.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Carrousel::DemoView_Carrousel() : DemoView("Carrousel", glm::vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_Carrousel>());
}

void DemoView_Carrousel::addCarrousel(CarrouselMode cm, int yOffs) {
    vec4 white = vec4{1.f, 1.f, 1.f, 1.f};
    vec4 black = vec4{0.f, 0.f, 0.f, 1.f};

    addChild<Label>(LabelPars{
            .pos = ivec2{ 0, yOffs },
            .size = ivec2{ 200, 30 },
            .align = align::left,
            .valign = valign::top,
            .text_color = white,
            .text = cm == CarrouselMode::fitAllOnScreen ? "fitAllOnScreen" : "fitOneOnScreen",
            .text_align_x = align::left,
            .font_type = "regular",
            .font_height = 18
    });

    auto caru = addChild<Carrousel>(UINodePars{
            .pos = ivec2{ 0, yOffs + 30 },
            .size = vec2{ 1.f, 0.4f },
            .bgColor = vec4{ 0.3f, 0.3f, 0.3f, 0.3f},
            .valign = valign::top
    });
    caru->setMode(cm);
    caru->getSelector()->setBorderColor(white);

    std::array bgColor { vec4{11.f / 255.f, 29.f / 255.f, 81.f / 255.f, 1.f},
                         vec4{114.f / 255.f, 92.f / 255.f, 173.f / 255.f, 1.f},
                         vec4{1.f, 227.f / 255.f, 169.f / 255.f, 1.f},
                         vec4{140.f / 255.f, 205.f / 255.f, 253.f / 255.f, 1.f},
                         vec4{121.f / 255.f, 101.f / 255.f, 193.f / 255.f, 1.f},
    };

    constexpr int numSlides = 5;
    for (int i=0; i<numSlides; i++) {
        auto slide = caru->add();
        auto lbl = slide->addChild<Label>();
        lbl->setFont("regular", 90, align::center, valign::center, black);
        lbl->setText(std::to_string(i));
        lbl->setBackgroundColor(bgColor[i]);
    }

    caru->show(2, false);
}

void DemoView_Carrousel::init() {
    addCarrousel(CarrouselMode::fitAllOnScreen, 40);
    addCarrousel(CarrouselMode::fitOneSlideOnScreen, getContentSize().y / 2 + 30);
}
