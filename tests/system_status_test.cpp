#include "app/system_status.h"

#include "app/format.h"

#include <chrono>
#include <gtest/gtest.h>
#include <optional>

namespace {

using Clock = net::Clock;

Clock::time_point At(int secondsSinceEpoch) {
    return Clock::time_point(std::chrono::seconds(secondsSinceEpoch));
}

// callsign and nodeId are still blank by default -- SystemStatus itself never reaches for
// kCallsign/kNodeId on its own, a caller assigns them (see AssignCleanlyIntoASystemStatus below).
// mode, state and lastSync now all have real starting values instead: mode's default-constructed
// blank is never actually shown, since SystemMode's own empty-input case returns "OFFLINE" (see
// the dedicated SystemMode tests below), not blank.
TEST(SystemStatus, CallsignAndNodeIdDefaultToBlankUntilAssigned) {
    const app::SystemStatus status;
    EXPECT_TRUE(status.callsign.empty());
    EXPECT_TRUE(status.nodeId.empty());
    EXPECT_TRUE(status.mode.empty());
    EXPECT_EQ(status.lastSync, Clock::time_point{});
    EXPECT_EQ(status.state, "INITIALIZING");
}

// kCallsign and kNodeId are hardcoded placeholders (see their own comment), not yet wired into a
// SystemStatus by any code in this repo -- these just guard the values themselves.
TEST(SystemStatusConstants, KCallsignIsTheHamRadioNoCallsignPlaceholder) {
    EXPECT_STREQ(app::kCallsign, "N0CALL");
}

TEST(SystemStatusConstants, KNodeIdIsSet) {
    EXPECT_STREQ(app::kNodeId, "NODE-01");
}

// Both constants must convert cleanly to SystemStatus's std::string fields, since that is the
// entire point of hardcoding them as placeholders for those fields.
TEST(SystemStatusConstants, AssignCleanlyIntoASystemStatus) {
    app::SystemStatus status;
    status.callsign = app::kCallsign;
    status.nodeId = app::kNodeId;
    EXPECT_EQ(status.callsign, "N0CALL");
    EXPECT_EQ(status.nodeId, "NODE-01");
}

TEST(SystemState, OnlineWhenAtLeastOneSatelliteLoaded) {
    EXPECT_EQ(app::SystemState(true), "ONLINE");
}

TEST(SystemState, OfflineWhenNoSatelliteLoaded) {
    EXPECT_EQ(app::SystemState(false), "OFFLINE");
}

TEST(SystemState, ResultAssignsCleanlyIntoASystemStatus) {
    app::SystemStatus status;
    status.state = app::SystemState(true);
    EXPECT_EQ(status.state, "ONLINE");
}

TEST(SystemMode, EmptyInputIsOffline) {
    EXPECT_EQ(app::SystemMode({}), "OFFLINE");
}

TEST(SystemMode, AllNetworkIsLive) {
    EXPECT_EQ(app::SystemMode({net::TleSource::Network, net::TleSource::Network}), "LIVE");
}

TEST(SystemMode, OneFreshCacheAmongNetworkIsCached) {
    EXPECT_EQ(app::SystemMode({net::TleSource::Network, net::TleSource::FreshCache}), "CACHED");
}

TEST(SystemMode, OneStaleCacheAmongTheRestIsLowVisibility) {
    EXPECT_EQ(app::SystemMode({net::TleSource::Network, net::TleSource::FreshCache,
                               net::TleSource::StaleCache}),
             "LOW-VISIBILITY");
}

TEST(SystemMode, StaleCacheOutranksFreshCacheRegardlessOfOrder) {
    EXPECT_EQ(app::SystemMode({net::TleSource::StaleCache, net::TleSource::FreshCache}),
             "LOW-VISIBILITY");
    EXPECT_EQ(app::SystemMode({net::TleSource::FreshCache, net::TleSource::StaleCache}),
             "LOW-VISIBILITY");
}

TEST(SystemMode, ASingleSourceMatchesFormatNodeMode) {
    EXPECT_EQ(app::SystemMode({net::TleSource::Network}), app::FormatNodeMode(net::TleSource::Network));
    EXPECT_EQ(app::SystemMode({net::TleSource::FreshCache}),
             app::FormatNodeMode(net::TleSource::FreshCache));
    EXPECT_EQ(app::SystemMode({net::TleSource::StaleCache}),
             app::FormatNodeMode(net::TleSource::StaleCache));
}

TEST(SystemMode, ResultAssignsCleanlyIntoASystemStatus) {
    app::SystemStatus status;
    status.mode = app::SystemMode({net::TleSource::Network});
    EXPECT_EQ(status.mode, "LIVE");
}

TEST(LastSync, EmptyInputGivesNullopt) {
    EXPECT_EQ(app::LastSync({}), std::nullopt);
}

TEST(LastSync, OneFetchTimeIsItself) {
    EXPECT_EQ(app::LastSync({At(1000)}), At(1000));
}

TEST(LastSync, PicksTheLatestOfSeveralInOrder) {
    EXPECT_EQ(app::LastSync({At(1000), At(3000), At(2000)}), At(3000));
}

TEST(LastSync, TheLatestCanBeFirstOrLast) {
    EXPECT_EQ(app::LastSync({At(3000), At(1000), At(2000)}), At(3000));
    EXPECT_EQ(app::LastSync({At(1000), At(2000), At(3000)}), At(3000));
}

TEST(LastSync, TiedFetchTimesStillResolveToThatTime) {
    EXPECT_EQ(app::LastSync({At(1000), At(1000)}), At(1000));
}

TEST(LastSync, ResultFoldsCleanlyIntoASystemStatusWithValueOr) {
    app::SystemStatus status;
    status.lastSync = app::LastSync({At(1000), At(2000)}).value_or(net::Clock::time_point{});
    EXPECT_EQ(status.lastSync, At(2000));

    // An empty result falls back to the struct's own "never synced" default.
    status.lastSync = app::LastSync({}).value_or(net::Clock::time_point{});
    EXPECT_EQ(status.lastSync, net::Clock::time_point{});
}

} // namespace
