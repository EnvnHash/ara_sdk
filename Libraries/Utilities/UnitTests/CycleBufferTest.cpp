#include <utility_unit_test_common.h>
#include "CycleBuffer.h"

namespace ara {

class CycleBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        cb.allocateBuffers(3, m_bufSize);
    }

    void TearDown() override {
        cb.clear();
    }

    CycleBuffer<int> cb;
    int m_bufSize = 10;
};

TEST_F(CycleBufferTest, AllocateBuffers) {
    EXPECT_EQ(cb.size(), 3);
    for (size_t i = 0; i < cb.size(); ++i) {
        EXPECT_EQ(cb.getBuff(i)->size(), 10);
    }
}

TEST_F(CycleBufferTest, FeedData) {
    std::vector<int> data(10, 42);
    EXPECT_EQ(cb.feed(data.data()), 1);
    EXPECT_EQ(cb.getLastBuff()->getData()[0], 42);

    // Test feeding again
    EXPECT_EQ(cb.feed(data.data()), 2);
    EXPECT_EQ(cb.getLastBuff()->getData()[0], 42);
}

TEST_F(CycleBufferTest, ConsumeData) {
    std::vector<int> data(10, 42);
    cb.feed(data.data());
    auto buffer = cb.consume();
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getData()[0], 42);

    // Test consuming again
    cb.feed(data.data());
    buffer = cb.consume();
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getData()[0], 42);
}

TEST_F(CycleBufferTest, ConsumeEmptyBuffer) {
    cb.clear();
    auto buffer = cb.consume();
    EXPECT_EQ(buffer, nullptr);
}

TEST_F(CycleBufferTest, IsFilled) {
    std::vector<int> data(10, 42);
    for (size_t i = 0; i < 3; ++i) {
        cb.feed(data.data());
    }
    EXPECT_TRUE(cb.isFilled());

    // Test after consuming one buffer
    auto buffer = cb.consume();
    ASSERT_NE(buffer, nullptr);
    EXPECT_FALSE(cb.isFilled());
}

TEST_F(CycleBufferTest, GetFreeSpace) {
    std::vector<int> data(10, 42);
    for (size_t i = 0; i < 3; ++i) {
        cb.feed(data.data());
        EXPECT_EQ(cb.getFreeSpace(), 2 - i);
    }
}

TEST_F(CycleBufferTest, ClearBuffer) {
    std::vector<int> data(10, 42);
    cb.feed(data.data());
    cb.clear();
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(cb.getWritePos(), 0);
    EXPECT_EQ(cb.getFillAmt(), 0);
}

TEST_F(CycleBufferTest, BufferOverflow) {
    std::vector<int> data(10, 42);
    for (size_t i = 0; i < 3; ++i) {
        cb.feed(data.data());
    }
    EXPECT_TRUE(cb.isFilled());

    // Feed again should overwrite the oldest buffer
    cb.feed(data.data());
    auto buffer = cb.getBuff(0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getData()[0], 42);

    // Consume all buffers
    for (size_t i = 0; i < 3; ++i) {
        buffer = cb.consume();
        ASSERT_NE(buffer, nullptr);
        EXPECT_EQ(buffer->getData()[0], 42);
    }
}

}  // namespace ara
