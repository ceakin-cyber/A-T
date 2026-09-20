#include "net/tle_cache.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

namespace fs = std::filesystem;
using namespace std::chrono_literals;

const char* const kTle = "ISS (ZARYA)\n"
                         "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n"
                         "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n";

class TleCacheTest : public testing::Test {
  protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               (std::string("a-t-cache-test-") +
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir_);
    }
    void TearDown() override { fs::remove_all(dir_); }

    fs::path dir_;
};

TEST_F(TleCacheTest, WriteThenReadRoundTrips) {
    const auto fetchedAt = std::chrono::time_point_cast<std::chrono::seconds>(net::Clock::now());
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kTle, fetchedAt));

    const auto cached = net::ReadTleCache(dir_, 25544);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, kTle);
    EXPECT_EQ(cached->fetchedAt, fetchedAt);
}

TEST_F(TleCacheTest, WriteCreatesMissingDirectory) {
    const fs::path nested = dir_ / "a" / "b";
    ASSERT_TRUE(net::WriteTleCache(nested, 1, kTle, net::Clock::now()));
    EXPECT_TRUE(net::ReadTleCache(nested, 1).has_value());
}

TEST_F(TleCacheTest, WriteOverwritesPreviousEntry) {
    const auto t0 = net::Clock::now();
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, "old\n", t0));
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kTle, t0 + 1h));

    const auto cached = net::ReadTleCache(dir_, 25544);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, kTle);
}

TEST_F(TleCacheTest, EntriesAreKeyedByCatalogNumber) {
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kTle, net::Clock::now()));
    EXPECT_FALSE(net::ReadTleCache(dir_, 20580).has_value());
}

TEST_F(TleCacheTest, ReadMissingFileReturnsNullopt) {
    EXPECT_FALSE(net::ReadTleCache(dir_, 25544).has_value());
}

TEST_F(TleCacheTest, ReadFileWithoutHeaderReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "tle_25544.txt") << kTle;
    EXPECT_FALSE(net::ReadTleCache(dir_, 25544).has_value());
}

TEST_F(TleCacheTest, ReadFileWithGarbledTimestampReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "tle_25544.txt") << "fetched_at=yesterday\n" << kTle;
    EXPECT_FALSE(net::ReadTleCache(dir_, 25544).has_value());
}

TEST_F(TleCacheTest, ReadFileWithHeaderOnlyReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "tle_25544.txt") << "fetched_at=1000\n";
    EXPECT_FALSE(net::ReadTleCache(dir_, 25544).has_value());
}

TEST(TleFreshness, RecentEntryIsFresh) {
    const auto now = net::Clock::now();
    EXPECT_TRUE(net::IsFresh(now - 1h - 59min, now));
    EXPECT_TRUE(net::IsFresh(now, now));
}

TEST(TleFreshness, OldEntryIsStale) {
    const auto now = net::Clock::now();
    EXPECT_FALSE(net::IsFresh(now - 2h, now));
    EXPECT_FALSE(net::IsFresh(now - 2h - 1min, now));
}

TEST(TleFreshness, FutureTimestampIsNotFresh) {
    const auto now = net::Clock::now();
    EXPECT_FALSE(net::IsFresh(now + 1min, now));
}

} // namespace
