//
// Created by sven on 03-04-25.
//

#pragma once

#include <gtest/gtest.h>
#include <UIApplication.h>
#include <Utils/Texture.h>
#include <WindowManagement/GLWindow.h>
#include <UIElements/DataBinding/NodeEdit.h>
#include <UIElements/Button/Button.h>

#include "threadpool/BS_thread_pool.hpp"

static inline BS::thread_pool g_thread_pool;
static inline glm::vec2       contentScale{1.f, 1.f};

static void drawAndSwap(const ara::UIApplication& app) {
    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
}

static void iterate(const ara::UIApplication& app) {
    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
}

static void stdAppSetup(ara::UIApplication& app, const int width, const int height, bool enableMenuAndResizeHandles=false) {
    app.setWinWidth(width);
    app.setWinHeight(height);
    app.setEnableMenuBar(enableMenuAndResizeHandles);
    app.setEnableWindowResizeHandles(enableMenuAndResizeHandles);
    app.setScaleToMonitor(false);
}

static void appBody(const std::function<void(ara::UIApplication&)>& drawFunc,
                    const std::function<void(ara::UIApplication&)>& verifyFunc,
                    const int width=1280, const int height=720,
                    const std::function<void(ara::UIApplication&)>& postInitFunc=nullptr,
                    const bool enableMenuAndResizeHandles=false,
                    const std::string& resFile="res.txt") { // width and height are in hardware pixels (non-scaled)
    ara::UIApplication app;
    stdAppSetup(app, width, height, enableMenuAndResizeHandles);
    app.setResFile(resFile);
    app.initSingleThreaded([&]{
        app.getMainWindow()->getWinHandle()->setIsInited(true);
        app.getMainWindow()->getWinHandle()->setIsRunning(true);

        drawFunc(app);

        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        verifyFunc(app);

        app.getMainWindow()->getWinHandle()->setIsRunning(false);
        app.setRunFlag(false); // for debugging comment this line in order to have to window stay
    });

    if (postInitFunc) {
        postInitFunc(app);
    }

    app.exit();
}

static void appRestartGL(const std::function<void(ara::UIApplication&)>& drawFunc,
                         const std::function<void(ara::UIApplication&)>& verifyFunc,
                         const int width = 1280, const int height = 720) {
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

            ara::UINode::itrNodes(app.getMainWindow()->getRootNode(), [](const ara::UINode* node) {
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

static std::vector<GLubyte> getPixels(const int x, const int y, const uint32_t width, const uint32_t height) {
    std::vector<GLubyte> data(width * height * 4);	// make some space to download
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(x, y, static_cast<GLint>(width), static_cast<GLint>(height), GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);	// synchronous, blocking command, no swap() needed
    return data;
}

static void compareBitmaps(const std::vector<GLubyte>& data, const std::filesystem::path& p, uint32_t width, const uint32_t height, uint8_t eps) {
    if (!std::filesystem::exists(p)) {
        LOGE << "compareBitmaps error, couldn't load " << p.string();
        return;
    }
    auto pBitmap = ara::FreeImage::Load(p.string(), nullptr);
    ASSERT_TRUE(pBitmap);

    std::list<std::future<void>> futures;
    const auto tc = g_thread_pool.get_thread_count();
    auto ySlices = height / static_cast<uint32_t>(tc);
    
    for (uint32_t i=0; i<tc; i++) {
        futures.emplace_back(g_thread_pool.submit_task([&data, pBitmap, width, ySlices, i, eps]() {
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

static void compareFrameBufferToImage(const std::filesystem::path& p, const uint32_t width, const uint32_t height, const uint8_t eps=0) {
    const auto data = getPixels(0, 0, width, height);
    compareBitmaps(data, p, width, height, eps);
}

struct checkPix {
    glm::ivec2 pos{};
    glm::vec4 col{};
};

static void checkVals(const std::vector<GLubyte>& data, const ara::GLWindow* mainWin, const std::vector<checkPix>& cv) {
    for (const auto &[pos, col] : cv) {
        const auto ptr = (pos.x + pos.y * mainWin->getWidthReal()) * 4;
        ASSERT_EQ(data[ptr], static_cast<GLubyte>(col.r * 255));
        ASSERT_EQ(data[ptr +1], static_cast<GLubyte>(col.g * 255));
        ASSERT_EQ(data[ptr +2], static_cast<GLubyte>(col.b * 255));
        ASSERT_EQ(data[ptr +3], static_cast<GLubyte>(col.a * 255));
    }
}

static void checkQuad(ara::GLWindow* win, const glm::ivec2& virtPos, const glm::ivec2& virtSize, const glm::vec4& col,
                      const glm::vec4& backCol) {
    const auto data = getPixels(0, 0, win->getWidthReal(), win->getHeightReal());

    // convert from virtual to hardware pixels
    const glm::ivec2 size { win->virt2RealX(virtSize.x) -1, win->virt2RealY(virtSize.y) -1 };
    glm::ivec2 pos { win->virt2RealX(virtPos.x), win->virt2RealY(virtPos.y) };

    const std::array<glm::ivec2, 4> edges {
        pos,                        // left-top
        { pos.x + size.x, pos.y },  // right-top
        { pos.x, pos.y + size.y },  // left-bottom,
        pos + size                  // right-bottom
    };

    constexpr std::array edgeOffsets {
        std::array{ glm::ivec2{ -1, 0 }, glm::ivec2{ 0, -1 } },   // left-top
        std::array{ glm::ivec2{  1, 0 }, glm::ivec2{ 0, -1 } },   // right-top
        std::array{ glm::ivec2{ -1, 0 }, glm::ivec2{ 0,  1 } },   // left-bottom,
        std::array{ glm::ivec2{  1, 0 }, glm::ivec2{ 0,  1 } }    // right-bottom
    };

    // check edges for front color
    std::vector<checkPix> checkPixels;
    for (auto i=0; i<edges.size(); ++i) {
        checkPixels.emplace_back(checkPix{edges[i], col});
        for (auto j=0; j<2; ++j) {
            if (auto p = edges[i] + edgeOffsets[i][j]; p.x > 0 && p.y > 0 && p.x < static_cast<float>(win->getWidthReal()) && p.y < static_cast<float>(win->getHeightReal())) {
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

static ara::Button& setupTestButton(const ara::UIApplication& app, ara::Property<bool>* prop=nullptr) {
    const auto rootNode = app.getMainWindow()->getRootNode();
    auto& button = rootNode->push<ara::Button>(ara::UINodePars{
        .size = glm::ivec2{200, 100},
        .fgColor = glm::vec4{0.f, 0.f, 1.f, 1.f},
        .bgColor = glm::vec4{0.2f, 0.2f, 0.2f, 1.f},
        .borderWidth = 1,
        .borderRadius = 25,
        .borderColor = glm::vec4{1.f, 0.f, 0.f, 1.f},
        .padding = glm::vec4{20.f, 0.f, 0.f, 0.f}
    });

    button.setFontSize(30);
    button.setText("HelloBut");
    button.setTextAlign(ara::align::center, ara::valign::center);
    button.setBackgroundColor(glm::vec4{0.4f, 0.4, 0.8f, 1.f}, ara::state::selected);

    if (prop) {
        button.setIsToggle(true);
        button.setProp(*prop);
    }

    return button;
}

template <typename T>
class TestNode : public ara::Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_testValue)
    TestNode() { setTypeName<TestNode>(); }
    T m_testValue{};
};

template <typename T>
class TestNode2 : public ara::Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_testVal, m_testVal2)
    TestNode2() { setTypeName<TestNode2>(); }
    T m_testVal{};
    T m_testVal2{};
};

template <typename T>
auto& addNodeEdit(const ara::UIApplication &app, T& node, const ara::arrange ar = ara::arrange::horizontal,
    const std::optional<std::unordered_map<std::string, ara::VariableEditOption<>>> alignMap = std::nullopt,
    const std::optional<std::string> style = std::nullopt) {
    auto& ne = app.getRootNode()->push<ara::NodeEdit>();
    ne.setEditAlign(ar);
    if (alignMap.has_value()) {
        ne.setOptPerKey(alignMap.value());
    }
    if (style) {
        ne.addStyleClass(style.value());
    }
    ne.setLineHeight(22);
    ne.setSpacing({10, 10});
    ne.setLabelWidth(100);
    ne.setNode(node);

    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
    return ne;
}

static void simulateMouseClick (const ara::UIApplication& app, int32_t xPos, int32_t yPos) {
    app.getMainWindow()->onMouseDownLeft(xPos, yPos, false, false, false);
    app.getMainWindow()->onMouseUpLeft();
}
