//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <UtilityUnitTestCommon.h>
#include "Table.h"

using namespace ara;

TEST(TableRCTest, AddElements) {
    TableRC rc;
    EXPECT_TRUE(rc.add(3));
    EXPECT_EQ(rc.getCount(), 3);
}

TEST(TableRCTest, AddPixElements) {
    TableRC rc;
    EXPECT_TRUE(rc.addPix(2, 10));
    EXPECT_EQ(rc.getCount(), 2);
    for (const auto& item : rc.V()) {
        EXPECT_EQ(item.type, dTableType::Pix);
        EXPECT_FLOAT_EQ(item.value, 10.0f);
    }
}

TEST(TableRCTest, AddPercentElements) {
    TableRC rc;
    EXPECT_TRUE(rc.addPercent(2, 50));
    EXPECT_EQ(rc.getCount(), 2);
    for (const auto& item : rc.V()) {
        EXPECT_EQ(item.type, dTableType::Percent);
        EXPECT_FLOAT_EQ(item.value, 50.0f);
    }
}

TEST(TableRC, InsertElements) {
    TableRC tableRC;
    eTable_rc tb{10.0f, dTableType::Pix};
    ASSERT_TRUE(tableRC.ins(-1, 1, tb));
    ASSERT_EQ(tableRC.getCount(), 1);
    EXPECT_FLOAT_EQ(tableRC(0).value, 10.0f);
}

TEST(TableRC, SetElements) {
    TableRC tableRC;
    eTable_rc tb{20.0f, dTableType::Pix};
    tableRC.ins(-1, 1, tb);
    tb.value = 30.0f;
    EXPECT_TRUE(tableRC.set(0, tb));
    EXPECT_FLOAT_EQ(tableRC(0).value, 30.0f);
}

TEST(TableRC, GetElements) {
    TableRC tableRC;
    eTable_rc tb{40.0f, dTableType::Pix};
    tableRC.ins(-1, 1, tb);
    EXPECT_FLOAT_EQ(tableRC.get(0).value, 40.0f);
}

TEST(TableRC, UpdateGeo) {
    TableRC tableRC;
    tableRC.addPix(2, 50);
    tableRC.addPercent(1, 20);

    EXPECT_TRUE(tableRC.updateGeo(200, 10, 10, 10));
    EXPECT_FLOAT_EQ(tableRC.get(0).size, 50);
    EXPECT_FLOAT_EQ(tableRC.get(1).size, 50);
}

TEST(TableRC, InsertAt) {
    TableRC tableRC;
    tableRC.addPix(1, 20);

    EXPECT_TRUE(tableRC.insPix(0, 1, 10));
    EXPECT_FLOAT_EQ(tableRC.get(0).value, 20);
    EXPECT_FLOAT_EQ(tableRC.get(1).value, 10);
}

TEST(TableRC, Add) {
    ara::TableRC table_rc;
    EXPECT_TRUE(table_rc.add(5)); // Adding 5 default entries
    EXPECT_EQ(table_rc.getCount(), 5);
}

TEST(TableRC, AddPix) {
    ara::TableRC table_rc;
    EXPECT_TRUE(table_rc.addPix(3, 100)); // Adding 3 entries with pixel value of 100
    EXPECT_EQ(table_rc.getCount(), 3);
    for (int i = 0; i < 3; ++i) {
        auto item = table_rc.get(i);
        EXPECT_EQ(item.type, ara::dTableType::Pix);
        EXPECT_EQ(item.value, 100.0f);
    }
}

TEST(TableRCTest, AddPercentTest) {
    ara::TableRC table_rc;
    EXPECT_TRUE(table_rc.addPercent(2, 50)); // Adding 2 entries with percent value of 50
    EXPECT_EQ(table_rc.getCount(), 2);
    for (int i = 0; i < 2; ++i) {
        auto rc = table_rc.get(i);
        EXPECT_EQ(rc.type, ara::dTableType::Percent);
        EXPECT_FLOAT_EQ(rc.value, 50.0f);
    }
}

TEST(TableRCTest, InsertionTest) {
    ara::TableRC table_rc;
    EXPECT_TRUE(table_rc.add(1));
    EXPECT_EQ(table_rc.getCount(), 1);

    EXPECT_TRUE(table_rc.ins(1, 2));
    EXPECT_EQ(table_rc.getCount(), 3);

    EXPECT_TRUE(table_rc.insPix(0, 1, 45));
    EXPECT_EQ(table_rc.getCount(), 4);
}

TEST(TableRCTest, DeletionTest) {
    ara::TableRC table_rc;
    EXPECT_TRUE(table_rc.add(5));
    EXPECT_EQ(table_rc.getCount(), 5);

    EXPECT_TRUE(table_rc.del(2, 3));
    EXPECT_EQ(table_rc.getCount(), 2);

    EXPECT_TRUE(table_rc.del(0, -1)); // delete all
    EXPECT_EQ(table_rc.getCount(), 0);
}

TEST(TableRCTest, SetOperations) {
    ara::TableRC table_rc;
    EXPECT_TRUE(table_rc.add(1));
    eTable_rc rc{5.0f, dTableType::Pix};
    EXPECT_TRUE(table_rc.set(0, rc));

    rc.type = dTableType::Percent;
    EXPECT_TRUE(table_rc.setPercent(0, 50.0f));
}

TEST(TableRCTest, GeoOperations) {
    ara::TableRC table_rc;
    table_rc.addPix(2, 10);
    table_rc.addPercent(1, 50.0f);

    table_rc.updateGeo(30.0f, 0.0f, 0.0f, 0.0f);
}

TEST(TableTest, UpdateGeo) {
    ara::Table table;
    table.updateGeo(100.0f, 100.0f, 10.0f, 10.0f, 10.0f, 10.0f, 5.0f, 5.0f);
}
