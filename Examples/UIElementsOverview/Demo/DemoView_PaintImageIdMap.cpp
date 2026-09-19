#include "DemoView.h"
#include <UIElements/PaintImageIdMap.h>
#include <UIElements/Menu/ComboBox.h>
#include <UIElements/Slider.h>
#include <UIElements/Button/Button.h>

#include "UIElements/DataBinding/PropSlider.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_PaintImageIdMap::DemoView_PaintImageIdMap() : DemoView("PaintImageIdMap Demo", vec4(.12f, .12f, .12f, 1.f)) {
    setName(getTypeName<DemoView_PaintImageIdMap>());
}

void DemoView_PaintImageIdMap::init() {
    setPadding(10.f);

    // Header / Description
    push<Label>(LabelPars{
        .pos = ivec2{20, 45},
        .size = ivec2{750, 40},
        .color = vec4{0.9f, 0.9f, 0.9f, 1.f},
        .text = "PaintImageIdMap allows painting 32 distinct layer ID bitmasks into a single texture and visualizing any selected ID.",
        .textAlignX = align::left,
        .textAlignY = valign::top,
        .fontType = "regular",
        .fontHeight = 16,
    });

    // Instructions
    push<Label>(LabelPars{
        .pos = ivec2{20, 75},
        .size = ivec2{750, 30},
        .color = vec4{0.7f, 0.7f, 0.7f, 1.f},
        .text = "Select a Brush ID to paint into that ID layer. Select a Visualization ID to display only that layer.",
        .textAlignX = align::left,
        .textAlignY = valign::top,
        .fontType = "regular",
        .fontHeight = 14,
    });

    // Paint Image Node
    ivec2 paintImageSize = ivec2{400, 400};

    m_paintImage = &push<PaintImageIdMap>(UINodePars{
        .pos = ivec2{20, 110},
        .size = paintImageSize,
        .bgColor = vec4{0.f, 0.f, 0.f, 1.f},
        .borderWidth = 2,
        .borderRadius = 4,
        .borderColor = vec4{0.3f, 0.3f, 0.3f, 1.f},
    });

    m_paintImage->setImg("test/black.png", 1);
    m_paintImage->setBrush(PaintImage::Brush{
        .size = 10.f,
        .hardness = 0.5f,
        .color = vec4(1.f, 1.f, 1.f, 1.f),
        .opacity = 1.0f
    });
    m_paintImage->setVisualizationID(0);

    const int panelX = 450;
    int curY = 110;

    // Brush ID dropdown
    auto comboPars = UINodePars{
        .pos = ivec2{panelX, curY},
        .size = ivec2{220, 35},
        .fgColor = vec4{1.f, 1.f, 1.f, 1.f},
        .bgColor = vec4{.15f, .15f, .15f, 1.f},
        .borderWidth = 2,
        .borderRadius = 5,
        .borderColor = m_sharedRes->colors->at(uiColors::blue),
        .padding = vec4{5, 5, 5, 5},
    };
    m_brushCombo = &push<ComboBox>(comboPars);
    m_brushCombo->setMenuName("Brush ID 0");
    m_brushCombo->setFontType("regular");

    for (int i = 0; i <= 31; ++i) {
        m_brushCombo->addEntry("ID " + std::to_string(i), [this, i] {
            if (m_paintImage) {
                m_paintImage->setDrawId(i);
            }
            if (m_brushCombo) {
                m_brushCombo->setMenuName("Brush ID " + std::to_string(i));
            }
        });
    }
    curY += 45;

    // Visualization ID dropdown
    comboPars.pos = ivec2{panelX, curY};
    m_visCombo = &push<ComboBox>(comboPars);
    m_visCombo->setMenuName("View ID 0");
    m_visCombo->setFontType("regular");

    for (int i = 0; i <= 31; ++i) {
        m_visCombo->addEntry("ID " + std::to_string(i), [this, i] {
            if (m_paintImage) {
                m_paintImage->setVisualizationID(i);
            }
            if (m_visCombo) {
                m_visCombo->setMenuName("View ID " + std::to_string(i));
            }
            getSharedRes()->requestRedraw = true;
        });
    }
    curY += 45;

    // Brush Size label and slider
    m_sizeLabel = &push<Label>(LabelPars{
        .pos = ivec2{panelX, curY},
        .size = ivec2{220, 25},
        .color = vec4{1.f, 1.f, 1.f, 1.f},
        .text = "Brush Size: 30 px",
        .textAlignX = align::left,
        .textAlignY = valign::center,
        .fontType = "regular",
        .fontHeight = 16,
    });
    curY += 30;

    m_prop = 10.f;
    auto& slider = push<PropSlider>({
        .pos = ivec2{panelX, curY},
        .size = ivec2{220, 25},
        .bgColor = vec4{.2f, .2f, .2f, 1.f},
        .style = "demos.dataBinding.propSlider",
        .borderRadius = 3
    });
    slider.setLabel("Slider");
    slider.setProp(m_prop);

    // listen for changes. when listening like this, the listener must be unregistered before his destructor is called
    m_prop.onPostChange([this] {
        if (m_paintImage) {
            m_paintImage->setBrushSize(m_prop());
        }
        if (m_sizeLabel) {
            m_sizeLabel->setText("Brush Size: " + std::to_string(static_cast<int>(m_prop())) + " px");
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
            m_paintImage->setImg("test/black.png", 1);
        }
    });
}
