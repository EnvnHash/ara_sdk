#include <utility_unit_test_common.h>
#include <numeric>
#include "RwBinFile.h"

namespace fs = std::filesystem;

class RwBinFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for the test files
        temp_dir_ = fs::temp_directory_path() / "RwBinFileTest";
        if (!fs::exists(temp_dir_)) {
            fs::create_directories(temp_dir_);
        }
    }

    void TearDown() override {
        // Remove the temporary directory and all its contents
        fs::remove_all(temp_dir_);
    }

    fs::path GetTempFilePath(const std::string& filename) const {
        return temp_dir_ / filename;
    }

private:
    fs::path temp_dir_;
};

TEST_F(RwBinFileTest, ReadNonExistingFile) {
    fs::path filepath = GetTempFilePath("non_existing_file.bin");
    std::vector<uint8_t> data;

    EXPECT_EQ(ara::ReadBinFile(data, filepath), 0);
    EXPECT_TRUE(data.empty());
}

TEST_F(RwBinFileTest, ReadEmptyFile) {
    fs::path filepath = GetTempFilePath("empty_file.bin");
    std::ofstream empty_file(filepath, std::ios::out | std::ios::binary);

    ASSERT_TRUE(empty_file.is_open());

    std::vector<uint8_t> data;
    EXPECT_EQ(ara::ReadBinFile(data, filepath), 0);
    EXPECT_TRUE(data.empty());
}

TEST_F(RwBinFileTest, WriteAndReadSmallData) {
    fs::path filepath = GetTempFilePath("small_data.bin");
    std::vector<uint8_t> write_data = {1, 2, 3, 4, 5};

    // Write data to the file
    EXPECT_EQ(ara::WriteBinFile(write_data, filepath), write_data.size());

    // Read data from the file
    std::vector<uint8_t> read_data;
    EXPECT_EQ(ara::ReadBinFile(read_data, filepath), write_data.size());
    EXPECT_EQ(read_data, write_data);
}

TEST_F(RwBinFileTest, WriteAndReadLargeData) {
    fs::path filepath = GetTempFilePath("large_data.bin");
    std::vector<uint8_t> write_data(1024 * 1024); // 1MB of data
    std::iota(write_data.begin(), write_data.end(), 0);

    // Write data to the file
    EXPECT_EQ(ara::WriteBinFile(write_data, filepath), write_data.size());

    // Read data from the file
    std::vector<uint8_t> read_data;
    EXPECT_EQ(ara::ReadBinFile(read_data, filepath), write_data.size());
    EXPECT_EQ(read_data, write_data);
}

TEST_F(RwBinFileTest, WriteBuffer) {
    fs::path filepath = GetTempFilePath("null_buffer.bin");
    std::size_t size = 1024;
    std::vector<uint8_t> buf(size);

    EXPECT_EQ(ara::WriteBinFile(buf.data(), size, filepath), size);
}

TEST_F(RwBinFileTest, WriteZeroSize) {
    fs::path filepath = GetTempFilePath("zero_size.bin");
    std::vector<uint8_t> data;

    // Write zero size to the file
    EXPECT_EQ(ara::WriteBinFile(data, filepath), 0);
}
