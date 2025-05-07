#include "DemoView.h"
#include <UIElements/Image.h>
#include <Log.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetManager.h>
#include <Asset/ResNode.h>
#include <Asset/ResSrcFile.h>
#include <Utils/PingPongFbo.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Resources::DemoView_Resources() : DemoView("Resources Demo",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_Resources>());
    addStyleClass("chtest");
}

void DemoView_Resources::init() {
    setPadding(10);
    fontList.setGlbase(m_glbase);

    addChild(make_unique<Label>("text_test"));

    auto dp= addChild<Div>("styles.rdiv");
    dp->addChild<Div>("styles.irdiv");

    auto image = addChild<Image>("chtest.sample-image");
    image->setLod(3);

    image = addChild<Image>("chtest.icon");
    image->setPos(150,100);
    image->setSize(300,200);

    image = addChild<Image>("chtest.icon");
    image->setPos(150,350);
    image->setSize(300,200);
    image->setSelected(isSelected());

    image = addChild<Image>("one.but1");
    image->setPos(150,600);
    image->setSize(300,200);
    image->selectSection(1);

    auto imgBut =addChild<ImageButton>("chtest.sample-imageButton");
    imgBut->setIsToggle(true);
    imgBut->setToggleCb([this](bool val){
        LOG << "val " << val;
    });
}

bool DemoView_Resources::draw(uint32_t& objId) {
    DemoView::draw(objId);

    int i=0;
    auto node = getStyleResNode();
    drawColors(node, i);

    node = m_sharedRes->res->findNode("prestyle");
    drawColors(node, i);

    node = m_sharedRes->res->findNode("poststyle");
    drawColors(node, i);

    {
        Font* f = fontList.addFromFilePath("resdata/Fonts/verdana.ttf", 22, getWindow()->getPixelRatio());
        vec2 pos{ 1000,100 };
        vec2 size{ 400.f,300.f };
        vec2 sep{0.f,0.f};
        vec2 scale{1.f,1.f};

        util_FillRect(pos, size, &glm::vec4(1.f,1.f,1.f,0.1f)[0]);

        if (f) {
            std::string teststring="In nuclear fission the nucleus of an atom breaks up into two lighter nuclei. The process may take place spontaneously in some cases or may be induced by the excitation of the nucleus with a variety of particles (e.g., neutrons, protons, deuterons, or alpha particles) or with electromagnetic radiation in the form of gamma rays.";
            FontGlyphVector dgv;
            vec4 bb;
            int valign=0;
            vec2 tp;

            dgv.Process(f, size, sep, align::left, teststring, true);
            dgv.calculateBoundingBox(bb);

            switch (valign) {
                case 1 : tp[1] = size[1]/2 - (bb[3]-bb[1])/2; break;
                case 2 : tp[1] = size[1] - (bb[3]-bb[1]); break;
                default : tp[1] = 0;
            }

            if (!m_glyphShader) {
                m_glyphShader = getSharedRes()->shCol->getStdGlyphShdr();
            }

            f->drawDGlyphs(dgv,
                           getMvp(),
                           m_glyphShader, *getSharedRes()->nullVao,
                           &glm::vec4(1.f)[0],
                           {pos[0]+tp[0], pos[1]+tp[1]-bb[1]},
                           pos,
                           size);
        }
    }

    return false;
}

void DemoView_Resources::drawColors(ResNode* node, int& it) {
    if (node != nullptr) {
        for (ResNode::Ptr& p : node->m_node) {
            auto& deref = *p; // silence warning with clang
            if (typeid(deref) == typeid(AssetColor)) {
                auto rc=dynamic_cast<AssetColor*>(p.get());
                util_FillRect({10,100+it*40}, {28,28}, rc->getColor4fv());
                ++it;
            }
        }
    }
}
