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
            if (typeid(*p) == typeid(ResColor)) {
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
            //LOG << p->m_Name << "  type=" << typeid(*p).name();
            if (typeid(*p) == typeid(ResColor)) {
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
            //LOG << p->m_Name << "  type=" << typeid(*p).name();
            if (typeid(*p) == typeid(ResColor)) {
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
                            LOG << p->m_Name << "  type=" << typeid(*p).name();

                            if (typeid(*p) == typeid(ResColor)) {

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

    if (0) {

        static Texture* tm=nullptr;

        if (!tm)
        {
            std::vector<uint8_t> vp;

            getSharedRes()->res->loadResource(nullptr, vp, "FullHD_Pattern.png");

            tm=new Texture(m_glbase);
            tm->loadFromMemPtr(&vp[0],vp.size(),GL_TEXTURE_2D, 1, 0);
        }

        Shaders* sh= m_sharedRes->shCol->getUIGridTexSimple();

        if (!sh) return false;

        sh->begin();

        sh->setUniform1i("stex", 0);
        sh->setUniformMatrix4fv("mvp", getMVPMatPtr());

        sh->setUniform2f("pos", getContentOffset().x, getContentOffset().y);
        sh->setUniform2f("size", getContentSize().x, getContentSize().y);

        sh->setUniform4fv("color", &glm::vec4(1.f)[0]);
        sh->setUniform1i("flags", 0);
        sh->setUniform2i("align", 0,0);
        sh->setUniform1f("scale", 1);

        sh->setUniform2i("section_pos", 0, 0);
        sh->setUniform2i("section_size", tm->getWidth(), tm->getHeight());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tm->getId());

        glBindVertexArray(*m_sharedRes->nullVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);


        /*

        (*m_texCol)[filename] = make_unique<Texture>();
        (*m_texCol)[filename]->loadTexture2D(filename, m_mimMapLevel);
        (*m_texCol)[filename]->setFiltering(GL_LINEAR, GL_LINEAR);
        (*m_texCol)[filename]->setWraping(GL_CLAMP_TO_BORDER);
        */
    }

    return false;
}
