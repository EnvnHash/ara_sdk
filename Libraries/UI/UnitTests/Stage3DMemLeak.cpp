#include <gtest/gtest.h>
#include "GLBase.h"
#include "UIApplication.h"
#include "UIElements/Scene3D.h"

#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace std;

namespace ara::SceneGraphUnitTest::Stage3DMemLeak
{
	ShaderCollector shCol;  // create a ShaderCollector
	GLFWWindow gwin;
	Shaders* colShader;
    glWinPar gp;             // define Parameters for windows instanciating

	bool didFindMemLeak = false;

	TEST(SceneGraphUnitTest, Stage3DMemLeak)
	{
        UIApplication app;
        app.winWidth = 200;
        app.winHeight = 100;
        app.setEnableMenuBar(false);
        app.setEnableWindowResizeHandles(false);

        glm::vec4 fgColor(0.f, 0.f, 1.f, 1.f);
        glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 1.f);

        app.init([&](){
            // get the root node of the UI scenegraph
            auto rootNode = app.getRootNode();

#ifdef _WIN32
            _CrtMemState sOld;
            _CrtMemState sNew;
            _CrtMemState sDiff;
            _CrtMemCheckpoint(&sOld); //take a snapchot
#endif
            for (int i=0; i<10; i++)
            {
                auto m_scene3D = rootNode->addChild<Scene3D>();

                app.getMainWindow()->getSharedRes()->setDrawFlag();
                app.getMainWindow()->draw(0,0,0);

                m_scene3D->freeGLResources();
                rootNode->remove_child(m_scene3D);

#ifdef _WIN32
                _CrtMemCheckpoint(&sNew); //take a snapchot
                if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
                {
                    LOG << "-----------_CrtMemDumpStatistics ---------";
                    _CrtMemDumpStatistics(&sDiff);
                    OutputDebugString("-----------_CrtMemDumpAllObjectsSince ---------");
                    _CrtMemDumpAllObjectsSince(&sOld);
                    OutputDebugString("-----------_CrtDumpMemoryLeaks ---------");
                    _CrtDumpMemoryLeaks();
                    didFindMemLeak = true;
                }
#endif
            }

            EXPECT_FALSE(didFindMemLeak);
        });

        //app.startEventLoop(); // for debugging comment in this line in order to have to window stay

        // the above method will return when the draw function was executed once
        // exit immediately!
        app.exit();
	}
}

