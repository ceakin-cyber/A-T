#include "app/config.h"
#include "app/observer.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

TEST(ParseConfig, EmptyTextGivesTheDefaults) {
    const app::Config config = app::ParseConfig("");
    EXPECT_DOUBLE_EQ(config.observer.latitude, app::kObserver.latitude);
    EXPECT_DOUBLE_EQ(config.observer.longitude, app::kObserver.longitude);
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, app::kObserver.altitudeKm);
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 25544);
    EXPECT_EQ(config.watchlist[0].name, "ISS (ZARYA)");
}

TEST(ParseConfig, ReadsTheObserverLocation) {
    const app::Config config = app::ParseConfig("observer_lat_deg = -33.8688\n"
                                                "observer_lon_deg = 151.2093\n"
                                                "observer_alt_km = 0.058\n");
    EXPECT_NEAR(Degrees(config.observer.latitude), -33.8688, 1e-9);
    EXPECT_NEAR(Degrees(config.observer.longitude), 151.2093, 1e-9);
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, 0.058);
}

TEST(ParseConfig, OnlyGivenFieldsChangeFromTheDefault) {
    const app::Config config = app::ParseConfig("observer_alt_km = 2.5\n");
    EXPECT_DOUBLE_EQ(config.observer.latitude, app::kObserver.latitude);
    EXPECT_DOUBLE_EQ(config.observer.longitude, app::kObserver.longitude);
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, 2.5);
}

TEST(ParseConfig, IgnoresBlankLinesAndComments) {
    const app::Config config = app::ParseConfig("\n"
                                                "# a comment\n"
                                                "   \n"
                                                "observer_alt_km = 1.0\n"
                                                "# another comment\n");
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, 1.0);
}

TEST(ParseConfig, ToleratesSurroundingWhitespace) {
    const app::Config config = app::ParseConfig("  observer_alt_km   =   3.0  \n");
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, 3.0);
}

TEST(ParseConfig, ReadsASingleWatchEntryWithAName) {
    const app::Config config = app::ParseConfig("watch 20580 HUBBLE\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 20580);
    EXPECT_EQ(config.watchlist[0].name, "HUBBLE");
}

TEST(ParseConfig, ReadsAWatchEntryWithNoName) {
    const app::Config config = app::ParseConfig("watch 20580\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 20580);
    EXPECT_TRUE(config.watchlist[0].name.empty());
}

TEST(ParseConfig, ReadsAWatchEntryWithAMultiWordName) {
    const app::Config config = app::ParseConfig("watch 25544 ISS (ZARYA)\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].name, "ISS (ZARYA)");
}

TEST(ParseConfig, ReadsMultipleWatchEntriesInOrder) {
    const app::Config config = app::ParseConfig("watch 25544 ISS (ZARYA)\n"
                                                "watch 20580 HUBBLE\n"
                                                "watch 43013\n");
    ASSERT_EQ(config.watchlist.size(), 3U);
    EXPECT_EQ(config.watchlist[0].noradId, 25544);
    EXPECT_EQ(config.watchlist[1].noradId, 20580);
    EXPECT_EQ(config.watchlist[2].noradId, 43013);
}

TEST(ParseConfig, AWatchSectionReplacesTheDefaultEntirelyEvenWithOneEntry) {
    const app::Config config = app::ParseConfig("watch 20580 HUBBLE\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 20580);
}

TEST(ParseConfig, SkipsAMalformedAssignmentAndKeepsReadingTheRest) {
    const app::Config config = app::ParseConfig("observer_alt_km = not_a_number\n"
                                                "observer_lat_deg = 10.0\n");
    EXPECT_NEAR(Degrees(config.observer.latitude), 10.0, 1e-9);
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, app::kObserver.altitudeKm);
}

TEST(ParseConfig, SkipsALineWithNoEqualsSignAndKeepsReadingTheRest) {
    const app::Config config = app::ParseConfig("this is not a valid line\n"
                                                "observer_alt_km = 5.0\n");
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, 5.0);
}

TEST(ParseConfig, SkipsAnUnknownKeyAndKeepsReadingTheRest) {
    const app::Config config = app::ParseConfig("made_up_key = 1.0\n"
                                                "observer_alt_km = 6.0\n");
    EXPECT_DOUBLE_EQ(config.observer.altitudeKm, 6.0);
}

TEST(ParseConfig, SkipsAWatchLineWithANonNumericIdAndKeepsReadingTheRest) {
    const app::Config config = app::ParseConfig("watch not-a-number SOMETHING\n"
                                                "watch 20580 HUBBLE\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 20580);
}

TEST(ParseConfig, SkipsAWatchLineWithAZeroOrNegativeId) {
    const app::Config config = app::ParseConfig("watch 0 ZERO\n"
                                                "watch -5 NEGATIVE\n"
                                                "watch 20580 HUBBLE\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 20580);
}

TEST(ParseConfig, DoesNotTreatAKeyStartingWithWatchAsAWatchLine) {
    // "watchlist_something = 1.0" must not be mistaken for a "watch" entry.
    const app::Config config = app::ParseConfig("watchlist_extra = 1.0\nwatch 20580\n");
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 20580);
}

TEST(LoadConfig, ReturnsTheDefaultsWhenTheFileDoesNotExist) {
    const app::Config config = app::LoadConfig("/nonexistent/path/config.txt");
    EXPECT_DOUBLE_EQ(config.observer.latitude, app::kObserver.latitude);
    ASSERT_EQ(config.watchlist.size(), 1U);
    EXPECT_EQ(config.watchlist[0].noradId, 25544);
}

TEST(DefaultConfigDir, EndsWithTheAppName) {
    EXPECT_EQ(app::DefaultConfigDir().filename(), "a-t");
}

} // namespace
