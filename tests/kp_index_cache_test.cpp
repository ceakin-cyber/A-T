#include "net/kp_index_cache.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

namespace fs = std::filesystem;
using namespace std::chrono_literals;

class KpIndexCacheTest : public testing::Test {
  protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               (std::string("a-t-kp-cache-test-") +
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir_);
    }
    void TearDown() override { fs::remove_all(dir_); }

    fs::path dir_;
};

TEST_F(KpIndexCacheTest, WriteThenReadRoundTrips) {
    const auto fetchedAt = std::chrono::time_point_cast<std::chrono::seconds>(net::Clock::now());
    ASSERT_TRUE(net::WriteKpCache(dir_, 3.67, fetchedAt));

    const auto cached = net::ReadKpCache(dir_);
    ASSERT_TRUE(cached.has_value());
    EXPECT_DOUBLE_EQ(cached->kp, 3.67);
    EXPECT_EQ(cached->fetchedAt, fetchedAt);
}

TEST_F(KpIndexCacheTest, WriteCreatesMissingDirectory) {
    const fs::path nested = dir_ / "a" / "b";
    ASSERT_TRUE(net::WriteKpCache(nested, 1.0, net::Clock::now()));
    EXPECT_TRUE(net::ReadKpCache(nested).has_value());
}

TEST_F(KpIndexCacheTest, WriteOverwritesPreviousEntry) {
    const auto t0 = net::Clock::now();
    ASSERT_TRUE(net::WriteKpCache(dir_, 1.0, t0));
    ASSERT_TRUE(net::WriteKpCache(dir_, 9.0, t0 + 1h));

    const auto cached = net::ReadKpCache(dir_);
    ASSERT_TRUE(cached.has_value());
    EXPECT_DOUBLE_EQ(cached->kp, 9.0);
}

TEST_F(KpIndexCacheTest, ReadMissingFileReturnsNullopt) {
    EXPECT_FALSE(net::ReadKpCache(dir_).has_value());
}

TEST_F(KpIndexCacheTest, ReadFileWithoutHeaderReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "kp_index.txt") << "3.67\n";
    EXPECT_FALSE(net::ReadKpCache(dir_).has_value());
}

TEST_F(KpIndexCacheTest, ReadFileWithGarbledTimestampReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "kp_index.txt") << "fetched_at=yesterday\n3.67\n";
    EXPECT_FALSE(net::ReadKpCache(dir_).has_value());
}

TEST_F(KpIndexCacheTest, ReadFileWithHeaderOnlyReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "kp_index.txt") << "fetched_at=1000\n";
    EXPECT_FALSE(net::ReadKpCache(dir_).has_value());
}

TEST_F(KpIndexCacheTest, ReadFileWithGarbledValueReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "kp_index.txt") << "fetched_at=1000\nnot-a-number\n";
    EXPECT_FALSE(net::ReadKpCache(dir_).has_value());
}

// net::IsFresh itself is generic and already thoroughly tested (see TleFreshness in
// tests/tle_cache_test.cpp); these just confirm kKpMaxAge (1 hour, much shorter than TLE's own
// default) is what actually gets used for a Kp reading.
TEST(KpFreshness, RecentEntryIsFresh) {
    const auto now = net::Clock::now();
    EXPECT_TRUE(net::IsFresh(now - 59min, now, net::kKpMaxAge));
    EXPECT_TRUE(net::IsFresh(now, now, net::kKpMaxAge));
}

TEST(KpFreshness, OldEntryIsStale) {
    const auto now = net::Clock::now();
    EXPECT_FALSE(net::IsFresh(now - 1h, now, net::kKpMaxAge));
    EXPECT_FALSE(net::IsFresh(now - 1h - 1min, now, net::kKpMaxAge));
}

} // namespace
