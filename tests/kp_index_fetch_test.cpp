// Only ParseMostRecentKp is tested here, not FetchKpIndex: it makes a real network call, and
// this project does not unit test that layer directly for any of its network fetchers (compare
// net/tle_fetch.h, which has no dedicated test file either -- only the cache/source layers built
// on top of it do, with a fake fetcher standing in for the network).
#include "net/kp_index_fetch.h"

#include <gtest/gtest.h>

namespace {

TEST(ParseMostRecentKp, ReadsTheLastEntryOfSeveral) {
    const std::string json =
        R"([{"time_tag":"2026-09-16T00:00:00","Kp":3.67,"a_running":22,"station_count":8},)"
        R"({"time_tag":"2026-09-16T03:00:00","Kp":3.00,"a_running":15,"station_count":8},)"
        R"({"time_tag":"2026-09-16T06:00:00","Kp":2.33,"a_running":9,"station_count":8}])";
    const auto kp = net::ParseMostRecentKp(json);
    ASSERT_TRUE(kp.has_value());
    EXPECT_DOUBLE_EQ(*kp, 2.33);
}

TEST(ParseMostRecentKp, ReadsASingleEntryArray) {
    const std::string json = R"([{"time_tag":"2026-09-23T00:00:00","Kp":0.33}])";
    const auto kp = net::ParseMostRecentKp(json);
    ASSERT_TRUE(kp.has_value());
    EXPECT_DOUBLE_EQ(*kp, 0.33);
}

TEST(ParseMostRecentKp, ReadsAnIntegerValueWithNoDecimalPoint) {
    const std::string json = R"([{"time_tag":"2026-09-16T00:00:00","Kp":3}])";
    const auto kp = net::ParseMostRecentKp(json);
    ASSERT_TRUE(kp.has_value());
    EXPECT_DOUBLE_EQ(*kp, 3.0);
}

TEST(ParseMostRecentKp, ToleratesWhitespaceAfterTheColon) {
    const std::string json = R"([{"time_tag": "2026-09-16T00:00:00", "Kp":   1.67}])";
    const auto kp = net::ParseMostRecentKp(json);
    ASSERT_TRUE(kp.has_value());
    EXPECT_DOUBLE_EQ(*kp, 1.67);
}

TEST(ParseMostRecentKp, IgnoresAnEarlierMalformedObjectAsLongAsTheLastOneIsFine) {
    const std::string json =
        R"([{"time_tag":"2026-09-16T00:00:00","a_running":22,"station_count":8},)"
        R"({"time_tag":"2026-09-16T03:00:00","Kp":1.33}])";
    const auto kp = net::ParseMostRecentKp(json);
    ASSERT_TRUE(kp.has_value());
    EXPECT_DOUBLE_EQ(*kp, 1.33);
}

TEST(ParseMostRecentKp, EmptyArrayGivesNullopt) {
    EXPECT_EQ(net::ParseMostRecentKp("[]"), std::nullopt);
}

TEST(ParseMostRecentKp, EmptyTextGivesNullopt) {
    EXPECT_EQ(net::ParseMostRecentKp(""), std::nullopt);
}

TEST(ParseMostRecentKp, NotJsonGivesNullopt) {
    EXPECT_EQ(net::ParseMostRecentKp("<html>503 Service Unavailable</html>"), std::nullopt);
}

TEST(ParseMostRecentKp, TheLastObjectMissingAKpFieldGivesNullopt) {
    const std::string json =
        R"([{"time_tag":"2026-09-16T00:00:00","Kp":3.67},)"
        R"({"time_tag":"2026-09-16T03:00:00","a_running":15,"station_count":8}])";
    EXPECT_EQ(net::ParseMostRecentKp(json), std::nullopt);
}

// A real response captured from https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json
// (see net/kp_index_fetch.h), trimmed to its last few entries: the actual shape this parses.
TEST(ParseMostRecentKp, MatchesARealCapturedResponse) {
    const std::string json =
        R"([{"time_tag":"2026-09-22T15:00:00","Kp":0.67,"a_running":3,"station_count":8},)"
        R"({"time_tag":"2026-09-22T18:00:00","Kp":0.67,"a_running":3,"station_count":8},)"
        R"({"time_tag":"2026-09-22T21:00:00","Kp":0.67,"a_running":3,"station_count":8},)"
        R"({"time_tag":"2026-09-23T00:00:00","Kp":0.33,"a_running":2,"station_count":8}])";
    const auto kp = net::ParseMostRecentKp(json);
    ASSERT_TRUE(kp.has_value());
    EXPECT_DOUBLE_EQ(*kp, 0.33);
}

} // namespace
