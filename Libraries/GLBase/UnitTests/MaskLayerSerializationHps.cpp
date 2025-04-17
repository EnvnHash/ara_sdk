#include <gtest/gtest.h>
#include <GLBase.h>
#include <Utils/MaskLayer.h>
#include <Shaders/ShaderCollector.h>

using namespace std;

namespace ara::GLBaseUnitTest::MaskLayerSerializationHps {
    TEST(GLBaseTest, MaskLayerSerializationHps) {
        // instantiate a new opengl context
        unique_ptr<GLFWWindow> ctx = GLBase::createOpenGLCtx(true);

        // if this operation fails the unique m_ptr will be empty
        bool ptrEmpty = ctx ? false : true;

        std::ofstream out_file("data.log", std::ofstream::binary);

        ShaderCollector shCol;
        MaskLayer maskLayer(maskType::Vector, &shCol);

        auto poly = maskLayer.getPolygon();
        ASSERT_TRUE(poly);

        auto p = poly->addPoint(0, -0.9f, 0.9f, 0.f, 1.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, -0.9f, -0.9f, 0.f, 0.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, 0.9f, -0.9f, 1.f, 0.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, 0.9f, 0.9f, 1.f, 1.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, -0.9f, 0.9f, 0.f, 1.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, -0.9f, -0.9f, 0.f, 0.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, 0.9f, -0.9f, 1.f, 0.f);
        p->texBasePoint = true;

        p = poly->addPoint(0, 0.9f, 0.9f, 1.f, 1.f);
        p->texBasePoint = true;

        // -------- save ---
        hps::to_stream(maskLayer, out_file);
        out_file.close();

        // -------- load ---
        std::ifstream in_file("data.log", std::ifstream::binary);
        auto parsed = hps::from_stream<MaskLayer>(in_file);
        auto parsedPoly = parsed.getPolygon();

        ASSERT_EQ(poly->getPoints(0)->size(), parsedPoly->getPoints(0)->size());
        for (int i = 0; i < 4; i++) {
            ASSERT_EQ(poly->getPoint(0, i)->position.x, parsedPoly->getPoint(0, i)->position.x);
            ASSERT_EQ(poly->getPoint(0, i)->position.y, parsedPoly->getPoint(0, i)->position.y);
            ASSERT_EQ(poly->getPoint(0, i)->refTexCoord.x, parsedPoly->getPoint(0, i)->refTexCoord.x);
            ASSERT_EQ(poly->getPoint(0, i)->refTexCoord.y, parsedPoly->getPoint(0, i)->refTexCoord.y);
        }

        // -------- exit -    ctx->destroy();

        // expect the unique_ptr not to be empty
        EXPECT_FALSE(ptrEmpty);

    }
}

