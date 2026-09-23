#include "app/system_status.h"

#include <gtest/gtest.h>

namespace {

// Nothing wires real values into SystemStatus yet (that starts in the next issue), so all this
// guards for now is that a default-constructed one is empty/unset across the board, not
// silently pre-filled with something that looks like real data.
TEST(SystemStatus, DefaultsToEmptyPlaceholders) {
    const app::SystemStatus status;
    EXPECT_TRUE(status.callsign.empty());
    EXPECT_TRUE(status.nodeId.empty());
    EXPECT_TRUE(status.mode.empty());
    EXPECT_TRUE(status.state.empty());
    EXPECT_EQ(status.lastSync, net::Clock::time_point{});
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

} // namespace
