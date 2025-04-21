//
// Created by sven on 11/15/20.
//

#include "../test_common.h"

#include "UIApplication.h"

using namespace std;
using namespace glm;

namespace ara::GLSceneGraphUnitTest::NestedAlignment{

    TEST(UITest, NestedAlignment)
    {
        UIApplication app;
        app.setEnableMenuBar(false);
        app.setEnableWindowResizeHandles(false);
        app.setMultisample(false);

        FIBITMAP* pBitmap;

        // implicitly creates a window and runs the lambda before calling the draw function for the first time
        app.init([&](){
            Texture m_texture(app.getGLBase());

            // get the root node of the UI scenegraph
            auto rootNode = app.getRootNode();

            // create a Div (simplest Element)
            vector<UINode*> parents;
            vector<UINode*> children;
            int nrElements = 15;
            int nrElementsPerRow = 5;
            int nrRows = (int)std::ceil((float)nrElements / (float)nrElementsPerRow);
            int margin = 20;
            glm::ivec2 elementSize = glm::ivec2( (app.getWinBase()->getWidth() - margin * (1 + nrElementsPerRow)) / nrElementsPerRow,
                                                 (app.getWinBase()->getHeight() - margin * (1 + nrRows)) / nrRows);

            vector< std::pair<pivotX, pivotY> > pivs = { { pivotX::center, pivotY::center },
                                                         { pivotX::left, pivotY::top },
                                                         { pivotX::right, pivotY::top },
                                                         { pivotX::right, pivotY::bottom },
                                                         { pivotX::left, pivotY::bottom } };

            vector< std::pair<align, valign> > align = {{align::center,    valign::center },
                                                        {align::center,    valign::center },
                                                        {align::center,    valign::center },
                                                        {align::center,    valign::center },
                                                        {align::center,    valign::center },
                                                        {align::center,    valign::center },
                                                        {align::left,  valign::top },
                                                        {align::right, valign::top },
                                                        {align::right, valign::bottom },
                                                        {align::left,  valign::bottom },
                                                        {align::center,    valign::center },
                                                        { align::left, valign::top },
                                                        { align::right, valign::top },
                                                        { align::right, valign::bottom },
                                                        { align::left, valign::bottom }};

            // x=left, y=top, z=right, w=bottom
            vector<glm::vec4> padding;
            for(int i=0; i<10;i++) padding.push_back(glm::vec4{0});

            padding.push_back(glm::vec4(0.f,0.f,0.f,0.f));
            padding.push_back(glm::vec4(10.f,10.f,0.f,0.f));
            padding.push_back(glm::vec4(0.f,10.f,10.f,0.f));
            padding.push_back(glm::vec4(0.f,0.f,10.f,10.f));
            padding.push_back(glm::vec4(10.f,0.f,0.f,10.f));


            for (int i=0; i<nrElements; i++)
            {
                int x = (i % nrElementsPerRow);
                int y = (i / nrElementsPerRow);
                parents.push_back( rootNode->addChild<Div>(margin * (x + 1) + x * elementSize.x,
                                                           margin * (y + 1) + y * elementSize.y,
                                                           elementSize.x, elementSize.y) );
                parents.back()->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
                parents.back()->setPadding(padding[i].x, padding[i].y, padding[i].z, padding[i].w);

                children.push_back( parents.back()->addChild<Div>() );
                children.back()->setSize(30, 30);
                children.back()->setAlign(align[i].first, align[i].second);
                children.back()->setPivot(pivs[i % pivs.size()].first, pivs[i % pivs.size()].second);
                children.back()->setBackgroundColor(0.f, 0.f, 1.f, 1.f);
            }

            EXPECT_EQ( postGLError(), GL_NO_ERROR); // be sure that there is no opengl error

            // ----------------------------------------------------------------------------------

            // manually draw - it would be done automatically after this function exits
            // but since we want to do our checks, and keep things simple - that is doing everything
            // right here - we do it explicitly
            app.getWinBase()->draw(0, 0, 0);
            app.getMainWindow()->swap();

            // ----------------------------------------------------------------------------------

            // check positions
            vector<glm::vec2> pos = {
                    vec2(73.f, 91.5f),
                    vec2(244.f, 106.5f),
                    vec2(370.f, 106.5f),
                    vec2(526.f, 76.5f),
                    vec2(712.f, 76.5f),
                    vec2(73.f, 284.5f),
                    vec2(176.f, 213.f),
                    vec2(438.f, 213.f),
                    vec2(594.f, 356.f),
                    vec2(644.f, 356.f),
                    vec2(73.f, 477.5f),
                    vec2(186.f, 416.f),
                    vec2(428.f, 416.f),
                    vec2(584.f, 539.f),
                    vec2(654.f, 539.f)
            };
            int i=0;
            for (auto &it : children){
                //LOG << glm::to_string(it->getWinPos());
                ASSERT_EQ(it->getWinPos().x, pos[i].x);
                ASSERT_EQ(it->getWinPos().y, pos[i].y);
                i++;
            }

            // ----------------------------------------------------------------------------------

            //m_texture.saveFrontBuffer("align_test_ref.png", app.winWidth, app.winHeight, 4);
            auto mainWin = app.getMainWindow()->getWinHandle();

            // read back and compare image
            vector<GLubyte> data((int)mainWin->getWidthReal() * (int)mainWin->getHeightReal() * 4);	// make some space to download
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glReadPixels(0, 0, (int)mainWin->getWidthReal(), (int)mainWin->getHeightReal(), GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);	// synchronous, blocking command, no swap() needed

            ASSERT_EQ(postGLError(), GL_NO_ERROR);

            pBitmap = m_texture.ImageLoader((filesystem::current_path() / "align_test.png").string().c_str(), 0);
            ASSERT_TRUE(pBitmap);

            // compare generated and reference image pixel by pixel
            uint8_t* texData = &data[0];
            uint8_t* refTex = (uint8_t*) FreeImage_GetBits(pBitmap);

            for (int y=0; y<app.getWinBase()->getHeight(); y++){
                for (int x=0; x<app.getWinBase()->getWidth(); x++){
                    for (int j=0; j<3; j++)
                        ASSERT_EQ(*(refTex++), *(texData++));

                    texData++; refTex++;
                }
            }
        });

        //app.startEventLoop(); // for debugging comment in this line in order to have to window stay

        // the above method will return when the draw function was executed once
        // exit immediately!
        app.exit();
    }
}