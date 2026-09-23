#include "app/system_status.h"

#include <gtest/gtest.h>

namespace {

// mode and lastSync are not wired to anything real yet (that starts in later issues), so all
// this guards for those two is that a default-constructed SystemStatus is empty/unset, not
// silently pre-filled with something that looks like real data. state now has a real starting
// value instead (see the dedicated SystemState tests below).
TEST(SystemStatus, DefaultsToUnwiredPlaceholdersExceptState) {
    const app::SystemStatus status;
    EXPECT_TRUE(status.callsign.empty());
    EXPECT_TRUE(status.nodeId.empty());
    EXPECT_TRUE(status.mode.empty());
    EXPECT_EQ(status.lastSync, net::Clock::time_point{});
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

} // namespace
