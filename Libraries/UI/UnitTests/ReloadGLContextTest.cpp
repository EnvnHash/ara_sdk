//
// Created by sven on 13-05-25.
//

#include "TestCommon.h"

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ReloadGLContextTests {

TEST(UITest, ReloadGLContextTests) {
    ivec2 winSize{400,400};
    vec4 col = { 1.f, 0.f, 0.f, 1.f };
    ivec2 size = { 200, 200 };

    ara::UIApplication app;
    app.setEnableMenuBar(false);
    app.setEnableWindowResizeHandles(false);
    app.setWinSize(winSize);

    app.init([&] {
        auto mainWin = app.getMainWindow();

        auto div = mainWin->getRootNode()->addChild<Div>();
        div->setSize(size.x, size.y);
        div->setBackgroundColor(col);
        app.getWinBase()->draw(0, 0, 0);
        mainWin->swap();

        checkQuad(app.getWinBase()->getWinHandle(), { 0, 0 }, size, col, {});
        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
    });

    // remove all gl resources, but leave the window and its UINode tree untouched
    app.stop();
    app.stopGLBaseRenderLoop();

    app.getMainWindow()->removeGLResources(false); // make the window release all it's opengl resources
    app.getGLBase()->destroy(false); // remove glbase opengl resources

    // rebuild the context
    app.initGLBase(); // no context current after this call

    app.startUiThread([&] {
        app.getMainWindow()->init(UIWindowParams{
            .glbase = app.getGLBase(),
            .size = { app.getMainWindow()->getWidth(), app.getMainWindow()->getHeight() },
            .shift = app.getMainWindow()->getPosition(),
            .initToCurrentCtx = app.getMainWindow()->usingSelfManagedCtx(),
            .multisample = app.getMainWindow()->usingMultiSample(),
        });

        UINode::itrNodes(app.getMainWindow()->getRootNode(), [](UINode* node) {
            node->reqUpdtTree();
        });

        app.getMainWindow()->getProcSteps()->at(Draw).active = true;
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        checkQuad(app.getWinBase()->getWinHandle(), { 0, 0 }, size, col, {});
        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
    });

    app.exit();
}

}