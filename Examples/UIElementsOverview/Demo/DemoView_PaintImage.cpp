#include "DemoView.h"
#include <UIElements/PaintImageIdMap.h>
#include <UIElements/Menu/ComboBox.h>
#include <UIElements/Slider.h>
#include <UIElements/Button/Button.h>

#include "UIElements/DataBinding/PropSlider.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_PaintImage::DemoView_PaintImage() : DemoView("PaintImageIdMap Demo", vec4(.12f, .12f, .12f, 1.f)) {
    setName(getTypeName<DemoView_PaintImage>());
}

void DemoView_PaintImage::init() {
    setPadding(10.f);

    // Paint Image Node
    auto paintImageSize = ivec2{710, 400};

    int curY = 110;

    m_paintImage = &push<PaintImage>(UINodePars{
        .pos = ivec2{20, curY},
        .size = paintImageSize,
        .bgColor = vec4{0.f, 0.f, 0.f, 1.f},
        .borderWidth = 2,
        .borderRadius = 4,
        .borderColor = vec4{0.3f, 0.3f, 0.3f, 1.f},
    });

    m_paintImage->setImg("test/test-tex.png", 1);
    m_paintImage->setBrush(PaintImage::Brush{
        .size = 10.f,
        .hardness = 0.5f,
        .color = vec4(1.f, 1.f, 1.f, 1.f),
        .opacity = 1.0f
    });

    constexpr int panelX = 760;

    m_sizeProp = 10.f;
    auto& sizeSlider = push<PropSlider>({
        .pos = ivec2{panelX, curY},
        .size = ivec2{320, 26},
        .bgColor = vec4{.2f, .2f, .2f, 1.f},
        .borderRadius = 3
    });
    sizeSlider.setLabel("Size");
    sizeSlider.setProp(m_sizeProp);

    // listen for changes. when listening like this, the listener must be unregistered before his destructor is called
    m_sizeProp.onPostChange([this] {
        if (m_paintImage) {
            m_paintImage->setBrushSize(m_sizeProp());
        }
    }, this);


    curY += 40;
    auto& hardSlider = push<PropSlider>({
        .pos = ivec2{panelX, curY},
        .size = ivec2{320, 26},
        .bgColor = vec4{.2f, .2f, .2f, 1.f},
        .borderRadius = 3
    });
    hardSlider.setLabel("Hard");
    hardSlider.setProp(m_hardProp);

    // listen for changes. when listening like this, the listener must be unregistered before his destructor is called
    m_hardProp.onPostChange([this] {
        if (m_paintImage) {
            m_paintImage->setBrushHardness(m_hardProp());
        }
    }, this);


    curY += 40;
    // Clear / Reset Canvas Button
    auto& clearBtn = push<Button>(UINodePars{
        .pos = ivec2{panelX, curY},
        .size = ivec2{150, 35},
        .fgColor = vec4{1.f, 1.f, 1.f, 1.f},
        .bgColor = vec4{.3f, .3f, .3f, 1.f},
        .borderWidth = 1,
        .borderRadius = 4,
        .borderColor = vec4{.5f, .5f, .5f, 1.f},
    });
    clearBtn.setText("Clear Canvas");
    clearBtn.setFontSize(15);
    clearBtn.setFontType("regular");
    clearBtn.setClickedCb([this] {
        if (m_paintImage) {
            m_paintImage->clear();
        }
    });

    curY += 40;
    // Clear / Reset Canvas Button
    auto& colSel = push<Button>(UINodePars{
        .pos = ivec2{panelX, curY},
        .size = ivec2{150, 35},
        .fgColor = vec4{1.f, 1.f, 1.f, 1.f},
        .bgColor = vec4{.3f, .3f, .3f, 1.f},
        .borderWidth = 1,
        .borderRadius = 4,
        .borderColor = vec4{.5f, .5f, .5f, 1.f},
    });
    colSel.setIsToggle(true);
    colSel.setText("Select Color");
    colSel.setBackgroundColor(0.2f, 0.2f, 0.2f, 1.f, state::selected);
    colSel.setFontSize(15);
    colSel.setFontType("regular");
    colSel.setToggleCb([this](const bool val) {
        if (m_paintImage) {
            m_paintImage->setMode(val ? PaintImage::mode::mask : PaintImage::mode::add);
            m_paintImage->setCursor();
            getSharedRes()->reqRedraw();
        }
    });

    curY += 40;
    auto& toleranceSlider = push<PropSlider>({
        .pos = ivec2{panelX, curY},
        .size = ivec2{320, 26},
        .bgColor = vec4{.2f, .2f, .2f, 1.f},
        .borderRadius = 3
    });
    toleranceSlider.setLabel("Toler");
    toleranceSlider.setProp(m_tolProp);

    // listen for changes. when listening like this, the listener must be unregistered before his destructor is called
    m_tolProp.onPostChange([this] {
        if (m_paintImage) {
            m_paintImage->setTolerance(m_tolProp());
        }
    }, this);
}
