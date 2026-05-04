//
// Created by sven on 04-05-26.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Div.h>
#include "UIElements/Resizable.h"

using namespace glm;
using namespace std;

namespace ara::UiUnitTest::ResizableTest {

static void dragResizableHandle(const UIApplication& app, const ivec2& mouseDownPos, const ivec2& moveDelta) {
    const auto mainWin = app.getMainWindow();
    mainWin->onMouseDownLeft(static_cast<float>(mouseDownPos.x), static_cast<float>(mouseDownPos.y), false, false, false);

    const auto movePos = mouseDownPos + moveDelta;
    mainWin->onMouseMove(static_cast<float>(movePos.x), static_cast<float>(movePos.y), 0);
    mainWin->onMouseUpLeft();
    iterate(app);
}

struct ResizableDragTestCase {
    ResizableHandle::Corner corner;
    ivec2                  mouseDownPos;
    ivec2                  expectedPos;
    ivec2                  expectedSize;
};

Resizable& addResizable(UINode& rootNode, const ivec2& pos, const ivec2& size, const align alignX = align::left,
    const valign alignY = valign::top) {
    auto& r = rootNode.push<Resizable>({
        .pos = pos,
        .size = size,
        .align = alignX,
        .valign = alignY,
        .borderWidth = 1,
        .borderColor = vec4{1.f, 1.f, 1.f, 1.f,},
    });
    r.setSnapToHwPix(true);
    return r;
}

TEST(UITest, ResizableBasicTest) {
    appBody([&](const UIApplication& app){
        addResizable(*app.getMainWindow()->getRootNode(), {}, {200, 150});
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "Resizable_basic_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 400, 300);
}

TEST(UITest, ResizableMoveAllHandlesTest) {
    Resizable* resizable = nullptr;
    constexpr ivec2 startPos{100, 75};
    constexpr ivec2 startSize{200, 150};
    constexpr ivec2 moveDelta{50, 50};

    appBody([&](const UIApplication& app){
        resizable = &addResizable(*app.getMainWindow()->getRootNode(), startPos, startSize);
    }, [&](const UIApplication& app){
        ASSERT_NE(resizable, nullptr);

        const array testCases{
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::topLeft,
                .mouseDownPos = startPos,
                .expectedPos = startPos + moveDelta,
                .expectedSize = startSize - moveDelta
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::topRight,
                .mouseDownPos = startPos + ivec2{startSize.x, 0},
                .expectedPos = startPos + ivec2{0, moveDelta.y},
                .expectedSize = startSize + ivec2{moveDelta.x, -moveDelta.y}
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::bottomRight,
                .mouseDownPos = startPos + startSize,
                .expectedPos = startPos,
                .expectedSize = startSize + moveDelta
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::bottomLeft,
                .mouseDownPos = startPos + ivec2{0, startSize.y},
                .expectedPos = startPos + ivec2{moveDelta.x, 0},
                .expectedSize = startSize + ivec2{-moveDelta.x, moveDelta.y}
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::top,
                .mouseDownPos = startPos + ivec2{startSize.x / 2, 0},
                .expectedPos = startPos + ivec2{0, moveDelta.y},
                .expectedSize = startSize + ivec2{0, -moveDelta.y}
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::right,
                .mouseDownPos = startPos + ivec2{startSize.x, startSize.y / 2},
                .expectedPos = startPos,
                .expectedSize = startSize + ivec2{moveDelta.x, 0}
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::bottom,
                .mouseDownPos = startPos + ivec2{startSize.x / 2, startSize.y},
                .expectedPos = startPos,
                .expectedSize = startSize + ivec2{0, moveDelta.y}
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::left,
                .mouseDownPos = startPos + ivec2{0, startSize.y / 2},
                .expectedPos = startPos + ivec2{moveDelta.x, 0},
                .expectedSize = startSize + ivec2{-moveDelta.x, 0}
            },
            ResizableDragTestCase{
                .corner = ResizableHandle::Corner::center,
                .mouseDownPos = startPos + startSize / 2,
                .expectedPos = startPos + moveDelta,
                .expectedSize = startSize
            }
        };

        for (const auto& testCase : testCases) {
            SCOPED_TRACE(static_cast<int32_t>(testCase.corner));
            resizable->setPos(startPos);
            resizable->setSize(startSize);
            dragResizableHandle(app, testCase.mouseDownPos, moveDelta);
            EXPECT_EQ(ivec2(resizable->getPos()), testCase.expectedPos);
            EXPECT_EQ(ivec2(resizable->getSize()), testCase.expectedSize);
        }
    }, 400, 300);
}



}