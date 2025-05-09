//
// Created by sven on 03-04-25.
//

#pragma once

#include <gtest/gtest.h>
#include "UIApplication.h"
#include "Utils/Texture.h"



#include "threadpool/BS_thread_pool.hpp"

static inline BS::thread_pool g_thread_pool;
static inline glm::vec2       contentScale{1.f, 1.f};

static void appBody(const std::function<void(ara::UIApplication*)>& drawFunc,
                    const std::function<void(ara::UIApplication*)>& verifyFunc, 
                    int width = 1280, int height = 720) { // width and height are in hardware pixels (non-scaled)
    ara::UIApplication app;
    app.setWinWidth(width);
    app.setWinHeight(height);
    app.setEnableMenuBar(false);
    app.setScaleToMonitor(false);
    app.setEnableWindowResizeHandles(false);
    app.init([&](){
        auto mainWin = app.getMainWindow();
        drawFunc(&app);

        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
        app.getWinBase()->draw(0, 0, 0);
        mainWin->swap();

        verifyFunc(&app);
    });

    // app.startEventLoop(); // for debugging comment in this line in order to have to window stay
    app.exit();
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