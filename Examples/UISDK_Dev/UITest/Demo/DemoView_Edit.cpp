#include "DemoView.h"
#include "../../../../Libraries/GLSceneGraph/src/UI/Image.h"
#include "../../../../Libraries/Utilities/src/Log.h"
#include "../../../../Libraries/GLBase/src/Res/ResColor.h"
#include "../../../../Libraries/GLBase/src/Res/ResInstance.h"
#include "../../../../Libraries/GLBase/src/Res/ResNode.h"
#include "../../../../Libraries/GLBase/src/Res/ResSrcFile.h"
#include "../../../../Libraries/GLSceneGraph/src/UI/UIEdit.h"
#include "../../../../Libraries/GLSceneGraph/src/UI/Label.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView_Edit::DemoView_Edit() : DemoView("Edit Demo",glm::vec4(.15f,.15f,.15f,1.f))
{
    setName(getTypeName<DemoView_Edit>());
    addStyleClass("demos.edit");
    setScissorChildren(true);
}

void DemoView_Edit::init()
{
    UIEdit* ed;

    /*
    ed=addChild<UIEdit>();
    ed->setText("Test Single-Line Edit");
    ed->setPos(10,100);
    ed->setSize(200,28);
    ed->setFontType("regular");
    ed->setFontSize(19);
    ed->setSingleLine();
    ed->setPadding(2.f);
    ed->setBorderWidth(2);
    ed->setBorderRadius(4);
    ed->setBorderColor(.3f, .3f, .3f, 1.f);
    ed->setBackgroundColor(.15f, .15f, .15f, 1.f);

    addChild<Label>(getStyleClass()+".ed-sl-label");
*/

    ed=addChild<UIEdit>(getStyleClass()+".ed-sl");
    ed->setOpt(UIEdit::single_line | UIEdit::num_int);
    ed->setValue(200);
    ed->addEnterCb([this](std::string str){ LOG << "entercb " << str; }, this);
    ed->setOnFocusedCb([this]{ LOG << "focused"; });
    ed->setOnLostFocusCb([this]{ LOG << "lost focus";  });
    ed->setCaretColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
    ed->setPos(10,150);
    ed->setUseWheel(true);
    //ed->setEditPixSpace(1e10,1e10);

    addChild<Label>(getStyleClass()+".ed-sl-label-float");

    ed=addChild<UIEdit>(getStyleClass()+".ed-sl-float");
    ed->setOpt(UIEdit::single_line | UIEdit::num_fp);
    ed->setValue(1.f);
    ed->setUseWheel(true);

    ed=addChild<UIEdit>(getStyleClass()+".ed-ml");
    ed->setText("Test Multi-Line Edit:\rLorem ipsum dolor sit amet, consectetur adipiscing elit. Sed neque ligula, tristique euismod scelerisque ut, finibus id libero. Praesent sagittis consectetur consequat. Integer et elit sed lorem finibus placerat in sit amet libero. Praesent sed nibh nec magna auctor aliquam quis ultrices sapien. Integer ullamcorper volutpat felis, condimentum facilisis mauris dignissim sed. Donec tempus non lorem vel facilisis. Sed pulvinar lorem in nulla facilisis sodales. Praesent commodo consectetur cursus. Sed tristique, nunc a suscipit hendrerit, nulla metus dapibus orci, non ultrices leo nulla eget ligula. Duis porta neque rutrum metus varius varius. Maecenas tincidunt varius leo. Duis risus leo, rutrum a ante sit amet, egestas cursus massa. Mauris aliquet lobortis ultrices.");
    //ed->setText("012345");
    //ed->setFont(font);
    //ed->setOpt(UIEdit::accept_tabs);
    ed->setPos(10,300);
    ed->setUseWheel(true);
    ed->setName("testItem");

    addChild<Label>(getStyleClass()+".label");
}

bool DemoView_Edit::draw(uint32_t* objId)
{
    DemoView::draw(objId);

    ResNode *node = getStyleResNode();
    ResFont* rfont=node->findNode<ResFont>("font-test.font");

    if (rfont==nullptr) return false;

    Font* font;

    if ((font=getSharedRes()->res->getGLFont(rfont->m_FontPath,rfont->m_Size, getWindow()->getPixelRatio()))==nullptr) return false;

    float* mvp=getMVPMatPtr();
    std::string teststring("The quick brown fox jumps over the lazy dog");
    float xp=700.f;
    float* tcolor=&glm::vec4(1.f)[0];

    if (!m_glyphShader) m_glyphShader = getSharedRes()->shCol->getStdGlyphShdr();
    //font->write(mvp,m_glyphShader,tcolor,xp,100,teststring);

    //util_Print(xp,150.f,"regular",(float)(rfont->m_Size),tcolor,teststring.c_str());

    //m_typo->at("regular")->m_print(glm::vec2(xp,200.f),teststring.c_str(),rfont->m_Size,tcolor,mvp,0,100000);

    if (0) {

        int i;
        float yp=100.f;

        for (i = 10; i < 30; i++)
        {
            if ((font = getSharedRes()->res->getGLFont(rfont->m_FontPath, i, getWindow()->getPixelRatio()))!=nullptr)
            {
                font->write(getMvp(), m_glyphShader, *getSharedRes()->nullVao, tcolor, xp+350.f, yp, std::to_string(i)+" pix  "+teststring);
                yp+=font->getPixAscent()+10;
            }
        }
    }

    if (0) {

        int i,j,ts=20;
        float yp=300.f;
        float xp=700.0f;

        for (i = 0; i < 20; i++) {
            for (j = 0; j < 10; j++) {
                if ((font = getSharedRes()->res->getGLFont(rfont->m_FontPath, ts, getWindow()->getPixelRatio())) != nullptr) {

                    //font->write(mvp, tcolor, xp + 350.f+float(j*50), yp, std::to_string(i) + "x" + std::to_string(j));
                    font->write(getMvp(), m_glyphShader, *getSharedRes()->nullVao, tcolor, xp+float(j*90+i),yp+i,"The Surface");

                }
            }

            yp += font->getPixAscent() + 10;
        }
    }

    return false;
}