#include "DemoView.h"
#include <Image.h>
#include <Log.h>
#include <Res/ResColor.h>
#include <Res/ResInstance.h>
#include <Res/ResNode.h>
#include <Res/ResSrcFile.h>

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

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

bool DemoView_Resources::draw(uint32_t* objId) {
    float pos[2]={200.f,100.f};
    float ss[2]={300.f,100.f};

    DemoView::draw(objId);
    glm::mat4 imat = *getMvp();
    int i=0;
    auto node = getStyleResNode();
    if (node) {
        // Colors
        for (ResNode::Ptr& p : node->m_Node) {
            auto& deref = *p; // silence warning with clang
            if (typeid(deref) == typeid(ResColor)) {
                ResColor* rc=dynamic_cast<ResColor*>(p.get());

                util_FillRect(10, 100+i*40, 28, 28, rc->getColor4fv());
//				util_Print(60,float(100+i*40+22),"regular",22,nullptr,rc->m_Name.c_str());

                i++;
            }
        }
    }

    node=m_sharedRes->res->findNode("prestyle");

    if (node != nullptr) {
        // Colors
        for (ResNode::Ptr& p : node->m_Node) {
            auto& deref = *p; // silence warning with clang
            if (typeid(deref) == typeid(ResColor)) {
                ResColor* rc=dynamic_cast<ResColor*>(p.get());
                util_FillRect(10,100+i*40,28,28,rc->getColor4fv());
                //util_Print(60,float(100+i*40+22),"regular",22,nullptr,rc->m_Name.c_str());

                i++;
            }
        }
    }

    node=m_sharedRes->res->findNode("poststyle");

    if (node != nullptr) {
        // Colors
        for (ResNode::Ptr& p : node->m_Node) {
            auto& deref = *p; // silence warning with clang
            if (typeid(deref) == typeid(ResColor)) {
                ResColor* rc=dynamic_cast<ResColor*>(p.get());
                util_FillRect(10,100+i*40,28,28,rc->getColor4fv());
                //util_Print(60,float(100+i*40+22),"regular",22,nullptr,rc->m_Name.c_str());
                i++;
            }
        }
    }

    if (1) {
        string style = "\n"
                       "sven:rgb(255,0,0)\n"
                       "\n";

        SrcFile s(m_glbase);

        vector<uint8_t> vp;

        vp.resize(style.size() + 1);
        memcpy(&vp[0], style.c_str(), style.size() + 1);

        ResNode::Ptr n=std::make_unique<ResNode>("ms", m_glbase);

        if (s.Process(n.get(), vp)) {

            n->Preprocess();
            n->Process();

            if (n->errList.empty()) {
                if (n->Load()) {
                    if (n->errList.empty()) {
                        for (ResNode::Ptr& p : n->m_Node) {
                            auto& deref = *p; // silence warning with clang
                            if (typeid(deref) == typeid(ResColor)) {

                                auto* rc=dynamic_cast<ResColor*>(p.get());
                                util_FillRect(10,100+i*40,28,28,rc->getColor4fv());
                                //util_Print(60,float(100+i*40+22),"regular",22,nullptr,rc->m_Name.c_str());

                                i++;
                            }
                        }
                    }
                }
            }
        }
    }

    {
        Font* f = fontList.addFromFilePath("resdata/Fonts/verdana.ttf", 22, getWindow()->getPixelRatio());
        vec2 pos{ 1000,100 };
        vec2 size{ 400.f,300.f };
        vec2 sep{0.f,0.f};
        vec2 scale{1.f,1.f};

        util_FillRect((int) pos[0],(int) pos[1],(int) size[0],(int) size[1],&glm::vec4(1.f,1.f,1.f,0.1f)[0]);

        if (f) {

            std::string teststring="In nuclear fission the nucleus of an atom breaks up into two lighter nuclei. The process may take place spontaneously in some cases or may be induced by the excitation of the nucleus with a variety of particles (e.g., neutrons, protons, deuterons, or alpha particles) or with electromagnetic radiation in the form of gamma rays.";
            FontGlyphVector dgv;
            vec4 bb;
            int valign=0;
            vec2 tp;

            dgv.Process(f, size, sep, align::left, teststring, true);
            dgv.calculateBoundingBox(bb);

            switch (valign)
            {
                case 1 : tp[1] = size[1]/2 - (bb[3]-bb[1])/2; break;
                case 2 : tp[1] = size[1] - (bb[3]-bb[1]); break;
                default : tp[1] = 0;
            }

            if (!m_glyphShader) m_glyphShader = getSharedRes()->shCol->getStdGlyphShdr();

            f->drawDGlyphs(dgv, getMvp(), m_glyphShader, *getSharedRes()->nullVao, &glm::vec4(1.f)[0],
                           pos[0]+tp[0], pos[1]+tp[1]-bb[1], pos[0], pos[1], size[0], size[1]);
        }
    }

    return false;
}
