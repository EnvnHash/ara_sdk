#include "GLBaseUnitTestCommon.h"
#include <GeoPrimitives/Polygon.h>

using namespace std;
using namespace pugi;

namespace ara::GLBaseUnitTest::PolygonSerialization {

void countNodes(const pugi::xml_node& node, int32_t& count) {
    if (std::string(node.name()).empty()) {
        return;
    }
    count += 1; // Count this node

    for (auto child : node.children()) {
        countNodes(child, count); // Recursively count child nodes
    }
}

TEST(GLBaseTest, PolygonSerialization) {

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

    // export once
    {
        xml_document doc;
        xml_node root = doc.append_child("Polygon");
        poly.serializeToXml(root);
        doc.save_file("PolygonSerialization.xml");
    }

    for (int i = 0; i < 10; i++) {
        xml_document doc;
        doc.load_file("PolygonSerialization.xml");

        auto root = doc.child("Polygon");
        ASSERT_EQ(root.select_nodes("Shape").size(), 1);

        auto shape = root.child("Shape");
        ASSERT_EQ(shape.select_nodes("CtrlPoint").size(), 4);

        int n = 0;
        countNodes(root, n);
        ASSERT_EQ(n, 38);

        // parse
        Polygon parsePoly(&shCol);
        parsePoly.parseFromXml(root);

        // export
        xml_document expDoc;
        xml_node expRoot = expDoc.append_child("Polygon");
        parsePoly.serializeToXml(expRoot);
        expDoc.save_file("PolygonSerialization.xml");
    }
}
}

