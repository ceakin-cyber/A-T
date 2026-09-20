#include "core/frames.h"
#include "core/sgp4.h"
#include "core/topocentric.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;
constexpr double kEquatorialRadiusKm = 6378.137;

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

core::Vec3 Ecef(double latDeg, double lonDeg, double altitudeKm) {
    return core::GeodeticToEcef({Radians(latDeg), Radians(lonDeg), altitudeKm});
}

core::Geodetic Site(double latDeg, double lonDeg, double altitudeKm) {
    return {Radians(latDeg), Radians(lonDeg), altitudeKm};
}

TEST(EcefToLookAngles, SatelliteDirectlyOverheadIsAtZenith) {
    const core::LookAngles look =
        core::EcefToLookAngles(Site(0.0, 0.0, 0.0), Ecef(0.0, 0.0, 400.0));
    EXPECT_NEAR(Degrees(look.elevation), 90.0, 1e-9);
    EXPECT_NEAR(look.rangeKm, 400.0, 1e-9);
}

TEST(EcefToLookAngles, OverheadAtAnyLatitudeAndObserverAltitude) {
    for (const double lat : {-70.0, -33.0, 12.0, 51.5, 80.0}) {
        const core::LookAngles look =
            core::EcefToLookAngles(Site(lat, 37.0, 1.5), Ecef(lat, 37.0, 421.5));
        EXPECT_NEAR(Degrees(look.elevation), 90.0, 1e-8) << lat;
        EXPECT_NEAR(look.rangeKm, 420.0, 1e-8) << lat;
    }
}

TEST(EcefToLookAngles, PointOnTheEquatorTenDegreesEast) {
    // On the equator the surface is a circle of radius a, so a point 10 degrees of longitude
    // away lies on a chord: its range is 2*a*sin(5 degrees) and it is 5 degrees below the
    // horizon, due east.
    const core::LookAngles look = core::EcefToLookAngles(Site(0.0, 0.0, 0.0), Ecef(0.0, 10.0, 0.0));
    EXPECT_NEAR(Degrees(look.azimuth), 90.0, 1e-9);
    EXPECT_NEAR(Degrees(look.elevation), -5.0, 1e-9);
    EXPECT_NEAR(look.rangeKm, 2.0 * kEquatorialRadiusKm * std::sin(Radians(5.0)), 1e-9);
}

TEST(EcefToLookAngles, DueWestOnTheEquator) {
    const core::LookAngles look =
        core::EcefToLookAngles(Site(0.0, 0.0, 0.0), Ecef(0.0, -10.0, 0.0));
    EXPECT_NEAR(Degrees(look.azimuth), 270.0, 1e-9);
    EXPECT_NEAR(Degrees(look.elevation), -5.0, 1e-9);
}

TEST(EcefToLookAngles, DueNorthAndSouthAlongAMeridian) {
    const core::LookAngles north =
        core::EcefToLookAngles(Site(10.0, 20.0, 0.0), Ecef(15.0, 20.0, 400.0));
    EXPECT_NEAR(Degrees(north.azimuth), 0.0, 1e-9);
    EXPECT_GT(north.elevation, 0.0);

    const core::LookAngles south =
        core::EcefToLookAngles(Site(10.0, 20.0, 0.0), Ecef(5.0, 20.0, 400.0));
    EXPECT_NEAR(Degrees(south.azimuth), 180.0, 1e-9);
    EXPECT_GT(south.elevation, 0.0);
}

TEST(EcefToLookAngles, AntipodeIsStraightDownThroughTheEarth) {
    const core::LookAngles look =
        core::EcefToLookAngles(Site(0.0, 0.0, 0.0), Ecef(0.0, 180.0, 0.0));
    EXPECT_NEAR(Degrees(look.elevation), -90.0, 1e-9);
    EXPECT_NEAR(look.rangeKm, 2.0 * kEquatorialRadiusKm, 1e-9);
}

// Reference values computed independently, in 40-digit decimal arithmetic, with an East/North/Up
// basis built from cross products (east = z x up, north = up x east) rather than the rotation
// matrix used by the implementation.
TEST(EcefToLookAngles, MatchesIndependentReferenceValues) {
    struct Case {
        core::Geodetic site;
        core::Vec3 satellite;
        double azimuthDeg;
        double elevationDeg;
        double rangeKm;
    };
    const Case cases[] = {
        // Greenwich, looking at a satellite over 45N 10E at 420 km.
        {Site(51.4779, 0.0, 0.062), Ecef(45.0, 10.0, 420.0), 130.2967775426, 16.8040315353,
         1145.0492043754},
        // Sydney, looking at a satellite over 20S 165E at 410 km.
        {Site(-33.8688, 151.2093, 0.058), Ecef(-20.0, 165.0, 410.0), 45.0669749210, 1.6057056895,
         2149.4975824931},
        // Mauna Kea (4.2 km up), looking at a satellite over 25N 150W at 420 km.
        {Site(19.8207, -155.4681, 4.2), Ecef(25.0, -150.0, 420.0), 43.4469103489, 22.9537410580,
         927.6864416116},
    };
    for (const Case& c : cases) {
        const core::LookAngles look = core::EcefToLookAngles(c.site, c.satellite);
        EXPECT_NEAR(Degrees(look.azimuth), c.azimuthDeg, 1e-8) << c.rangeKm;
        EXPECT_NEAR(Degrees(look.elevation), c.elevationDeg, 1e-8) << c.rangeKm;
        EXPECT_NEAR(look.rangeKm, c.rangeKm, 1e-6) << c.rangeKm;
    }
}

TEST(EcefToLookAngles, AzimuthIsAlwaysInZeroToTwoPi) {
    for (double lon = -180.0; lon <= 180.0; lon += 15.0) {
        const core::LookAngles look =
            core::EcefToLookAngles(Site(40.0, 10.0, 0.0), Ecef(30.0, lon, 400.0));
        EXPECT_GE(look.azimuth, 0.0) << lon;
        EXPECT_LT(look.azimuth, 2.0 * kPi) << lon;
    }
}

TEST(EcefToLookAngles, CoincidentPointGivesZeros) {
    const core::Vec3 site = Ecef(10.0, 20.0, 0.0);
    const core::LookAngles look = core::EcefToLookAngles(Site(10.0, 20.0, 0.0), site);
    EXPECT_DOUBLE_EQ(look.rangeKm, 0.0);
    EXPECT_DOUBLE_EQ(look.elevation, 0.0);
}

TEST(EcefToEnu, AxesPointEastNorthAndUp) {
    // Moving 1 km along each local axis from the observer gives a unit ENU vector.
    const core::Geodetic site = Site(0.0, 0.0, 0.0);
    // At (0, 0): up is +X, east is +Y, north is +Z.
    const core::Vec3 origin = core::GeodeticToEcef(site);
    const core::Vec3 east = core::EcefToEnu(site, {origin.x, origin.y + 1.0, origin.z});
    EXPECT_NEAR(east.x, 1.0, 1e-12);
    EXPECT_NEAR(east.y, 0.0, 1e-12);
    EXPECT_NEAR(east.z, 0.0, 1e-12);
    const core::Vec3 north = core::EcefToEnu(site, {origin.x, origin.y, origin.z + 1.0});
    EXPECT_NEAR(north.y, 1.0, 1e-12);
    const core::Vec3 up = core::EcefToEnu(site, {origin.x + 1.0, origin.y, origin.z});
    EXPECT_NEAR(up.z, 1.0, 1e-12);
}

TEST(EcefToLookAngles, IssPassesFromAMidLatitudeSiteHaveConsistentGeometry) {
    // ISS elements from Celestrak, 2026-09-20.
    const auto tle =
        core::ParseTle("ISS (ZARYA)\n"
                       "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\n"
                       "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\n");
    ASSERT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(*tle);
    ASSERT_TRUE(model.has_value());

    const core::Geodetic site = Site(40.0, -75.0, 0.05);
    int minutesAboveHorizon = 0;
    double bestElevation = -kPi;
    for (double minutes = 0.0; minutes <= 1440.0; minutes += 1.0) {
        const auto state = core::Propagate(*model, minutes);
        ASSERT_TRUE(state.has_value());
        const core::Vec3 ecef =
            core::TemeToEcef(state->position, model->epochJd + minutes / 1440.0);
        const core::LookAngles look = core::EcefToLookAngles(site, ecef);

        EXPECT_GE(look.elevation, -kPi / 2.0);
        EXPECT_LE(look.elevation, kPi / 2.0);
        EXPECT_GT(look.rangeKm, 350.0) << minutes;
        if (look.elevation >= 0.0) {
            ++minutesAboveHorizon;
            // A 420 km orbit is never more than about 2300 km away when it is above the horizon.
            EXPECT_LT(look.rangeKm, 2500.0) << minutes;
        }
        bestElevation = std::max(bestElevation, look.elevation);
    }
    // From 40N the ISS (51.6 degree inclination) is visible for a few passes a day.
    EXPECT_GT(minutesAboveHorizon, 20);
    EXPECT_LT(minutesAboveHorizon, 120);
    EXPECT_GT(Degrees(bestElevation), 20.0);
}

} // namespace
