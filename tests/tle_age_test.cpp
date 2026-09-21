#include "app/tle_age.h"

#include <chrono>
#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

// ISS elements from Celestrak, 2026-09-20.
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\n"
                         "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\n";

app::TrackedSatellite Satellite() {
    const auto loaded = net::LoadedTle{kIss, net::Clock::now(), net::TleSource::Network};
    const auto satellite = app::MakeSatellite(loaded);
    EXPECT_TRUE(satellite.has_value());
    return satellite.value();
}

net::Clock::time_point FromJulianDate(double julianDate) {
    const std::chrono::duration<double> sinceEpoch((julianDate - 2440587.5) * 86400.0);
    return net::Clock::time_point(std::chrono::duration_cast<net::Clock::duration>(sinceEpoch));
}

app::Seconds Hours(double hours) {
    return app::Seconds(hours * 3600.0);
}

TEST(TleAge, IsZeroAtTheEpoch) {
    const app::TrackedSatellite satellite = Satellite();
    const app::Seconds age = app::TleAge(satellite, FromJulianDate(satellite.model.epochJd));
    EXPECT_NEAR(age.count(), 0.0, 1e-3);
}

TEST(TleAge, GrowsWithTheClock) {
    const app::TrackedSatellite satellite = Satellite();
    const double epoch = satellite.model.epochJd;
    EXPECT_NEAR(app::TleAge(satellite, FromJulianDate(epoch + 3.5 / 24.0)).count(), 3.5 * 3600.0,
                1e-2);
    EXPECT_NEAR(app::TleAge(satellite, FromJulianDate(epoch + 2.25)).count(), 2.25 * 86400.0, 1e-2);
}

TEST(TleAge, IsNegativeBeforeTheEpoch) {
    const app::TrackedSatellite satellite = Satellite();
    EXPECT_LT(app::TleAge(satellite, FromJulianDate(satellite.model.epochJd - 0.5)).count(), 0.0);
}

TEST(TleAge, MeasuresFromTheEpochNotFromTheDownloadTime) {
    // The satellite was "downloaded" just now, but its elements are from the epoch, which is
    // earlier.
    const app::TrackedSatellite satellite = Satellite();
    const net::Clock::time_point now = FromJulianDate(satellite.model.epochJd + 1.0);
    EXPECT_NEAR(app::TleAge(satellite, now).count(), 86400.0, 1e-2);
}

TEST(FormatAge, MinutesOnly) {
    EXPECT_EQ(app::FormatAge(0s), "0M");
    EXPECT_EQ(app::FormatAge(59s), "0M");
    EXPECT_EQ(app::FormatAge(60s), "1M");
    EXPECT_EQ(app::FormatAge(59min), "59M");
}

TEST(FormatAge, HoursAndMinutes) {
    EXPECT_EQ(app::FormatAge(60min), "1H 0M");
    EXPECT_EQ(app::FormatAge(5h + 12min), "5H 12M");
    EXPECT_EQ(app::FormatAge(23h + 59min + 59s), "23H 59M");
}

TEST(FormatAge, DaysAndHours) {
    EXPECT_EQ(app::FormatAge(24h), "1D 0H");
    EXPECT_EQ(app::FormatAge(26h), "1D 2H");
    EXPECT_EQ(app::FormatAge(3 * 24h + 4h + 30min), "3D 4H");
    EXPECT_EQ(app::FormatAge(10 * 24h), "10D 0H");
}

TEST(FormatAge, NegativeAgeShowsAsZero) {
    EXPECT_EQ(app::FormatAge(-5min), "0M");
    EXPECT_EQ(app::FormatAge(-100h), "0M");
}

TEST(ClassifyAge, FreshUpToThreeDays) {
    EXPECT_EQ(app::ClassifyAge(0s), app::TleFreshness::Fresh);
    EXPECT_EQ(app::ClassifyAge(Hours(6)), app::TleFreshness::Fresh);
    EXPECT_EQ(app::ClassifyAge(Hours(72)), app::TleFreshness::Fresh);
}

TEST(ClassifyAge, AgingBetweenThreeAndSevenDays) {
    EXPECT_EQ(app::ClassifyAge(Hours(72.01)), app::TleFreshness::Aging);
    EXPECT_EQ(app::ClassifyAge(Hours(5 * 24)), app::TleFreshness::Aging);
    EXPECT_EQ(app::ClassifyAge(Hours(7 * 24)), app::TleFreshness::Aging);
}

TEST(ClassifyAge, StaleBeyondSevenDays) {
    EXPECT_EQ(app::ClassifyAge(Hours(7 * 24 + 0.01)), app::TleFreshness::Stale);
    EXPECT_EQ(app::ClassifyAge(Hours(60 * 24)), app::TleFreshness::Stale);
}

TEST(ClassifyAge, NegativeAgeCountsAsFresh) {
    EXPECT_EQ(app::ClassifyAge(-1h), app::TleFreshness::Fresh);
}

} // namespace
