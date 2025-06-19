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
#include <AssetLoader.h>
#include <MappedFile.h>

using namespace std;

namespace ara::UtilitiesTest::AssetLoaderTest {

static std::vector<uint8_t> write_data = {1, 2, 3, 4, 5};

static void checkBinData(const uint8_t* binData) {
    auto ptr = binData;
    auto vecPtr = write_data.begin();
    for (int i = 0; i < write_data.size(); ++i) {
        EXPECT_EQ(*ptr++, *vecPtr++);
    }
}

TEST(Utilities_UnitTests, AssetLoaderStringTest) {
    AssetLoader::setAssetPath("resdata");
    const auto sample = AssetLoader::loadAssetAsString(filesystem::path("test") / "res_sample.txt");
    EXPECT_EQ(sample.substr(0, 27), "# Sample ara Resource File\n");
    EXPECT_EQ(sample.substr(28, 12), "demo_style {");
}

TEST(Utilities_UnitTests, AssetLoaderBinaryTest) {
    AssetLoader::setAssetPath("resdata");
    vector<uint8_t> read_data;
    auto sample = AssetLoader::loadAssetToMem(read_data, filesystem::path("test") / "small_data.bin");
    EXPECT_EQ(read_data.size(), 5);
    EXPECT_EQ(read_data, write_data);
}

TEST(Utilities_UnitTests, MappedFileTest) {
    MappedFile mf((filesystem::path("resdata") / "test" / "small_data.bin").string());
    EXPECT_EQ(mf.size(), 5);
    checkBinData(mf());
    mf.unmap();
}

TEST(Utilities_UnitTests, AssetLoaderMappedBinaryTest) {
    auto memPtr = AssetLoader::mapAssetToMem(filesystem::path("test") / "small_data.bin");
    EXPECT_EQ(std::get<size_t>(memPtr), 5);
    checkBinData(std::get<const uint8_t*>(memPtr));
    AssetLoader::unMapAsset(memPtr);
}

}
