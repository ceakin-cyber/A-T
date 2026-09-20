#include "net/tle_source.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

namespace fs = std::filesystem;
using namespace std::chrono_literals;

const std::string kOldTle = "OLD\n1 ...\n2 ...\n";
const std::string kNewTle = "NEW\n1 ...\n2 ...\n";

class TleSourceTest : public testing::Test {
  protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               (std::string("a-t-source-test-") +
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir_);
        now_ = std::chrono::time_point_cast<std::chrono::seconds>(net::Clock::now());
    }
    void TearDown() override { fs::remove_all(dir_); }

    net::TleFetcher Succeeding() {
        return [this](int) {
            ++fetchCalls_;
            return std::optional<std::string>(kNewTle);
        };
    }
    net::TleFetcher Failing() {
        return [this](int) {
            ++fetchCalls_;
            return std::optional<std::string>();
        };
    }

    fs::path dir_;
    net::Clock::time_point now_;
    int fetchCalls_ = 0;
};

TEST_F(TleSourceTest, FreshCacheIsUsedWithoutFetching) {
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kOldTle, now_ - 30min));

    const auto tle = net::LoadTle(dir_, 25544, now_, Succeeding());
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->source, net::TleSource::FreshCache);
    EXPECT_EQ(tle->text, kOldTle);
    EXPECT_EQ(tle->fetchedAt, now_ - 30min);
    EXPECT_EQ(fetchCalls_, 0);
}

TEST_F(TleSourceTest, NoCacheFetchesAndCaches) {
    const auto tle = net::LoadTle(dir_, 25544, now_, Succeeding());
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->source, net::TleSource::Network);
    EXPECT_EQ(tle->text, kNewTle);
    EXPECT_EQ(tle->fetchedAt, now_);
    EXPECT_EQ(fetchCalls_, 1);

    const auto cached = net::ReadTleCache(dir_, 25544);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, kNewTle);
}

TEST_F(TleSourceTest, StaleCacheIsReplacedByASuccessfulFetch) {
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kOldTle, now_ - 3h));

    const auto tle = net::LoadTle(dir_, 25544, now_, Succeeding());
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->source, net::TleSource::Network);
    EXPECT_EQ(tle->text, kNewTle);
    EXPECT_EQ(net::ReadTleCache(dir_, 25544)->text, kNewTle);
}

TEST_F(TleSourceTest, FailedFetchFallsBackToStaleCache) {
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kOldTle, now_ - 3h));

    const auto tle = net::LoadTle(dir_, 25544, now_, Failing());
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->source, net::TleSource::StaleCache);
    EXPECT_EQ(tle->text, kOldTle);
    EXPECT_EQ(tle->fetchedAt, now_ - 3h);
    EXPECT_EQ(fetchCalls_, 1);
}

TEST_F(TleSourceTest, FailedFetchDoesNotTouchTheCache) {
    ASSERT_TRUE(net::WriteTleCache(dir_, 25544, kOldTle, now_ - 3h));
    net::LoadTle(dir_, 25544, now_, Failing());

    const auto cached = net::ReadTleCache(dir_, 25544);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->fetchedAt, now_ - 3h);
}

TEST_F(TleSourceTest, FailedFetchWithNoCacheReturnsNullopt) {
    EXPECT_FALSE(net::LoadTle(dir_, 25544, now_, Failing()).has_value());
}

TEST_F(TleSourceTest, FailedFetchWithCorruptCacheReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "tle_25544.txt") << "not a cache file\n";
    EXPECT_FALSE(net::LoadTle(dir_, 25544, now_, Failing()).has_value());
}

TEST_F(TleSourceTest, CorruptCacheIsReplacedByASuccessfulFetch) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "tle_25544.txt") << "not a cache file\n";

    const auto tle = net::LoadTle(dir_, 25544, now_, Succeeding());
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->source, net::TleSource::Network);
}

TEST_F(TleSourceTest, StillWorksWhenTheCacheCannotBeWritten) {
    // A regular file where the cache directory should be makes every write fail.
    fs::create_directories(dir_.parent_path());
    std::ofstream(dir_) << "blocker";

    const auto tle = net::LoadTle(dir_, 25544, now_, Succeeding());
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->source, net::TleSource::Network);
    EXPECT_EQ(tle->text, kNewTle);
}

} // namespace
