//
// Created by sven on 03-04-25.
//

#pragma once

#include <gtest/gtest.h>
#include <UIApplication.h>
#include <Utils/Texture.h>
#include <WindowManagement/GLWindow.h>

#include "threadpool/BS_thread_pool.hpp"

static inline BS::thread_pool g_thread_pool;
static inline glm::vec2       contentScale{1.f, 1.f};

static void stdAppSetup(ara::UIApplication& app, int width, int height) {
    app.setWinWidth(width);
    app.setWinHeight(height);
    app.setEnableMenuBar(false);
    app.setScaleToMonitor(false);
    app.setEnableWindowResizeHandles(false);
}

static void appBody(const std::function<void(ara::UIApplication&)>& drawFunc,
                    const std::function<void(ara::UIApplication&)>& verifyFunc,
                    int width=1280, int height=720,
                    const std::function<void(ara::UIApplication&)>& postInitFunc=nullptr) { // width and height are in hardware pixels (non-scaled)
    ara::UIApplication app;
    stdAppSetup(app, width, height);

    app.initSingleThreaded([&]{
        drawFunc(app);

        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        verifyFunc(app);
        app.setRunFlag(false); // for debugging comment this line in order to have to window stay
    });

    if (postInitFunc) {
        postInitFunc(app);
    }

    app.exit();
}

static void appRestartGL(const std::function<void(ara::UIApplication&)>& drawFunc,
                         const std::function<void(ara::UIApplication&)>& verifyFunc,
                         int width = 1280, int height = 720) {
    auto postVerifyFunc = [&](ara::UIApplication& app) {
        // remove all gl resources, but leave the window and its UINode tree untouched
        app.stopGLBaseProcCallbackLoop();

        app.getMainWindow()->removeGLResources(); // make the window release all it's opengl resources
        app.getGLBase()->destroy(false); // remove glbase opengl resources

        // rebuild the context
        app.initGLBase(); // no context current after this call

        app.initThread([&] {
            app.getMainWindow()->init(ara::UIWindowParams{
                    .glbase = app.getGLBase(),
                    .size = { app.getMainWindow()->getWidth(), app.getMainWindow()->getHeight() },
                    .shift = app.getMainWindow()->getPosition(),
                    .initToCurrentCtx = app.getMainWindow()->usingSelfManagedCtx(),
                    .multisample = app.getMainWindow()->usingMultiSample(),
            });

            ara::UINode::itrNodes(app.getMainWindow()->getRootNode(), [](ara::UINode* node) {
                node->reqUpdtTree();
            });

            app.getMainWindow()->getProcSteps()->at(ara::Draw).active = true;
            app.getWinBase()->draw(0, 0, 0);
            app.getMainWindow()->swap();

            verifyFunc(app);
            app.setRunFlag(false);
        });
    };

    appBody(drawFunc, verifyFunc, width, height, postVerifyFunc);
}

static std::vector<GLubyte> getPixels(int x, int y, uint32_t width, uint32_t height) {
    std::vector<GLubyte> data(width * height * 4);	// make some space to download
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(x, y, static_cast<GLint>(width), static_cast<GLint>(height), GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);	// synchronous, blocking command, no swap() needed
    return data;
}

static void compareBitmaps(const std::vector<GLubyte>& data, const std::filesystem::path& p, uint32_t width, uint32_t height, uint8_t eps) {
    if (!std::filesystem::exists(p)) {
        LOGE << "compareBitmaps error, couldn't load " << p.string();
        return;
    }
    FIBITMAP* pBitmap = ara::Texture::ImageLoader(p.string().c_str(), 0);
    ASSERT_TRUE(pBitmap);

    std::list<std::future<void>> futures;
    const auto tc = g_thread_pool.get_thread_count();
    uint32_t ySlices = height / tc;
    
    for (uint32_t i=0; i<tc; i++) {
        futures.emplace_back(g_thread_pool.submit([&data, pBitmap, width, ySlices, i, eps]() {
            auto texData = &data[0];
            auto refTex = FreeImage_GetBits(pBitmap);

            const uint32_t offs = (ySlices * width * 4) * i;
            texData += offs;
            refTex += offs;

            for (uint32_t y = 0; y < ySlices; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    // freeimage reads in BGRA format, but textures are supposed to be in RGBA
                    EXPECT_NEAR(*(refTex+2),  *(texData), eps);
                    EXPECT_NEAR(*(refTex+1),  *(texData +1), eps);
                    EXPECT_NEAR(*(refTex),    *(texData +2), eps);
                    EXPECT_NEAR(*(refTex+3),  *(texData +3), eps);

                    refTex += 4;
                    texData += 4;
                }
            }
        }));
    }

    for (auto &it : futures) {
        if (it.valid()) {
            it.wait();
        }
    };
}

static void compareFrameBufferToImage(const std::filesystem::path& p, uint32_t width, uint32_t height, uint8_t eps=0) {
    auto data = getPixels(0, 0, width, height);
    compareBitmaps(data, p, width, height, eps);
}

struct checkPix {
    glm::ivec2 pos{};
    glm::vec4 col{};
};

static void checkVals(const std::vector<GLubyte>& data, ara::GLWindow* mainWin, const std::vector<checkPix>& cv) {
    for (auto &it : cv) {
        auto ptr = (it.pos.x + it.pos.y * mainWin->getWidthReal()) * 4;
        ASSERT_EQ(data[ptr], it.col.r * 255);
        ASSERT_EQ(data[ptr +1], it.col.g * 255);
        ASSERT_EQ(data[ptr +2], it.col.b * 255);
        ASSERT_EQ(data[ptr +3], it.col.a * 255);
    }
}

static void checkQuad(ara::GLWindow* win, const glm::ivec2& virtPos, const glm::ivec2& virtSize, const glm::ivec4& col,
                      const glm::ivec4& backCol) {
    auto data = getPixels(0, 0, win->getWidthReal(), win->getHeightReal());

    // convert from virtual to hardware pixels
    glm::ivec2 size { win->virt2RealX(virtSize.x) -1, win->virt2RealY(virtSize.y) -1 };
    glm::ivec2 pos { win->virt2RealX(virtPos.x), win->virt2RealY(virtPos.y) };

    std::array<glm::ivec2, 4> edges {
        pos,                        // left-top
        { pos.x + size.x, pos.y },  // right-top
        { pos.x, pos.y + size.y },  // left-bottom,
        pos + size                  // right-bottom
    };

    std::array<std::array<glm::ivec2, 2>, 4> edgeOffsets {
        std::array<glm::ivec2, 2>{ glm::ivec2{ -1, 0 }, glm::ivec2{ 0, -1 } },   // left-top
        std::array<glm::ivec2, 2>{ glm::ivec2{  1, 0 }, glm::ivec2{ 0, -1 } },   // right-top
        std::array<glm::ivec2, 2>{ glm::ivec2{ -1, 0 }, glm::ivec2{ 0,  1 } },   // left-bottom,
        std::array<glm::ivec2, 2>{ glm::ivec2{  1, 0 }, glm::ivec2{ 0,  1 } }    // right-bottom
    };

    // check edges for front color
    std::vector<checkPix> checkPixels;
    for (auto i=0; i<edges.size(); ++i) {
        checkPixels.emplace_back(checkPix{edges[i], col});
        for (auto j=0; j<2; ++j) {
            auto p = edges[i] + edgeOffsets[i][j];
            if (p.x > 0 && p.y > 0 && p.x < static_cast<float>(win->getWidthReal()) && p.y < static_cast<float>(win->getHeightReal())) {
                checkPixels.emplace_back(checkPix{p, backCol});
            }
        }
    }

    // flip y-axis
    std::ranges::transform(checkPixels.begin(), checkPixels.end(), checkPixels.begin(), [win](auto& it) {
        return checkPix{.pos = { it.pos.x, win->getHeightReal() -1 -it.pos.y },
                        .col = it.col };
    });

    // check outside edges for back color
    checkVals(data, win, checkPixels);
}
