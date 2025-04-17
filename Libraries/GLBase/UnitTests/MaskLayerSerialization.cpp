#include <gtest/gtest.h>
#include <GLBase.h>
#include <GeoPrimitives/Quad.h>
#include <Utils/MaskLayer.h>
#include <Utils/TypoGlyphMap.h>
#include <pugixml.hpp>
#include "Res/ResInstance.h"

using namespace std;
using namespace pugi;

namespace ara::GLBaseUnitTest::MaskLayerSerialization {

void countNodes(const pugi::xml_node& node, int32_t& count) {
    if (strlen(node.name()) == 0) {
        return;
    }
    count += 1; // Count this node

    for (auto child : node.children()) {
        countNodes(child, count); // Recursively count child nodes
    }
}


TEST(GLBaseTest, MaskLayerSerialization) {
    GLBase glbase;
    MaskLayer maskLayer(maskType::Vector, &glbase);

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

    // export once
    {
        xml_document doc;
        xml_node root = doc.append_child("MaskLayer");
        maskLayer.serializeToXml(root);
        doc.save_file("MaskLayerSerialization.xml");
    }

    for (int i = 0; i < 10; i++) {
        xml_document doc;
        doc.load_file("MaskLayerSerialization.xml");

        auto root = doc.child("MaskLayer");
        ASSERT_EQ(root.select_nodes("Polygon").size(), 1);
        ASSERT_EQ(root.select_nodes("maskType").size(), 1);
        ASSERT_EQ(root.select_nodes("Bool").size(), 4);
        ASSERT_EQ(root.select_nodes("String").size(), 4);
        ASSERT_EQ(root.select_nodes("vec4").size(), 3);
        ASSERT_EQ(root.select_nodes("mat4").size(), 1);

        int n = 0;
        countNodes(root, n);
        ASSERT_EQ(n, 55);

        // parse
        MaskLayer parseMaskLayer(maskType::Vector, &glbase);
        parseMaskLayer.parseFromXml(root);

        // export
        xml_document expDoc;
        xml_node expRoot = expDoc.append_child("MaskLayer");
        parseMaskLayer.serializeToXml(expRoot);
        expDoc.save_file("MaskLayerSerialization.xml");
    }
}

}

