#include <gtest/gtest.h>
#include <GLBase.h>
#include <GeoPrimitives/Polygon.h>
#include <Utils/MaskLayer.h>
#include <Utils/MaskStack.h>

using namespace std;
using namespace pugi;

namespace ara::GLBaseUnitTest::MaskStackSerialization {

    TEST(GLBaseTest, MaskStackSerialization) {
        ShaderCollector shCol;
        MaskStack maskStack;
        maskStack.setShaderCollector(&shCol);

        auto poly = maskStack.addLayer(maskType::Vector);
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
            xml_node root = doc.append_child("MaskStack");
            maskStack.serializeToXml(root);
            doc.save_file("MaskStackSerialization.xml");
        }

        for (int i = 0; i < 10; i++) {
            xml_document doc;
            doc.load_file("MaskStackSerialization.xml");

            auto root = doc.child("MaskStack");

            ASSERT_EQ(root.select_nodes("Layers").size(), 1);
            auto layers = root.child("Layers");

            ASSERT_EQ(layers.select_nodes("Layer").size(), 1);
            auto layer = layers.child("Layer");

            ASSERT_EQ(layer.select_nodes("Polygon").size(), 1);
            ASSERT_EQ(layer.select_nodes("PolygonInv").size(), 1);
            ASSERT_EQ(layer.select_nodes("maskType").size(), 1);
            ASSERT_EQ(layer.select_nodes("Bool").size(), 4); // visible, selected, inverted, copyLoadedBitmap
            ASSERT_EQ(layer.select_nodes("UInt").size(), 1); // layerIdx
            ASSERT_EQ(layer.select_nodes("String").size(), 4);
            ASSERT_EQ(layer.select_nodes("vec4").size(), 1);
            ASSERT_EQ(layer.select_nodes("mat4").size(), 1);

            int n = 0;
            countChildren(root, n);
            ASSERT_EQ(n, 55);

            // parse
            MaskStack parseMaskStack;
            parseMaskStack.setShaderCollector(&shCol);
            parseMaskStack.parseFromXml(layers);

            // export
            xml_document expDoc;
            xml_node expRoot = expDoc.append_child("MaskStack");
            parseMaskStack.serializeToXml(expRoot);
            expDoc.save_file("MaskStackSerialization.xml");
        }
    }
}

