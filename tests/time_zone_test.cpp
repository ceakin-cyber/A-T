#include "app/format.h"
#include "app/time_zone.h"

#include <chrono>
#include <gtest/gtest.h>

namespace {

using Clock = std::chrono::system_clock;

// 2026-09-21 14:32:10 UTC (northern summer time) and 2026-01-05 03:04:05 UTC (winter).
const Clock::time_point kSeptember{std::chrono::seconds(1790001130)};
const Clock::time_point kJanuary{std::chrono::seconds(1767582245)};

// The display zone is process-wide, so every test here puts it back to UTC afterward, leaving
// the rest of the suite (which expects UTC, the app's default) undisturbed.
class TimeZoneTest : public ::testing::Test {
  protected:
    void TearDown() override { app::SetDisplayTimeZone(app::kUtcTimeZone); }
};

TEST_F(TimeZoneTest, StartsInUtc) {
    EXPECT_EQ(app::DisplayTimeZone(), "UTC");
    EXPECT_EQ(app::FormatTime(kSeptember), "09-21 14:32:10");
    EXPECT_EQ(app::DisplayTimeZoneAbbreviation(kSeptember), "UTC");
}

TEST_F(TimeZoneTest, KnowsUtcLocalAndRealZones) {
    EXPECT_TRUE(app::IsKnownTimeZone("UTC"));
    EXPECT_TRUE(app::IsKnownTimeZone("LOCAL"));
    EXPECT_TRUE(app::IsKnownTimeZone("America/New_York"));
    EXPECT_TRUE(app::IsKnownTimeZone("Asia/Tokyo"));
}

TEST_F(TimeZoneTest, RejectsNamesThatAreNotZones) {
    EXPECT_FALSE(app::IsKnownTimeZone(""));
    EXPECT_FALSE(app::IsKnownTimeZone("Not/AZone"));
    EXPECT_FALSE(app::IsKnownTimeZone("America"));          // a directory, not a zone
    EXPECT_FALSE(app::IsKnownTimeZone("../../etc/passwd")); // no stepping outside the database
    EXPECT_FALSE(app::IsKnownTimeZone("/etc/localtime"));
    EXPECT_FALSE(app::IsKnownTimeZone(":America/New_York"));
}

TEST_F(TimeZoneTest, AnUnknownZoneLeavesTheDisplayAsItWas) {
    ASSERT_TRUE(app::SetDisplayTimeZone("Asia/Tokyo"));
    EXPECT_FALSE(app::SetDisplayTimeZone("Not/AZone"));
    EXPECT_EQ(app::DisplayTimeZone(), "Asia/Tokyo");
    EXPECT_EQ(app::FormatTime(kSeptember), "09-21 23:32:10");
}

TEST_F(TimeZoneTest, FollowsDaylightSavingTime) {
    ASSERT_TRUE(app::SetDisplayTimeZone("America/New_York"));
    // UTC-4 in September (EDT)...
    EXPECT_EQ(app::FormatTime(kSeptember), "09-21 10:32:10");
    EXPECT_EQ(app::DisplayTimeZoneAbbreviation(kSeptember), "EDT");
    // ...and UTC-5 in January (EST), which also moves this one back to the day before.
    EXPECT_EQ(app::FormatTime(kJanuary), "01-04 22:04:05");
    EXPECT_EQ(app::DisplayTimeZoneAbbreviation(kJanuary), "EST");
}

TEST_F(TimeZoneTest, HandlesZonesOffsetByPartsOfAnHour) {
    ASSERT_TRUE(app::SetDisplayTimeZone("Asia/Kolkata")); // UTC+5:30, no daylight saving
    EXPECT_EQ(app::FormatTime(kSeptember), "09-21 20:02:10");
    EXPECT_EQ(app::DisplayTimeZoneAbbreviation(kSeptember), "IST");
}

TEST_F(TimeZoneTest, ShowsAnOffsetForAZoneWithNoLettersOfItsOwn) {
    ASSERT_TRUE(app::SetDisplayTimeZone("America/Sao_Paulo")); // abbreviated just "-03"
    EXPECT_EQ(app::DisplayTimeZoneAbbreviation(kSeptember), "-0300");
}

TEST_F(TimeZoneTest, ClockTimesCarryTheZoneAndItsOwnIdeaOfToday) {
    ASSERT_TRUE(app::SetDisplayTimeZone("Asia/Tokyo")); // UTC+9
    // 23:32 in Tokyo; seen from 10:00 UTC the same UTC day, which is already 19:00 there.
    const Clock::time_point morning{std::chrono::seconds(1789984800)}; // 2026-09-21 10:00 UTC
    EXPECT_EQ(app::FormatClock(kSeptember, morning), "23:32 JST");
    // Seen from 20:00 UTC, which in Tokyo is already the next day, so the date is added.
    const Clock::time_point evening{std::chrono::seconds(1790020800)}; // 2026-09-21 20:00 UTC
    EXPECT_EQ(app::FormatClock(kSeptember, evening), "09-21 23:32 JST");
}

TEST_F(TimeZoneTest, CanSwitchToTheComputersOwnZoneAndBack) {
    EXPECT_TRUE(app::SetDisplayTimeZone(app::kLocalTimeZone));
    EXPECT_EQ(app::DisplayTimeZone(), "LOCAL");
    EXPECT_FALSE(app::DisplayTimeZoneAbbreviation(kSeptember).empty());
    EXPECT_TRUE(app::SetDisplayTimeZone(app::kUtcTimeZone));
    EXPECT_EQ(app::FormatTime(kSeptember), "09-21 14:32:10");
}

} // namespace
