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

} // namespace
