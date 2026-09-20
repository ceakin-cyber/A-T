#include "app/tracked_satellite.h"

#include <chrono>
#include <gtest/gtest.h>
#include <string>

namespace {

using namespace std::chrono_literals;

// ISS elements from Celestrak, 2026-09-20.
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\n"
                         "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\n";

// A valid TLE with a 1436 minute period: a deep-space orbit, which SGP4 here cannot propagate.
const char* const kGeostationary =
    "1 90004U 98067A   26263.14255447  .00007470  00000+0  00000-0 0  9991\n"
    "2 90004   0.0500  80.0000 0002000  90.0000  10.0000  1.00270000123455\n";

net::LoadedTle Loaded(const std::string& text, net::TleSource source = net::TleSource::Network) {
    return {text, net::Clock::now() - 3h, source};
}

TEST(MakeSatellite, ParsesTheTleAndInitializesThePropagator) {
    const auto satellite = app::MakeSatellite(Loaded(kIss));
    ASSERT_TRUE(satellite.has_value());

    EXPECT_EQ(satellite->tle.name, "ISS (ZARYA)");
    EXPECT_EQ(satellite->tle.catalogNumber, 25544);
    EXPECT_EQ(satellite->tle.epochYear, 2026);

    // The model is exactly what SGP4 initialization gives for the same elements.
    const auto expected = core::InitSgp4(*core::ParseTle(kIss));
    ASSERT_TRUE(expected.has_value());
    EXPECT_DOUBLE_EQ(satellite->model.meanMotion, expected->meanMotion);
    EXPECT_DOUBLE_EQ(satellite->model.semiMajorAxis, expected->semiMajorAxis);
    EXPECT_DOUBLE_EQ(satellite->model.epochJd, expected->epochJd);
}

TEST(MakeSatellite, ProducesASatelliteThatCanBePropagated) {
    const auto satellite = app::MakeSatellite(Loaded(kIss));
    ASSERT_TRUE(satellite.has_value());
    const auto state = core::Propagate(satellite->model, 30.0);
    ASSERT_TRUE(state.has_value());
    const double radius =
        std::sqrt(state->position.x * state->position.x + state->position.y * state->position.y +
                  state->position.z * state->position.z);
    EXPECT_NEAR(radius, 6378.135 + 420.0, 40.0);
}

TEST(MakeSatellite, KeepsTheFetchTimeAndSource) {
    const net::LoadedTle stale = Loaded(kIss, net::TleSource::StaleCache);
    const auto satellite = app::MakeSatellite(stale);
    ASSERT_TRUE(satellite.has_value());
    EXPECT_EQ(satellite->fetchedAt, stale.fetchedAt);
    EXPECT_EQ(satellite->source, net::TleSource::StaleCache);
}

TEST(MakeSatellite, RejectsTextThatIsNotATle) {
    EXPECT_FALSE(app::MakeSatellite(Loaded("No GP data found")).has_value());
    EXPECT_FALSE(app::MakeSatellite(Loaded("")).has_value());
}

TEST(MakeSatellite, RejectsATleWithABadChecksum) {
    std::string text = kIss;
    text[text.find("9991") + 3] = '2'; // line 1 checksum 1 -> 2
    EXPECT_FALSE(app::MakeSatellite(Loaded(text)).has_value());
}

TEST(MakeSatellite, RejectsDeepSpaceOrbits) {
    EXPECT_FALSE(app::MakeSatellite(Loaded(kGeostationary)).has_value());
}

TEST(ToString, NamesEachTleSource) {
    EXPECT_STREQ(net::ToString(net::TleSource::Network), "network");
    EXPECT_STREQ(net::ToString(net::TleSource::FreshCache), "cache");
    EXPECT_STREQ(net::ToString(net::TleSource::StaleCache), "stale cache");
}

} // namespace
