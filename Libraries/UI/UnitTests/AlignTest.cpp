//
// Created by sven on 11/15/20.
//

#include "test_common.h"
#include "UIApplication.h"
#include <UIElements/Div.h>

using namespace std;

namespace ara::SceneGraphUnitTest::AlignTest {

struct checkPix {
    glm::ivec2 pos{};
    glm::ivec4 col{};
};

size_t getPtr(size_t xPos, size_t yPos, GLWindow* win) {
    return (win->getWidthReal() * yPos + xPos) * 4;
}

void checkVals(const std::vector<GLubyte>& data, GLWindow* mainWin, std::vector<checkPix>&& cv) {
    for (auto &it : cv) {
        auto ptr = getPtr(it.pos.x, it.pos.y, mainWin);
        ASSERT_EQ(data[ptr], it.col.r);
        ASSERT_EQ(data[ptr +1], it.col.g);
        ASSERT_EQ(data[ptr +2], it.col.b);
        ASSERT_EQ(data[ptr +3], it.col.a);
    }
}

TEST(UITest, AlignLeftBottom) {
    int width = 200;
    int height = 100;

    appBody([&](UIApplication* app){
        auto win = app->getMainWindow();
        auto div = win->getRootNode()->addChild<Div>();
        div->setSize(width, height);
        div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
        div->setAlign(align::left, valign::bottom);
    },
    [&](UIApplication* app){
        auto mainWin = app->getWinBase()->getWinHandle();
        auto data = getPixels(0, 0, mainWin->getWidthReal(), mainWin->getHeightReal());
        checkVals(data, mainWin, std::vector<checkPix>{
            {
                {0, 0}, // div bottom left corner of div in relation to downloaded framebuffer
                {255, 0, 0, 255}
            },
            {
                // div top right corner of div in relation to downloaded framebuffer
                { static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                  static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y) -1},
                {255, 0, 0, 255}
            },
            {
                // div top and right +1 corner of div in relation to downloaded framebuffer
                { static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y) },
                {0, 0, 0, 0}
            },
            {
                // div top +1 and right corner of div in relation to downloaded framebuffer
                { static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x),
                static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y) -1 },
                {0, 0, 0, 0}
            }
        });
    });
}

TEST(UITest, AlignRightBottom) {
    int width = 200;
    int height = 100;
    appBody([&](UIApplication* app){
        auto win = app->getMainWindow();
        auto div = win->getRootNode()->addChild<Div>();
        div->setSize(width, height);
        div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);

        div->setAlign(align::right, valign::bottom);
    }, [&](UIApplication* app){
        auto mainWin = app->getWinBase()->getWinHandle();
        auto data = getPixels(0, 0, mainWin->getWidth(), mainWin->getHeight());
        checkVals(data, mainWin, std::vector<checkPix>{
            {
                // div bottom left corner of div in relation to downloaded framebuffer
                { mainWin->getWidthReal() - static_cast<int>(width * mainWin->getContentScale().x), 0 },
                { 255, 0, 0, 255 }
            },
            {
                // div top right corner of div in relation to downloaded framebuffer
                { mainWin->getWidthReal() -1, static_cast<int>(height * mainWin->getContentScale().y) -1 },
                { 255, 0, 0, 255 }
            },
            {
                // div top and left +1 corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() - static_cast<int>(width * mainWin->getContentScale().x),
                    static_cast<int>(height * mainWin->getContentScale().y) -1
                },
                { 255, 0, 0, 255 }
            },
            {
                // div top +1 and left corner of div in relation to downloaded framebuffer
                { mainWin->getWidthReal() -1, static_cast<int>(height * mainWin->getContentScale().y) -1 },
                { 255, 0, 0, 255 }
            }
        });
    });
}

TEST(UITest, AlignLeftTop) {
    int width = 200;
    int height = 100;
    appBody([&](UIApplication* app){
        auto win = app->getMainWindow();
        auto div = win->getRootNode()->addChild<Div>();
        div->setSize(width, height);
        div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
        div->setAlign(align::left, valign::top);
    }, [&](UIApplication* app){
        auto mainWin = app->getWinBase()->getWinHandle();
        auto data = getPixels(0, 0, mainWin->getWidthReal(), mainWin->getHeightReal());
        checkVals(data, mainWin, std::vector<checkPix>{
            {
                // div top left corner of div in relation to downloaded framebuffer
                { 0, mainWin->getHeightReal() -1 },
                { 255, 0, 0, 255 }
            },
            {
                // div bottom right corner of div in relation to downloaded framebuffer
                {
                    static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y)
                },
                { 255, 0, 0, 255 }
            },
            {
                // div bottom and right +1 corner of div in relation to downloaded framebuffer
                {
                    static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y) -1
                },
                { 0, 0, 0, 0 }
            },
            {
                // div top +1 and left corner of div in relation to downloaded framebuffer
                {
                    static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y +1)
                },
                { 0, 0, 0, 0 }
            },
            {
                // div top and right +1 corner of div in relation to downloaded framebuffer
                {
                    static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y +1)
                },
                { 0, 0, 0, 0 }
            }
        });
    });
}

TEST(UITest, AlignRightTop) {
    int width = 200;
    int height = 100;
    appBody([&](UIApplication* app){
        auto win = app->getMainWindow();
        auto div = win->getRootNode()->addChild<Div>();
        div->setSize(width, height);
        div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
        div->setAlign(align::right, valign::top);
    }, [&](UIApplication* app){
        auto mainWin = app->getWinBase()->getWinHandle();
        auto data = getPixels(0, 0, mainWin->getWidthReal(), mainWin->getHeightReal());
        checkVals(data, mainWin, std::vector<checkPix>{
            {
                // div top right corner of div in relation to downloaded framebuffer
                { mainWin->getWidthReal() - 1, mainWin->getHeightReal() - 1 },
                {255, 0, 0, 255}
            },
            {
                // div bottom left corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() - static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x),
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y)
                },
                {255, 0, 0, 255}
            },
            {
                // div bottom and left -1 corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() - static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) -1,
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y)
                },
                {0, 0, 0, 0}
            },
            {
                // div bottom +1 and left corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() - static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x),
                    mainWin->getHeightReal() - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().y) -1
                },
                {0, 0, 0, 0}
            }
        });
    });
}

TEST(UITest, AlignCenter) {
    int width = 200;
    int height = 100;
    appBody([&](UIApplication* app){
        auto win = app->getMainWindow();
        auto div = win->getRootNode()->addChild<Div>();
        div->setSize(width, height);
        div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
        div->setAlign(align::center, valign::center);
    }, [&](UIApplication* app){
        auto mainWin = app->getWinBase()->getWinHandle();
        auto data = getPixels(0, 0, mainWin->getWidthReal(), mainWin->getHeightReal());
        checkVals(data, mainWin, std::vector<checkPix>{
            {
                // div bottom left corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() /2 - static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) /2,
                    mainWin->getHeightReal()/2 - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().x) /2
                },
                {255, 0, 0, 255}
            },
            {
                // div top right corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() /2 + static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) /2 -1,
                    mainWin->getHeightReal()/2 + static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().x) /2 -1
                },
                {255, 0, 0, 255}
            },
            {
                // div bottom -1 and left -1 corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() /2 - static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) /2 -1,
                    mainWin->getHeightReal()/2 - static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().x) /2 -1
                },
                {0, 0, 0, 0}
            },
            {
                // div top right corner of div in relation to downloaded framebuffer
                {
                    mainWin->getWidthReal() /2 + static_cast<int>(static_cast<float>(width) * mainWin->getContentScale().x) /2,
                    mainWin->getHeightReal()/2 + static_cast<int>(static_cast<float>(height) * mainWin->getContentScale().x) /2
                },
                {0, 0, 0, 0}
            },
        });
    });
}

TEST(UITest, BorderRadiusOutOfBoundsLimit) {
    int width = 200;
    int height = 200;
    appBody([&](UIApplication* app){
        auto win = app->getMainWindow();
        auto div = win->getRootNode()->addChild<Div>();
        div->setX(-100);
        div->setY(100);
        div->setSize(width, height);
        div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
        div->setAlign(align::left, valign::bottom);
        div->setBorderRadius(40);
        div->setBorderWidth(20);
        div->setBorderColor(0.f, 0.5f, 1.f, 1.f);
    }, [&](UIApplication* app){
        auto mainWin = app->getWinBase();
        compareFrameBufferToImage(filesystem::current_path() / "border_radius_oob.png", mainWin->getWidth(), mainWin->getHeight());
    }, 200, 200);
}

}