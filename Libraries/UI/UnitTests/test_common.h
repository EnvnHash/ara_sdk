//
// Created by sven on 03-04-25.
//

#pragma once

#include <gtest/gtest.h>
#include <UIApplication.h>
#include <Utils/Texture.h>
#include <BS_thread_pool.hpp>

static inline BS::thread_pool g_thread_pool;
static inline glm::vec2       contentScale{1.f, 1.f};

static void appBody(const std::function<void(ara::UIApplication*)>& drawFunc,
                    const std::function<void(ara::UIApplication*)>& verifyFunc, 
                    int width = 1280, int height = 720) { // width and height are in hardware pixels (non scaled)
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

    /*
     auto texData = &data[0];

      for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (static_cast<int>(*(texData)) != 0) {
                LOG << "[" << x << ":" << y << "]" << static_cast<int>(*(texData)) << ", " << static_cast<int>(*(texData + 1)) << ", "
                    << static_cast<int>(*(texData + 2)) << ", " << static_cast<int>(*(texData + 3));
            }
            texData += 4;
        }
      }*/
    return data;
}

static void compareBitmaps(const std::vector<GLubyte>& data, const std::filesystem::path& p, uint32_t width, uint32_t height) {
    if (!std::filesystem::exists(p)) {
        LOGE << "compareBitmaps error, couldn't load " << p.string();
        return;
    }
    FIBITMAP* pBitmap = ara::Texture::ImageLoader(p.string().c_str(), 0);
    ASSERT_TRUE(pBitmap);

    /*
    auto texData = &data[0];

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (static_cast<int>(*(texData)) != 0) {
                LOG << "[" << x << ":" << y << "]" << static_cast<int>(*(texData)) << ", "
                    << static_cast<int>(*(texData + 1)) << ", " << static_cast<int>(*(texData + 2)) << ", "
                    << static_cast<int>(*(texData + 3));
            }
            texData += 4;
        }
    }*/

    std::list<std::future<void>> futures;
    auto tc = g_thread_pool.get_thread_count();
    uint32_t ySlices = height / tc;
    
    for (uint32_t i=0; i<tc; i++) {
        futures.emplace_back(g_thread_pool.submit([&] {
            auto texData = &data[0];
            auto refTex = static_cast<uint8_t*>(FreeImage_GetBits(pBitmap));

            uint32_t offs = (ySlices * width * 4) * i;
            texData += offs;
            refTex += offs;

            for (uint32_t y = 0; y < ySlices; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    // freeimage reads in BGRA format, but textures are supposed to be in RGBA
                    ASSERT_EQ(*(refTex+2),  *(texData));
                    ASSERT_EQ(*(refTex+1),  *(texData +1));
                    ASSERT_EQ(*(refTex),    *(texData +2));
                    ASSERT_EQ(*(refTex+3),  *(texData +3));

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

static void compareFrameBufferToImage(const std::filesystem::path& p, uint32_t width, uint32_t height) {
    auto data = getPixels(0, 0, width, height);
    compareBitmaps(data, p, width, height);
}