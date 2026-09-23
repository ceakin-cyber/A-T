#include "net/kp_index_source.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

namespace fs = std::filesystem;
using namespace std::chrono_literals;

constexpr double kOldKp = 1.0;
constexpr double kNewKp = 9.0;

class KpIndexSourceTest : public testing::Test {
  protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               (std::string("a-t-kp-source-test-") +
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir_);
        now_ = std::chrono::time_point_cast<std::chrono::seconds>(net::Clock::now());
    }
    void TearDown() override { fs::remove_all(dir_); }

    net::KpFetcher Succeeding() {
        return [this] {
            ++fetchCalls_;
            return std::optional<double>(kNewKp);
        };
    }
    net::KpFetcher Failing() {
        return [this] {
            ++fetchCalls_;
            return std::optional<double>();
        };
    }

    fs::path dir_;
    net::Clock::time_point now_;
    int fetchCalls_ = 0;
};

TEST_F(KpIndexSourceTest, FreshCacheIsUsedWithoutFetching) {
    ASSERT_TRUE(net::WriteKpCache(dir_, kOldKp, now_ - 30min));

    const auto kp = net::LoadKpIndex(dir_, now_, Succeeding());
    ASSERT_TRUE(kp.has_value());
    EXPECT_EQ(kp->source, net::KpSource::FreshCache);
    EXPECT_DOUBLE_EQ(kp->kp, kOldKp);
    EXPECT_EQ(kp->fetchedAt, now_ - 30min);
    EXPECT_EQ(fetchCalls_, 0);
}

TEST_F(KpIndexSourceTest, NoCacheFetchesAndCaches) {
    const auto kp = net::LoadKpIndex(dir_, now_, Succeeding());
    ASSERT_TRUE(kp.has_value());
    EXPECT_EQ(kp->source, net::KpSource::Network);
    EXPECT_DOUBLE_EQ(kp->kp, kNewKp);
    EXPECT_EQ(kp->fetchedAt, now_);
    EXPECT_EQ(fetchCalls_, 1);

    const auto cached = net::ReadKpCache(dir_);
    ASSERT_TRUE(cached.has_value());
    EXPECT_DOUBLE_EQ(cached->kp, kNewKp);
}

TEST_F(KpIndexSourceTest, StaleCacheIsReplacedByASuccessfulFetch) {
    // kKpMaxAge is 1 hour; 2 hours old is well past it.
    ASSERT_TRUE(net::WriteKpCache(dir_, kOldKp, now_ - 2h));

    const auto kp = net::LoadKpIndex(dir_, now_, Succeeding());
    ASSERT_TRUE(kp.has_value());
    EXPECT_EQ(kp->source, net::KpSource::Network);
    EXPECT_DOUBLE_EQ(kp->kp, kNewKp);
    EXPECT_DOUBLE_EQ(net::ReadKpCache(dir_)->kp, kNewKp);
}

TEST_F(KpIndexSourceTest, FailedFetchFallsBackToStaleCache) {
    ASSERT_TRUE(net::WriteKpCache(dir_, kOldKp, now_ - 2h));

    const auto kp = net::LoadKpIndex(dir_, now_, Failing());
    ASSERT_TRUE(kp.has_value());
    EXPECT_EQ(kp->source, net::KpSource::StaleCache);
    EXPECT_DOUBLE_EQ(kp->kp, kOldKp);
    EXPECT_EQ(kp->fetchedAt, now_ - 2h);
    EXPECT_EQ(fetchCalls_, 1);
}

TEST_F(KpIndexSourceTest, FailedFetchDoesNotTouchTheCache) {
    ASSERT_TRUE(net::WriteKpCache(dir_, kOldKp, now_ - 2h));
    net::LoadKpIndex(dir_, now_, Failing());

    const auto cached = net::ReadKpCache(dir_);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->fetchedAt, now_ - 2h);
}

TEST_F(KpIndexSourceTest, FailedFetchWithNoCacheReturnsNullopt) {
    EXPECT_FALSE(net::LoadKpIndex(dir_, now_, Failing()).has_value());
}

TEST_F(KpIndexSourceTest, FailedFetchWithCorruptCacheReturnsNullopt) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "kp_index.txt") << "not a cache file\n";
    EXPECT_FALSE(net::LoadKpIndex(dir_, now_, Failing()).has_value());
}

TEST_F(KpIndexSourceTest, CorruptCacheIsReplacedByASuccessfulFetch) {
    fs::create_directories(dir_);
    std::ofstream(dir_ / "kp_index.txt") << "not a cache file\n";

    const auto kp = net::LoadKpIndex(dir_, now_, Succeeding());
    ASSERT_TRUE(kp.has_value());
    EXPECT_EQ(kp->source, net::KpSource::Network);
}

TEST_F(KpIndexSourceTest, StillWorksWhenTheCacheCannotBeWritten) {
    // A regular file where the cache directory should be makes every write fail.
    fs::create_directories(dir_.parent_path());
    std::ofstream(dir_) << "blocker";

    const auto kp = net::LoadKpIndex(dir_, now_, Succeeding());
    ASSERT_TRUE(kp.has_value());
    EXPECT_EQ(kp->source, net::KpSource::Network);
    EXPECT_DOUBLE_EQ(kp->kp, kNewKp);
}

} // namespace
