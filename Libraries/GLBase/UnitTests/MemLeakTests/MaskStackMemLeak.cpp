#include <GLBase.h>
#include <Utils/MaskStack.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace std;

namespace ara::GLBaseUnitTest::MaskStackMemLeak {
    ShaderCollector shCol;  // create a ShaderCollector
    GLFWWindow gwin;
    glWinPar gp;             // define Parameters for windows instanciating

    bool didFindMemLeak = false;

    TEST(GLBaseTest, MaskStackMemLeak
    ) {
    // direct window creation
    // gp.debug = true;
    gp.
    width = 1920;            // set the windows width
    gp.
    height = 1080;            // set the windows height
    gp.
    doInit = true;            // GWindow needs GLFW library to be m_inited. GWindow can do this itself. Standard is true
    gp.
    shiftX = 100;            // x offset relative to OS screen canvas
    gp.
    shiftY = 100;            // y offset relative to OS screen canvas
    gp.
    scaleToMonitor = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours
    gp.
    createHidden = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours

    ASSERT_TRUE(gwin
    .
    create(gp)
    );    // now pass the arguments and create the window

    // init glew
    glewExperimental = GL_TRUE;
    if (GLEW_OK !=

    glewInit()

    ) { LOGE

    << " m_glbase.initToThisCtx Error couldn't init GLEW "; return;
}

glGetError();    // delete glew standard error (bug in glew)

#ifdef _WIN32
_CrtMemState sOld, sNew, sDiff;
_CrtMemCheckpoint(&sOld); //take a snapshot
#endif

for (
int i = 0;
i<10; i++)
{
auto stack = make_unique<MaskStack>();
stack->
setShaderCollector(&shCol);
auto layer = stack->addLayer(maskType::Vector);

auto p = layer->addPoint(0, -0.5f, 0.5f, 0.f, 1.f);
p->
texBasePoint = true;

p = layer->addPoint(0, -0.5f, -0.5f, 0.f, 0.f);
p->
texBasePoint = true;

p = layer->addPoint(0, 0.5f, -0.5f, 1.f, 0.f);
p->
texBasePoint = true;

p = layer->addPoint(0, 0.5f, 0.5f, 1.f, 1.f);
p->
texBasePoint = true;

stack->

update();

gwin.

swap();

stack.

reset();

shCol.

clear();

#ifdef _WIN32
_CrtMemCheckpoint(&sNew); //take a snapshot
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
}
}

