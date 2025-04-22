#include "GLBaseUnitTestCommon.h"
#include <GeoPrimitives/Polygon.h>
#include <hps/hps.h>

using namespace std;

namespace ara::GLBaseUnitTest::PolygonSerializationHps {
TEST(GLBaseTest, PolygonSerializationHps) {
    std::ofstream out_file("data.log", std::ofstream::binary);
    std::ifstream in_file("data.log", std::ifstream::binary);

    ShaderCollector shCol;
    Polygon poly(&shCol);
    auto p = poly.addPoint(0, -0.9f, 0.9f, 0.f, 1.f);
    p->texBasePoint = true;

    p = poly.addPoint(0, -0.9f, -0.9f, 0.f, 0.f);
    p->texBasePoint = true;

    p = poly.addPoint(0, 0.9f, -0.9f, 1.f, 0.f);
    p->texBasePoint = true;

    p = poly.addPoint(0, 0.9f, 0.9f, 1.f, 1.f);
    p->texBasePoint = true;


    hps::to_stream(poly, out_file);
    out_file.close();


    Polygon parsed(&shCol);
    hps::from_stream(in_file, parsed);
    //parsed.parse(buf);
    //auto parsed = hps::from_stream<Polygon>(in_file);

    ASSERT_EQ(poly.getPoints(0)->size(), parsed.getPoints(0)->size());
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(poly.getPoint(0, i)->position.x, parsed.getPoint(0, i)->position.x);
        ASSERT_EQ(poly.getPoint(0, i)->position.y, parsed.getPoint(0, i)->position.y);
        ASSERT_EQ(poly.getPoint(0, i)->refTexCoord.x, parsed.getPoint(0, i)->refTexCoord.x);
        ASSERT_EQ(poly.getPoint(0, i)->refTexCoord.y, parsed.getPoint(0, i)->refTexCoord.y);
    }

}
}

