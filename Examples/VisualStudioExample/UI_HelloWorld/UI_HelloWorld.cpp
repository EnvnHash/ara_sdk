
#include "UI_HelloWorld.h"

using namespace std;
using namespace ara;
using namespace ara;
using namespace ara;

void UI_HelloWorld::init(std::function<void()> initCb)
{
    m_mainWindow = addWindow(1600, 1000, 100, 100, false);

    createContainerDiv();
    createStyleSheetDiv();
    createImages();

    startRenderLoop();  // main UIApplication renderloop (for all windows) -> blocking
}


void UI_HelloWorld::createContainerDiv()
{
    // get the rootNode of the UI scenegraph
    auto rootNode = m_mainWindow->getRootNode();

    // add a child to the root node, let's take the simplest element, a Div which is just a box
    m_div = rootNode->addChild<Div>();

    // layout via C++
    m_div->setWidth(1400);                             // set Size (note: by default elements are 100% the size of their parents) an integer is interpreted as pixels
    m_div->setHeight(0.9f);                           // floats are interpreted as percentages (0-1 means 0 - 100%)
    m_div->setBackgroundColor(0.1f, 0.1f, 0.1f, 1.f); // color can be defined as normalized rgba values from 0 - 1 or as glm::vec4
    m_div->setAlign(align::center, valign::center);   // position this Div to the center of it's parent
    m_div->setBorderRadius(20);                       // border radius is always in pixels.
    m_div->setBorderWidth(10);                        // border width is always in pixels
    m_div->setBorderColor(0.6f, 0.6f, 0.6f, 0.5f);    // color can be defined as normalized rgba values from 0 - 1 or as glm::vec4

    // note: those layout parameters are available in all UI-elements
}


void UI_HelloWorld::createStyleSheetDiv()
{
    // create a Label - the basic element for displaying simple text - and layout via stylesheet
    auto label = m_div->addChild<Label>("demo_style");

    // open the res.txt inside the UISDK/resdata folder in any text editor an play around with the "demo_style" definition
    // when saving the file, the app will automatically update

    // make it clickable
    label->addMouseClickCb([this](hidData& data) {
        LOG << "hello";
    });
}


void UI_HelloWorld::createImages()
{

    auto image = m_div->addChild<Image>("chtest.sample-image");
    image->setPos(500, 100);
    image->setSize(400, 400);
}
