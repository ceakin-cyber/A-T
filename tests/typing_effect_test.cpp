#include "app/typing_effect.h"

#include <gtest/gtest.h>
#include <utility>

namespace {

using Seconds = std::chrono::duration<double>;

TEST(TypingEffect, ZeroElapsedShowsNothing) {
    EXPECT_EQ(app::TypingEffect("HELLO", Seconds(0.0), 10.0), "");
}

TEST(TypingEffect, NegativeElapsedShowsNothing) {
    EXPECT_EQ(app::TypingEffect("HELLO", Seconds(-1.0), 10.0), "");
}

TEST(TypingEffect, PartialElapsedShowsAMatchingPrefix) {
    // 10 chars/sec for 0.3s = 3 characters.
    EXPECT_EQ(app::TypingEffect("HELLO WORLD", Seconds(0.3), 10.0), "HEL");
}

TEST(TypingEffect, ElapsedExactlyLongEnoughShowsTheWholeText) {
    // 10 chars/sec for 0.5s = 5 characters, the whole word.
    EXPECT_EQ(app::TypingEffect("HELLO", Seconds(0.5), 10.0), "HELLO");
}

TEST(TypingEffect, ElapsedWellPastTheEndShowsTheWholeTextWithoutOverrunning) {
    EXPECT_EQ(app::TypingEffect("HELLO", Seconds(100.0), 10.0), "HELLO");
}

TEST(TypingEffect, EmptyTextIsAlwaysEmpty) {
    EXPECT_EQ(app::TypingEffect("", Seconds(0.0), 10.0), "");
    EXPECT_EQ(app::TypingEffect("", Seconds(100.0), 10.0), "");
}

TEST(TypingEffect, ZeroOrNegativeSpeedShowsTheWholeTextImmediately) {
    EXPECT_EQ(app::TypingEffect("HELLO", Seconds(0.0), 0.0), "HELLO");
    EXPECT_EQ(app::TypingEffect("HELLO", Seconds(0.0), -5.0), "HELLO");
}

TEST(TypingEffect, LengthGrowsMonotonicallyWithElapsedTime) {
    std::string::size_type previousLength = 0;
    for (double seconds = 0.0; seconds <= 1.0; seconds += 0.05) {
        const std::string visible = app::TypingEffect("THE QUICK BROWN FOX", Seconds(seconds), 20.0);
        EXPECT_GE(visible.size(), previousLength) << seconds;
        previousLength = visible.size();
    }
}

TEST(TypingEffect, VisiblePrefixIsAlwaysAPrefixOfTheFullText) {
    const std::string full = "THE QUICK BROWN FOX";
    for (double seconds = 0.0; seconds <= 2.0; seconds += 0.1) {
        const std::string visible = app::TypingEffect(full, Seconds(seconds), 20.0);
        EXPECT_EQ(full.compare(0, visible.size(), visible), 0) << seconds;
    }
}

using Clock = net::Clock;

Clock::time_point At(int secondsSinceEpoch) {
    return Clock::time_point(std::chrono::seconds(secondsSinceEpoch));
}

app::LogEntry MakeEntry(Clock::time_point time, std::string message) {
    return {time, std::move(message)};
}

TEST(TypingStartTimes, EmptyInputGivesEmptyOutput) {
    EXPECT_TRUE(app::TypingStartTimes({}, 10.0).empty());
}

TEST(TypingStartTimes, ASingleEntryStartsAtItsOwnTimestamp) {
    const std::deque<app::LogEntry> entries = {MakeEntry(At(1000), "HI")};
    const auto starts = app::TypingStartTimes(entries, 10.0);
    ASSERT_EQ(starts.size(), 1U);
    EXPECT_EQ(starts[0], At(1000));
}

TEST(TypingStartTimes, TwoEntriesWithTheSameTimestampStartOneAfterTheOther) {
    // "HELLO" at 10 chars/sec takes 0.5s to type, so the second entry -- logged at the exact
    // same instant as the first -- must not start until that finishes.
    const std::deque<app::LogEntry> entries = {MakeEntry(At(1000), "HELLO"),
                                               MakeEntry(At(1000), "WORLD")};
    const auto starts = app::TypingStartTimes(entries, 10.0);
    ASSERT_EQ(starts.size(), 2U);
    EXPECT_EQ(starts[0], At(1000));
    EXPECT_EQ(starts[1], At(1000) + std::chrono::milliseconds(500));
}

TEST(TypingStartTimes, ThreeEntriesAtTheSameInstantQueueInOrder) {
    const std::deque<app::LogEntry> entries = {
        MakeEntry(At(1000), "AAAAA"),  // 0.5s to type at 10 chars/sec
        MakeEntry(At(1000), "BB"),     // 0.2s
        MakeEntry(At(1000), "CCC"),    // 0.3s
    };
    const auto starts = app::TypingStartTimes(entries, 10.0);
    ASSERT_EQ(starts.size(), 3U);
    EXPECT_EQ(starts[0], At(1000));
    EXPECT_EQ(starts[1], At(1000) + std::chrono::milliseconds(500));
    EXPECT_EQ(starts[2], At(1000) + std::chrono::milliseconds(700));
}

TEST(TypingStartTimes, ALaterEntryWhoseOwnTimestampIsAfterThePreviousFinishStartsOnTime) {
    // The first entry finishes at 1000.5s; the second's own timestamp, 1010s, is well after
    // that, so it starts on its own schedule rather than being held back.
    const std::deque<app::LogEntry> entries = {MakeEntry(At(1000), "HELLO"),
                                               MakeEntry(At(1010), "WORLD")};
    const auto starts = app::TypingStartTimes(entries, 10.0);
    ASSERT_EQ(starts.size(), 2U);
    EXPECT_EQ(starts[0], At(1000));
    EXPECT_EQ(starts[1], At(1010));
}

TEST(TypingStartTimes, ZeroOrNegativeSpeedNeverDelaysALaterEntry) {
    // No typing delay at all (charsPerSecond <= 0 means TypingEffect shows text immediately), so
    // even same-instant entries all start at their own real timestamp.
    const std::deque<app::LogEntry> entries = {MakeEntry(At(1000), "HELLO"),
                                               MakeEntry(At(1000), "WORLD")};
    const auto starts = app::TypingStartTimes(entries, 0.0);
    ASSERT_EQ(starts.size(), 2U);
    EXPECT_EQ(starts[0], At(1000));
    EXPECT_EQ(starts[1], At(1000));
}

} // namespace
