#include "app/star_map_time.h"

#include <gtest/gtest.h>

namespace {

using Clock = std::chrono::system_clock;

Clock::time_point At(int secondsSinceEpoch) {
    return Clock::time_point(std::chrono::seconds(secondsSinceEpoch));
}

TEST(Effective, FollowsNowByDefault) {
    const app::StarMapTime time;
    EXPECT_EQ(app::Effective(time, At(1000)), At(1000));
}

TEST(Effective, TracksNowAsItChangesWhileFollowing) {
    const app::StarMapTime time;
    EXPECT_EQ(app::Effective(time, At(1000)), At(1000));
    EXPECT_EQ(app::Effective(time, At(2000)), At(2000));
}

TEST(Effective, ReturnsSimulatedTimeOnceDetached) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(60));
    EXPECT_FALSE(time.following);
    EXPECT_EQ(app::Effective(time, At(9999)), At(1060));
}

TEST(Jump, DetachesFromNowAndAppliesThePositiveOffset) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(3600));
    EXPECT_EQ(app::Effective(time, At(1000)), At(4600));
}

TEST(Jump, AppliesANegativeOffset) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(-3600));
    EXPECT_EQ(app::Effective(time, At(1000)), At(-2600));
}

TEST(Jump, AccumulatesAcrossRepeatedCallsFromTheDetachedTime) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(60));
    app::Jump(time, At(9999), std::chrono::seconds(60)); // `now` no longer matters, already
                                                          // detached
    EXPECT_EQ(app::Effective(time, At(1000)), At(1120));
}

TEST(Jump, LeavesPlayingUnchanged) {
    app::StarMapTime time;
    time.playing = true;
    app::Jump(time, At(1000), std::chrono::seconds(60));
    EXPECT_TRUE(time.playing);
}

TEST(SetPlaying, DetachesFromNowWhenTurnedOnWhileFollowing) {
    app::StarMapTime time;
    app::SetPlaying(time, At(1000), true);
    EXPECT_FALSE(time.following);
    EXPECT_TRUE(time.playing);
    EXPECT_EQ(app::Effective(time, At(9999)), At(1000));
}

TEST(SetPlaying, TurningOffDoesNotReattachToNow) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(500));
    app::SetPlaying(time, At(1000), true);
    app::SetPlaying(time, At(1000), false);
    EXPECT_FALSE(time.following);
    EXPECT_FALSE(time.playing);
    EXPECT_EQ(app::Effective(time, At(9999)), At(1500));
}

TEST(SetPlaying, TurningOnWhileAlreadyDetachedKeepsTheExistingSimulatedTime) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(500));
    app::SetPlaying(time, At(9999), true); // `now` here must not overwrite the jump
    EXPECT_EQ(app::Effective(time, At(1000)), At(1500));
}

TEST(Resume, ReattachesToNowAndStopsPlaying) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(500));
    app::SetPlaying(time, At(1000), true);
    app::Resume(time);
    EXPECT_TRUE(time.following);
    EXPECT_FALSE(time.playing);
    EXPECT_EQ(app::Effective(time, At(2000)), At(2000));
}

TEST(Resume, LeavesSpeedUnchangedForTheNextDetach) {
    app::StarMapTime time;
    time.speed = 42.0;
    app::SetPlaying(time, At(1000), true);
    app::Resume(time);
    EXPECT_DOUBLE_EQ(time.speed, 42.0);
}

TEST(Advance, DoesNothingWhileFollowing) {
    app::StarMapTime time;
    app::Advance(time, At(1000), 10.0);
    EXPECT_TRUE(time.following);
    EXPECT_EQ(app::Effective(time, At(1000)), At(1000));
}

TEST(Advance, DoesNothingWhileDetachedButPaused) {
    app::StarMapTime time;
    app::Jump(time, At(1000), std::chrono::seconds(0));
    app::Advance(time, At(1000), 10.0);
    EXPECT_EQ(app::Effective(time, At(1000)), At(1000));
}

TEST(Advance, MovesSimulatedTimeForwardAtTheGivenSpeedWhilePlaying) {
    app::StarMapTime time;
    time.speed = 60.0; // 60 simulated seconds per real second
    app::SetPlaying(time, At(1000), true);
    app::Advance(time, At(1000), 2.0); // 2 real seconds elapse
    EXPECT_EQ(app::Effective(time, At(1000)), At(1120));
}

TEST(Advance, ANegativeSpeedRunsTheSimulatedTimeBackwards) {
    app::StarMapTime time;
    time.speed = -60.0;
    app::SetPlaying(time, At(1000), true);
    app::Advance(time, At(1000), 2.0);
    EXPECT_EQ(app::Effective(time, At(1000)), At(880));
}

TEST(Advance, AccumulatesOverMultipleCalls) {
    app::StarMapTime time;
    time.speed = 60.0;
    app::SetPlaying(time, At(1000), true);
    app::Advance(time, At(1000), 1.0);
    app::Advance(time, At(1000), 1.0);
    EXPECT_EQ(app::Effective(time, At(1000)), At(1120));
}

} // namespace
