#include "app/system_status.h"

#include <chrono>
#include <gtest/gtest.h>
#include <optional>

namespace {

using Clock = net::Clock;

Clock::time_point At(int secondsSinceEpoch) {
    return Clock::time_point(std::chrono::seconds(secondsSinceEpoch));
}

// mode is not wired to anything real yet (that starts in a later issue), so all this guards for
// it is that a default-constructed SystemStatus leaves it empty, not silently pre-filled with
// something that looks like real data. state and lastSync now have real starting values instead
// (see the dedicated SystemState and LastSync tests below); lastSync's default, epoch, is its own
// "never synced" -- the same value LastSync() folds an empty input down to for the caller.
TEST(SystemStatus, DefaultsToUnwiredPlaceholdersExceptStateAndLastSync) {
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
