#include "app/transmission_events.h"

#include <gtest/gtest.h>
#include <optional>

namespace {

using Clock = net::Clock;

Clock::time_point At(int secondsSinceEpoch) {
    return Clock::time_point(std::chrono::seconds(secondsSinceEpoch));
}

core::Pass MakePass(double riseJd) {
    core::Pass pass;
    pass.riseJd = riseJd;
    pass.setJd = riseJd + 0.01;
    return pass;
}

TEST(PassChanged, BothNulloptIsUnchanged) {
    EXPECT_FALSE(app::PassChanged(std::nullopt, std::nullopt));
}

TEST(PassChanged, NulloptToAPassIsChanged) {
    EXPECT_TRUE(app::PassChanged(std::nullopt, MakePass(100.0)));
}

TEST(PassChanged, APassToNulloptIsChanged) {
    EXPECT_TRUE(app::PassChanged(MakePass(100.0), std::nullopt));
}

TEST(PassChanged, TheSameRiseTimeIsUnchanged) {
    EXPECT_FALSE(app::PassChanged(MakePass(100.0), MakePass(100.0)));
}

TEST(PassChanged, ADifferentRiseTimeIsChanged) {
    EXPECT_TRUE(app::PassChanged(MakePass(100.0), MakePass(100.5)));
}

TEST(PassChanged, OnlySetJdOrElevationDifferingWithTheSameRiseTimeIsUnchanged) {
    // riseJd is the identity of a pass for this purpose; the planner re-scanning and refining
    // the same pass's other fields (a tighter setJd or maxElevation) is not a new pass.
    core::Pass previous = MakePass(100.0);
    core::Pass current = MakePass(100.0);
    current.setJd = previous.setJd + 0.001;
    current.maxElevation = previous.maxElevation + 0.1;
    EXPECT_FALSE(app::PassChanged(previous, current));
}

TEST(IsIdle, NoLastEventIsIdle) {
    EXPECT_TRUE(app::IsIdle(std::nullopt, At(1000)));
}

TEST(IsIdle, JustAfterAnEventIsNotIdle) {
    EXPECT_FALSE(app::IsIdle(At(1000), At(1000)));
}

TEST(IsIdle, JustUnderTheThresholdIsNotIdle) {
    const auto justUnder = At(1000) + app::kIdleThreshold - std::chrono::seconds(1);
    EXPECT_FALSE(app::IsIdle(At(1000), justUnder));
}

TEST(IsIdle, ExactlyAtTheThresholdIsIdle) {
    EXPECT_TRUE(app::IsIdle(At(1000), At(1000) + app::kIdleThreshold));
}

TEST(IsIdle, WellPastTheThresholdIsIdle) {
    EXPECT_TRUE(app::IsIdle(At(1000), At(1000) + app::kIdleThreshold * 10));
}

} // namespace
