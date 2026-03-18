//
// Created by sven on 18-03-26.
//

#include "GLBaseUnitTestCommon.h"
#include <Asset/AssetManager.h>

#include "Asset/AssetColor.h"

using namespace std;
using namespace glm;

namespace ara::ResourceTest::ResourceFileLoading {

class ResourceTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        writeTestResFile();
    }

    static void TearDownTestSuite() {}

    void SetUp() override {
        glBase = std::make_shared<GLBase>();
        glBase->setResFile(m_resFile);
        glBase->setResRootPath(m_resRootPath);
        glBase->setLoadMouseCursorIcons(false);
        glBase->setContinousChangeCheck(false);
        glBase->init();
        am = glBase->getAssetManager();
    }

    void TearDown() override {
        glBase->destroy(true);
        glBase.reset();
    }

    static void writeTestResFile() {
        std::string testResFile = STRINGIFY(defaults {
        width:100%
        height: 195px
        exampleText: "blabla"
        color: rgb(0,0,0)
    }

    testBlock {
        text: defaults.exampleText
        color: defaults.color
    });
        auto p = std::filesystem::path(m_resRootPath) / m_resFile;
        std::ofstream outFile(p);
        outFile << testResFile;
        outFile.close();
    }

    static void appendToResFile(const std::string& str) {
        auto p = std::filesystem::path(glBase->m_resRootPath) / glBase->m_resFile;
        std::ifstream inFile(p);
        if (!inFile.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + p.string());
        }

        std::string content((std::istreambuf_iterator<char>(inFile)),
                            std::istreambuf_iterator<char>());
        inFile.close();

        if (!content.empty() && content.back() != '\n') {
            content += '\n';  // Add newline before appending if file doesn't end with one
        }
        content += str+"\n";

        std::ofstream outFile(p);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + p.string());
        }

        outFile << content;
        outFile.close();
    }

    static inline AssetManager* am;
    static inline std::shared_ptr<GLBase> glBase;
    static inline std::string m_resFile = "test_res.txt";
    static inline std::string m_resRootPath = "test_resdata";
};

TEST_F(ResourceTest, ResourceFileBasicLoading) {
    EXPECT_EQ(glBase->init(), true);
}

TEST_F(ResourceTest, ResourceFileGetRoot) {
    EXPECT_NE(am->getRoot(), nullptr);
}

TEST_F(ResourceTest, ResourceFileGetTestBlock) {
    auto nd = am->findNode("defaults");
    EXPECT_NE(nd, nullptr);

    nd = am->findNode("testBlock");
    EXPECT_NE(nd, nullptr);
}

TEST_F(ResourceTest, ResourceFileGetRawValue) {
    const auto nd = am->findNode("testBlock");
    const auto val = nd->getValue("text");
    EXPECT_EQ(val, "defaults.exampleText");

    const auto res = am->findNode(val);
    EXPECT_NE(res, nullptr);
    EXPECT_EQ(res->getRawValue(), "blabla");
}

TEST_F(ResourceTest, ResourceFileGetColor) {
    const auto nd = am->findNode("testBlock");
    const auto col = nd->getValue("color");
    EXPECT_EQ(col, "defaults.color");

    const auto resCol = static_cast<AssetColor*>(am->findNode(col));
    EXPECT_NE(resCol, nullptr);
    EXPECT_EQ(resCol->getColorVec4().r, 0.f);
    EXPECT_EQ(resCol->getColorVec4().g, 0.f);
    EXPECT_EQ(resCol->getColorVec4().b, 0.f);
    EXPECT_EQ(resCol->getColorVec4().a, 1.f);
}

TEST_F(ResourceTest, ResourceFileGetPercent) {
    const auto nd = am->findNode("testBlock");
    const auto col = nd->getValue("color");
    EXPECT_EQ(col, "defaults.color");

    const auto resCol = static_cast<AssetColor*>(am->findNode(col));
    EXPECT_NE(resCol, nullptr);
    EXPECT_EQ(resCol->getColorVec4().r, 0.f);
    EXPECT_EQ(resCol->getColorVec4().g, 0.f);
    EXPECT_EQ(resCol->getColorVec4().b, 0.f);
    EXPECT_EQ(resCol->getColorVec4().a, 1.f);
}

TEST_F(ResourceTest, ResourceFileValueVector) {
    const auto defaults = am->findNode("defaults");

    vector<float> rgb_values;
    EXPECT_TRUE(defaults->value_v(rgb_values, "color", 3));
    EXPECT_EQ(rgb_values.size(), 3);
    EXPECT_FLOAT_EQ(rgb_values[0], 0.0f);  // R
    EXPECT_FLOAT_EQ(rgb_values[1], 0.0f);  // G
    EXPECT_FLOAT_EQ(rgb_values[2], 0.0f);  // B
}

TEST_F(ResourceTest, ResourceFileGetPath) {
    const auto nd = am->findNode("testBlock.color");
    EXPECT_EQ(nd->getPath(), "testBlock.color");
}

TEST_F(ResourceTest, ResourceFileGetParent) {
    const auto root = am->getRoot();
    const auto nd = am->findNode("testBlock");
    EXPECT_EQ(nd->getParent(), root);
}

TEST_F(ResourceTest, ResourceFileHasFlag) {
    const auto defaults = am->findNode("defaults");
    EXPECT_FALSE(defaults->hasFlag("some_flag"));
}

TEST_F(ResourceTest, ResourceFileIsInPixels) {
    const auto defaults = am->findNode("defaults");
    EXPECT_TRUE(defaults->isInPixels("height"));
}

TEST_F(ResourceTest, ResourceFileIsInPercent) {
    const auto defaults = am->findNode("defaults");
    EXPECT_TRUE(defaults->isInPercent("width"));
}

TEST_F(ResourceTest, ResourceFileValueTemplates) {
    const auto defaults = am->findNode("defaults");
    EXPECT_EQ(defaults->value<int32_t>("height", 0), 195);
    EXPECT_FLOAT_EQ(defaults->value<float>("width", 0.0f), 100.f);
}

TEST_F(ResourceTest, ResourceFileGetIntPar) {
    const auto color_node = am->findNode("defaults.color");
    EXPECT_EQ(color_node->getIntPar(0, 0), 0);  // R
    EXPECT_EQ(color_node->getIntPar(1, 0), 0);  // G
    EXPECT_EQ(color_node->getIntPar(2, 0), 0);  // B
}

TEST_F(ResourceTest, ResourceFileGetFloatPar) {
    const auto color_node = am->findNode("defaults.color");
    EXPECT_FLOAT_EQ(color_node->getFloatPar(0, 0.0f), 0.0f);  // R
    EXPECT_FLOAT_EQ(color_node->getFloatPar(1, 0.0f), 0.0f);  // G
    EXPECT_FLOAT_EQ(color_node->getFloatPar(2, 0.0f), 0.0f);  // B
}

TEST_F(ResourceTest, ResourceFileGetParCount) {
    const auto color_node = am->findNode("defaults.color");
    EXPECT_EQ(color_node->getParCount(), 3);
}

TEST_F(ResourceTest, ResourceFileGetPar) {
    const auto color_node = am->findNode("defaults.color");
    EXPECT_EQ(color_node->getPar(0), "0");
    EXPECT_EQ(color_node->getPar(1), "0");
    EXPECT_EQ(color_node->getPar(2), "0");
}

TEST_F(ResourceTest, ResourceFileHasValue) {
    const auto defaults = am->findNode("defaults");
    EXPECT_TRUE(defaults->hasValue("width"));
    EXPECT_TRUE(defaults->hasValue("height"));
    EXPECT_TRUE(defaults->hasValue("exampleText"));
    EXPECT_TRUE(defaults->hasValue("color"));
    EXPECT_FALSE(defaults->hasValue("nonexistent"));
}

TEST_F(ResourceTest, ResourceFileUpdateCheckDoNothing) {
    bool called = false;
    glBase->startContinousCheck();
    glBase->addUpdtResCb([&] {
        called = true;
    });
    this_thread::sleep_for(chrono::milliseconds(1000)); // wait for check to pass once
    EXPECT_FALSE(called); // nothing was changed updated cb should not have been called
}

TEST_F(ResourceTest, ResourceFileUpdateCheckGetChange) {
    bool called = false;
    glBase->startContinousCheck();
    glBase->addUpdtResCb([&] {
        called = true;
    });
    this_thread::sleep_for(chrono::milliseconds(100)); // wait for check to pass once

    appendToResFile("newBlock {\n}");
    this_thread::sleep_for(chrono::milliseconds(1000)); // wait for check to pass once
    EXPECT_TRUE(called);

    called = false;
    this_thread::sleep_for(chrono::milliseconds(1000)); // no more updated should occur
    EXPECT_FALSE(called);
}
}