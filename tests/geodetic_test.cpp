#include "core/frames.h"
#include "core/geodetic.h"
#include "core/sgp4.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

void ExpectVec(const core::Vec3& actual, const core::Vec3& expected, double toleranceKm) {
    EXPECT_NEAR(actual.x, expected.x, toleranceKm);
    EXPECT_NEAR(actual.y, expected.y, toleranceKm);
    EXPECT_NEAR(actual.z, expected.z, toleranceKm);
}

// Reference points below were computed independently, in 40-digit decimal arithmetic from the
// WGS-84 defining constants (a = 6378.137 km, 1/f = 298.257223563).

TEST(Wgs84, SemiMinorAxis) {
    EXPECT_NEAR(core::wgs84::kSemiMinorAxisKm, 6356.752314245179, 1e-9);
}

TEST(GeodeticToEcef, EquatorAtPrimeMeridian) {
    ExpectVec(core::GeodeticToEcef({0.0, 0.0, 0.0}), {6378.137, 0.0, 0.0}, 1e-9);
}

TEST(GeodeticToEcef, EquatorAt90East) {
    ExpectVec(core::GeodeticToEcef({0.0, kPi / 2.0, 0.0}), {0.0, 6378.137, 0.0}, 1e-9);
}

TEST(GeodeticToEcef, Poles) {
    const double b = core::wgs84::kSemiMinorAxisKm;
    ExpectVec(core::GeodeticToEcef({kPi / 2.0, 0.0, 0.0}), {0.0, 0.0, b}, 1e-9);
    ExpectVec(core::GeodeticToEcef({-kPi / 2.0, 0.0, 0.0}), {0.0, 0.0, -b}, 1e-9);
}

TEST(GeodeticToEcef, GeostationaryOrbit) {
    ExpectVec(core::GeodeticToEcef({0.0, 0.0, 35786.0}), {42164.137, 0.0, 0.0}, 1e-9);
}

TEST(GeodeticToEcef, MidLatitudePoints) {
    ExpectVec(core::GeodeticToEcef({Radians(45.0), 0.0, 0.0}),
              {4517.5908788489, 0.0, 4487.3484088659}, 1e-6);
    ExpectVec(core::GeodeticToEcef({Radians(45.0), kPi / 2.0, 1.0}),
              {0.0, 4518.2979856301, 4488.0555156471}, 1e-6);
    // Sydney: southern hemisphere, eastern longitude. And a point a hair from the north pole.
    ExpectVec(core::GeodeticToEcef({Radians(-33.8688), Radians(151.2093), 0.058}),
              {-4646.0934772883, 2553.2295358171, -3534.4047109104}, 1e-6);
    ExpectVec(core::GeodeticToEcef({Radians(89.999), Radians(10.0), 0.0}),
              {0.1099970970, 0.0193954560, 6356.7523132705}, 1e-6);
}

TEST(EcefToGeodetic, ReferencePointsGoBackToTheirCoordinates) {
    auto check = [](double latDeg, double lonDeg, double h, const core::Vec3& ecef) {
        const core::Geodetic g = core::EcefToGeodetic(ecef);
        EXPECT_NEAR(Degrees(g.latitude), latDeg, 1e-9) << latDeg;
        EXPECT_NEAR(Degrees(g.longitude), lonDeg, 1e-9) << latDeg;
        EXPECT_NEAR(g.altitudeKm, h, 1e-6) << latDeg;
    };
    check(45.0, 0.0, 0.0, {4517.5908788489, 0.0, 4487.3484088659});
    check(45.0, 90.0, 1.0, {0.0, 4518.2979856301, 4488.0555156471});
    check(-33.8688, 151.2093, 0.058, {-4646.0934772883, 2553.2295358171, -3534.4047109104});
    check(0.0, 0.0, 35786.0, {42164.137, 0.0, 0.0});
}

TEST(EcefToGeodetic, Poles) {
    const double b = core::wgs84::kSemiMinorAxisKm;
    const core::Geodetic north = core::EcefToGeodetic({0.0, 0.0, b + 400.0});
    EXPECT_NEAR(north.latitude, kPi / 2.0, 1e-12);
    EXPECT_NEAR(north.altitudeKm, 400.0, 1e-9);

    const core::Geodetic south = core::EcefToGeodetic({0.0, 0.0, -b});
    EXPECT_NEAR(south.latitude, -kPi / 2.0, 1e-12);
    EXPECT_NEAR(south.altitudeKm, 0.0, 1e-9);
}

TEST(EcefToGeodetic, LongitudeCoversTheWholeCircle) {
    EXPECT_NEAR(Degrees(core::EcefToGeodetic({0.0, 6378.137, 0.0}).longitude), 90.0, 1e-12);
    EXPECT_NEAR(Degrees(core::EcefToGeodetic({-6378.137, 0.0, 0.0}).longitude), 180.0, 1e-12);
    EXPECT_NEAR(Degrees(core::EcefToGeodetic({0.0, -6378.137, 0.0}).longitude), -90.0, 1e-12);
}

TEST(EcefToGeodetic, RoundTripsAcrossTheGlobeAndFromTheSeaFloorToTheMoon) {
    for (double latDeg = -89.9; latDeg <= 89.9; latDeg += 8.9) {
        for (double lonDeg = -179.0; lonDeg <= 180.0; lonDeg += 37.0) {
            for (const double altitude : {-10.0, 0.0, 0.4, 400.0, 2000.0, 35786.0, 384400.0}) {
                const core::Geodetic original{Radians(latDeg), Radians(lonDeg), altitude};
                const core::Geodetic back = core::EcefToGeodetic(core::GeodeticToEcef(original));
                EXPECT_NEAR(back.latitude, original.latitude, 1e-11)
                    << latDeg << " " << lonDeg << " " << altitude;
                EXPECT_NEAR(back.longitude, original.longitude, 1e-11)
                    << latDeg << " " << lonDeg << " " << altitude;
                EXPECT_NEAR(back.altitudeKm, original.altitudeKm, 1e-6)
                    << latDeg << " " << lonDeg << " " << altitude;
            }
        }
    }
}

TEST(EcefToGeodetic, LatitudeIsGeodeticNotGeocentric) {
    // At 45 degrees geodetic latitude the geocentric latitude is about 0.19 degrees smaller.
    const core::Vec3 ecef = core::GeodeticToEcef({Radians(45.0), 0.0, 0.0});
    const double geocentric = Degrees(std::atan2(ecef.z, ecef.x));
    EXPECT_NEAR(geocentric, 44.8076, 1e-3);
    EXPECT_NEAR(Degrees(core::EcefToGeodetic(ecef).latitude), 45.0, 1e-9);
}

TEST(EcefToGeodetic, IssGroundTrackStaysWithinItsInclinationAndAltitudeBand) {
    // ISS elements from Celestrak, 2026-09-20.
    const auto tle =
        core::ParseTle("ISS (ZARYA)\n"
                       "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\n"
                       "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\n");
    ASSERT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(*tle);
    ASSERT_TRUE(model.has_value());

    double maxLatitude = 0.0;
    double minLatitude = 0.0;
    for (double minutes = 0.0; minutes <= 1440.0; minutes += 1.0) {
        const auto state = core::Propagate(*model, minutes);
        ASSERT_TRUE(state.has_value());
        const core::Vec3 ecef =
            core::TemeToEcef(state->position, model->epochJd + minutes / 1440.0);
        const core::Geodetic g = core::EcefToGeodetic(ecef);

        EXPECT_GT(g.altitudeKm, 380.0) << minutes;
        EXPECT_LT(g.altitudeKm, 450.0) << minutes;
        EXPECT_GT(g.longitude, -kPi - 1e-12);
        EXPECT_LE(g.longitude, kPi + 1e-12);
        maxLatitude = std::max(maxLatitude, Degrees(g.latitude));
        minLatitude = std::min(minLatitude, Degrees(g.latitude));
    }
    // Over a day the track reaches almost the full +-51.63 degrees.
    EXPECT_NEAR(maxLatitude, 51.6, 0.2);
    EXPECT_NEAR(minLatitude, -51.6, 0.2);
}

} // namespace
