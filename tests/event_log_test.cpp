#include "app/event_log.h"

#include <chrono>
#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

net::Clock::time_point At(int seconds) {
    return net::Clock::time_point(std::chrono::seconds(seconds));
}

TEST(EventLog, StartsEmpty) {
    app::EventLog log;
    EXPECT_TRUE(log.Entries().empty());
}

TEST(EventLog, AddAppendsInOrder) {
    app::EventLog log;
    log.Add(At(1), "FIRST");
    log.Add(At(2), "SECOND");
    ASSERT_EQ(log.Entries().size(), 2U);
    EXPECT_EQ(log.Entries()[0].message, "FIRST");
    EXPECT_EQ(log.Entries()[1].message, "SECOND");
    EXPECT_EQ(log.Entries()[0].time, At(1));
}

TEST(EventLog, DropsTheOldestEntryOnceOverCapacity) {
    app::EventLog log(3);
    log.Add(At(1), "A");
    log.Add(At(2), "B");
    log.Add(At(3), "C");
    log.Add(At(4), "D");
    ASSERT_EQ(log.Entries().size(), 3U);
    EXPECT_EQ(log.Entries()[0].message, "B");
    EXPECT_EQ(log.Entries()[1].message, "C");
    EXPECT_EQ(log.Entries()[2].message, "D");
}

TEST(EventLog, CapacityOfOneKeepsOnlyTheLatest) {
    app::EventLog log(1);
    log.Add(At(1), "A");
    log.Add(At(2), "B");
    ASSERT_EQ(log.Entries().size(), 1U);
    EXPECT_EQ(log.Entries()[0].message, "B");
}

TEST(LogVisibilityChange, TheFirstCallNeverLogs) {
    app::EventLog log;
    EXPECT_FALSE(log.LogVisibilityChange(At(1), true));
    EXPECT_TRUE(log.Entries().empty());
}

TEST(LogVisibilityChange, TheSecondCallLogsIfTheStateHasAlreadyChanged) {
    // Confirms the first call really does establish a baseline, and is not simply always silent.
    app::EventLog log;
    log.LogVisibilityChange(At(1), true);
    EXPECT_TRUE(log.LogVisibilityChange(At(2), false));
    ASSERT_EQ(log.Entries().size(), 1U);
    EXPECT_EQ(log.Entries()[0].message, "SIGNAL LOST");
}

TEST(LogVisibilityChange, RiseLogsSignalAcquired) {
    app::EventLog log;
    log.LogVisibilityChange(At(1), false); // establishes the starting state
    EXPECT_TRUE(log.LogVisibilityChange(At(2), true));
    ASSERT_EQ(log.Entries().size(), 1U);
    EXPECT_EQ(log.Entries()[0].message, "SIGNAL ACQUIRED");
    EXPECT_EQ(log.Entries()[0].time, At(2));
}

TEST(LogVisibilityChange, SetLogsSignalLost) {
    app::EventLog log;
    log.LogVisibilityChange(At(1), true);
    EXPECT_TRUE(log.LogVisibilityChange(At(2), false));
    ASSERT_EQ(log.Entries().size(), 1U);
    EXPECT_EQ(log.Entries()[0].message, "SIGNAL LOST");
}

TEST(LogVisibilityChange, RepeatingTheSameStateDoesNotLogAgain) {
    app::EventLog log;
    log.LogVisibilityChange(At(1), true);
    EXPECT_FALSE(log.LogVisibilityChange(At(2), true));
    EXPECT_FALSE(log.LogVisibilityChange(At(3), true));
    EXPECT_TRUE(log.Entries().empty());
}

TEST(LogVisibilityChange, TracksARiseAndSetAndAnotherRise) {
    app::EventLog log;
    log.LogVisibilityChange(At(0), false);
    log.LogVisibilityChange(At(1), true);
    log.LogVisibilityChange(At(2), true);
    log.LogVisibilityChange(At(3), false);
    log.LogVisibilityChange(At(4), true);

    ASSERT_EQ(log.Entries().size(), 3U);
    EXPECT_EQ(log.Entries()[0].message, "SIGNAL ACQUIRED");
    EXPECT_EQ(log.Entries()[0].time, At(1));
    EXPECT_EQ(log.Entries()[1].message, "SIGNAL LOST");
    EXPECT_EQ(log.Entries()[1].time, At(3));
    EXPECT_EQ(log.Entries()[2].message, "SIGNAL ACQUIRED");
    EXPECT_EQ(log.Entries()[2].time, At(4));
}

} // namespace
