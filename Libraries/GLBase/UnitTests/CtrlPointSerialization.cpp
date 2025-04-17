#include <gtest/gtest.h>
#include <GLBase.h>
#include <GeoPrimitives/CtrlPoint.h>

using namespace std;

namespace ara::GLBaseUnitTest::CtrlPointSerialization {
    TEST(GLBaseTest, CtrlPointSerialization) {
        /*
        std::ofstream out_file("data.log", std::ofstream::binary);

        CtrlPoint ctrlPoint(0.5f, -0.5f, 0.2f, -0.2f);
        ctrlPoint.offsetPos.x = 0.12f; ctrlPoint.offsetPos.y = 0.125f;
        ctrlPoint.refPosition.x = 0.212f; ctrlPoint.refPosition.y = 0.3125f;
        ctrlPoint.iPolTime = 0.3f;
        ctrlPoint.refGridPos[0] = 3;
        ctrlPoint.refGridPos[1] = 5;
        ctrlPoint.patchInd = 1;
        ctrlPoint.texBasePoint = true;
        ctrlPoint.selected = true;
        ctrlPoint.mDir = CtrlPoint::UI_CTRL_VERT;
        ctrlPoint.mouseIcon = MouseIcon::ibeam;

        hps::to_stream(ctrlPoint, out_file);
        out_file.close();

        std::ifstream in_file("data.log", std::ifstream::binary);
        auto parsedCp = hps::from_stream<CtrlPoint>(in_file);
        ASSERT_EQ(parsedCp.position.x, ctrlPoint.position.x);
        ASSERT_EQ(parsedCp.refTexCoord.x, ctrlPoint.refTexCoord.x);
        ASSERT_EQ(parsedCp.offsetPos.x, ctrlPoint.offsetPos.x);
        ASSERT_EQ(parsedCp.refPosition.x, ctrlPoint.refPosition.x);
        ASSERT_EQ((int)parsedCp.extrPolMethod, (int)ctrlPoint.extrPolMethod);
        ASSERT_EQ((int)parsedCp.intrPolMethod, (int)ctrlPoint.intrPolMethod);
        ASSERT_EQ(parsedCp.refGridPos[0], ctrlPoint.refGridPos[0]);
        ASSERT_EQ(parsedCp.refGridPos[1], ctrlPoint.refGridPos[1]);
        ASSERT_EQ(parsedCp.patchInd, ctrlPoint.patchInd);
        ASSERT_EQ(parsedCp.texBasePoint, ctrlPoint.texBasePoint);
        ASSERT_EQ(parsedCp.selected, ctrlPoint.selected);
        ASSERT_EQ(parsedCp.mDir, ctrlPoint.mDir);
        ASSERT_EQ(parsedCp.mouseIcon, ctrlPoint.mouseIcon);
         */
    }
}

