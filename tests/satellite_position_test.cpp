#include "app/satellite_position.h"
#include "core/frames.h"

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <numbers>
#include <string>

namespace {

constexpr double kPi = std::numbers::pi;

// ISS elements from Celestrak, 2026-09-20.
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\n"
                         "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\n";

// A synthetic orbit with a perigee near 90 km, which drag destroys within hours.
const char* const kDecaying =
    "1 90002U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9992\n"
    "2 90002  51.6307 190.1401 0005000 160.6694 199.4478 16.67663952123456\n";

app::TrackedSatellite Satellite(const char* text) {
    const auto loaded = net::LoadedTle{text, net::Clock::now(), net::TleSource::Network};
    const auto satellite = app::MakeSatellite(loaded);
    EXPECT_TRUE(satellite.has_value());
    return satellite.value();
}

// Converts a Julian date to a system clock time (the Unix epoch is JD 2440587.5).
net::Clock::time_point FromJulianDate(double julianDate) {
    const std::chrono::duration<double> sinceEpoch((julianDate - 2440587.5) * 86400.0);
    return net::Clock::time_point(std::chrono::duration_cast<net::Clock::duration>(sinceEpoch));
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

double Distance(const core::Vec3& a, const core::Vec3& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) +
                     (a.z - b.z) * (a.z - b.z));
}

TEST(ComputePosition, AtTheTleEpochMatchesRunningEachStageByHand) {
    const app::TrackedSatellite satellite = Satellite(kIss);
    const double epoch = satellite.model.epochJd;

    const auto position = app::ComputePosition(satellite, FromJulianDate(epoch));
    ASSERT_TRUE(position.has_value());

    const auto state = core::Propagate(satellite.model, 0.0);
    ASSERT_TRUE(state.has_value());
    const core::Vec3 ecef = core::TemeToEcef(state->position, epoch);
    const core::Geodetic geodetic = core::EcefToGeodetic(ecef);

    // The clock time is rounded to a nanosecond, which moves the satellite by well under a metre.
    EXPECT_LT(Distance(position->ecef, ecef), 1e-3);
    EXPECT_NEAR(position->geodetic.latitude, geodetic.latitude, 1e-6);
    EXPECT_NEAR(position->geodetic.longitude, geodetic.longitude, 1e-6);
    EXPECT_NEAR(position->geodetic.altitudeKm, geodetic.altitudeKm, 1e-3);
}

TEST(ComputePosition, StaysInsideTheIssOrbitOverADay) {
    const app::TrackedSatellite satellite = Satellite(kIss);
    const double epoch = satellite.model.epochJd;

    double maxLatitude = -90.0;
    double minLatitude = 90.0;
    for (int minute = 0; minute <= 1440; ++minute) {
        const auto position =
            app::ComputePosition(satellite, FromJulianDate(epoch + minute / 1440.0));
        ASSERT_TRUE(position.has_value()) << minute;

        EXPECT_GT(position->geodetic.altitudeKm, 380.0) << minute;
        EXPECT_LT(position->geodetic.altitudeKm, 450.0) << minute;
        EXPECT_NEAR(position->speedKmPerSec, 7.66, 0.1) << minute;
        EXPECT_GT(position->geodetic.longitude, -kPi - 1e-12);
        EXPECT_LE(position->geodetic.longitude, kPi + 1e-12);
        maxLatitude = std::max(maxLatitude, Degrees(position->geodetic.latitude));
        minLatitude = std::min(minLatitude, Degrees(position->geodetic.latitude));
    }
    EXPECT_NEAR(maxLatitude, 51.6, 0.2);
    EXPECT_NEAR(minLatitude, -51.6, 0.2);
}

TEST(ComputePosition, AdvancesSmoothlyFromOneSecondToTheNext) {
    // The ISS moves at about 7.66 km/s, so a second later it is about 7.66 km further along.
    const app::TrackedSatellite satellite = Satellite(kIss);
    const net::Clock::time_point start = FromJulianDate(satellite.model.epochJd + 0.25);
    auto previous = app::ComputePosition(satellite, start);
    ASSERT_TRUE(previous.has_value());
    for (int second = 1; second <= 120; ++second) {
        const auto current = app::ComputePosition(satellite, start + std::chrono::seconds(second));
        ASSERT_TRUE(current.has_value());
        // In ECEF the ground speed is a little lower than the inertial speed (Earth rotates
        // under the satellite), between about 7.0 and 7.7 km/s for this orbit.
        const double step = Distance(previous->ecef, current->ecef);
        EXPECT_GT(step, 6.9) << second;
        EXPECT_LT(step, 7.8) << second;
        previous = current;
    }
}

TEST(ComputePosition, WorksBeforeTheTleEpochToo) {
    const app::TrackedSatellite satellite = Satellite(kIss);
    const auto position =
        app::ComputePosition(satellite, FromJulianDate(satellite.model.epochJd - 1.0));
    ASSERT_TRUE(position.has_value());
    EXPECT_GT(position->geodetic.altitudeKm, 380.0);
    EXPECT_LT(position->geodetic.altitudeKm, 450.0);
}

TEST(ComputePosition, ReturnsNulloptWhenTheOrbitHasDecayed) {
    const app::TrackedSatellite satellite = Satellite(kDecaying);
    EXPECT_FALSE(
        app::ComputePosition(satellite, FromJulianDate(satellite.model.epochJd + 1.0)).has_value());
}

TEST(ComputePosition, WorksWithTheCurrentSystemClock) {
    // A smoke test of the path the app uses every frame. The bundled TLE is stale by the time
    // this runs, but a few weeks is still well within SGP4's range for the ISS.
    const app::TrackedSatellite satellite = Satellite(kIss);
    const auto position = app::ComputePosition(satellite, net::Clock::now());
    if (position) {
        EXPECT_GT(position->geodetic.altitudeKm, 300.0);
        EXPECT_LT(position->geodetic.altitudeKm, 500.0);
    }
}

// Positions reported by a separate tracker, api.wheretheiss.at, on 2026-09-20, together with the
// Celestrak TLE that was current then. This checks the whole chain (TLE, SGP4, TEME to ECEF,
// geodetic) against an independent implementation, offline. At the time of writing the two
// agreed to 2.35 km, entirely along the direction of travel, which is about 0.3 s of ISS motion.
TEST(ComputePosition, AgreesWithAnIndependentTrackerToWithinAFewKilometres) {
    const char* const tle =
        "ISS (ZARYA)\n"
        "1 25544U 98067A   26263.52959654  .00008422  00000+0  15975-3 0  9999\n"
        "2 25544  51.6308 188.2246 0004825 162.2847 197.8311 15.49196792586535\n";

    struct Sample {
        long long unixSeconds;
        double latitudeDeg;
        double longitudeDeg;
        double altitudeKm;
        double speedKmPerSec;
    };
    const Sample samples[] = {
        {1789961692, -29.138588700603, -22.560802595712, 425.77891164917, 7.656989628605278},
        {1789961732, -30.9461169329, -20.596427117514, 426.59792399429, 7.656425471773056},
        {1789961773, -32.760211747134, -18.499419926938, 427.43980991834, 7.655849320982223},
        {1789961793, -33.629655028294, -17.443519939849, 427.84985257671, 7.6555698054163885},
    };

    const app::TrackedSatellite satellite = Satellite(tle);
    for (const Sample& sample : samples) {
        const auto position = app::ComputePosition(
            satellite, net::Clock::time_point(std::chrono::seconds(sample.unixSeconds)));
        ASSERT_TRUE(position.has_value()) << sample.unixSeconds;

        // Distance between the two positions, as points in space.
        const core::Vec3 reference =
            core::GeodeticToEcef({sample.latitudeDeg * kPi / 180.0,
                                  sample.longitudeDeg * kPi / 180.0, sample.altitudeKm});
        EXPECT_LT(Distance(position->ecef, reference), 4.0) << sample.unixSeconds;

        EXPECT_NEAR(position->geodetic.altitudeKm, sample.altitudeKm, 0.5) << sample.unixSeconds;
        EXPECT_NEAR(Degrees(position->geodetic.latitude), sample.latitudeDeg, 0.05)
            << sample.unixSeconds;
        EXPECT_NEAR(Degrees(position->geodetic.longitude), sample.longitudeDeg, 0.05)
            << sample.unixSeconds;
        // Both report the speed relative to the Earth's centre, in an inertial frame.
        EXPECT_NEAR(position->speedKmPerSec, sample.speedKmPerSec, 0.005) << sample.unixSeconds;
    }
}

} // namespace
