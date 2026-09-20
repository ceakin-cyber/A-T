#include "core/sgp4.h"

#include <gtest/gtest.h>
#include <numbers>
#include <string>
#include <vector>

namespace {

// Expected values were generated once, offline, by running python-sgp4 (the reference
// implementation, WGS-72 constants) on each TLE. They are the values it holds after sgp4init:
// no_unkozai, a, con41, gsto, eta, altp (perigee height in Earth radii) and isimp.
struct Case {
    const char* name;
    const char* line1;
    const char* line2;
    double noUnkozai;
    double a;
    double con41;
    double gsto;
    double eta;
    double altp;
    int isimp;
};

const std::vector<Case>& Cases() {
    static const std::vector<Case> cases = {
        // Real: the ISS, fetched from Celestrak on 2026-09-20. Near-circular low Earth orbit.
        {"iss2026", "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991",
         "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472",
         0.06758856145142098, 1.0657885402662441, 0.15590891411638247, 0.8789580662324994,
         0.00959143340796816, 0.06527483018983582, 0},
        // Real: Vallado's SGP4 verification satellite 00005. Highly eccentric.
        {"sat00005", "1 00005U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753",
         "2 00005  34.2682 348.7242 1859667 331.7664  19.3264 10.82419157413667",
         0.04720630155917529, 1.3538998206027828, 1.048865087995659, 3.4691723423794016,
         0.7369095429280241, 0.10211953883469116, 0},
        // Synthetic (valid checksums): perigee of about 140 km, in the 98-156 km drag branch.
        {"lowPerigee140", "1 90001U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991",
         "2 90001  51.6307 190.1401 0005000 160.6694 199.4478 16.48512058123454",
         0.07192119818892878, 1.0225437665687562, 0.15590891411638247, 0.8789580662324994,
         0.04012949206382487, 0.02203249468547197, 1},
        // Synthetic: perigee of about 90 km, below 98 km, where s is fixed at 20 km.
        {"lowPerigee90", "1 90002U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9992",
         "2 90002  51.6307 190.1401 0005000 160.6694 199.4478 16.67663952123456",
         0.07275661928126016, 1.014701200905755, 0.15590891411638247, 0.8789580662324994,
         0.04386763436548967, 0.01419385030530207, 1},
        // Synthetic: a 221.5 minute period, just inside the near-Earth limit of 225 minutes.
        {"period221", "1 90003U 98067A   26263.14255447  .00007470  00000+0  10000-3 0  9994",
         "2 90003  63.4000 100.0000 3000000 270.0000  10.0000  6.50000000123457",
         0.028364524412885256, 1.9013825806550722, -0.3985353977733786, 0.8789580662324994,
         0.6415257907693741, 0.3309678064585504, 0},
    };
    return cases;
}

core::Tle Parse(const Case& c) {
    const auto tle = core::ParseTle(std::string(c.line1) + "\n" + c.line2 + "\n");
    EXPECT_TRUE(tle.has_value()) << c.name;
    return tle.value_or(core::Tle{});
}

TEST(InitSgp4, MatchesPythonSgp4ForEachCase) {
    constexpr double kTolerance = 1e-10;
    for (const Case& c : Cases()) {
        SCOPED_TRACE(c.name);
        const auto model = core::InitSgp4(Parse(c));
        ASSERT_TRUE(model.has_value());

        EXPECT_NEAR(model->meanMotion, c.noUnkozai, kTolerance);
        EXPECT_NEAR(model->semiMajorAxis, c.a, kTolerance);
        EXPECT_NEAR(model->con41, c.con41, kTolerance);
        EXPECT_NEAR(model->gsto, c.gsto, kTolerance);
        EXPECT_NEAR(model->eta, c.eta, kTolerance);
        EXPECT_NEAR(model->perigeeKm, c.altp * core::wgs72::kRadiusEarthKm, 1e-6);
        EXPECT_EQ(model->isSimple, c.isimp == 1);
    }
}

TEST(InitSgp4, ConvertsUnitsToRadiansAndRadiansPerMinute) {
    const auto model = core::InitSgp4(Parse(Cases()[0]));
    ASSERT_TRUE(model.has_value());

    constexpr double kPi = std::numbers::pi;
    EXPECT_NEAR(model->inclination, 51.6307 * kPi / 180.0, 1e-15);
    EXPECT_NEAR(model->raan, 190.1401 * kPi / 180.0, 1e-15);
    EXPECT_NEAR(model->argPerigee, 160.6694 * kPi / 180.0, 1e-15);
    EXPECT_NEAR(model->meanAnomaly, 199.4478 * kPi / 180.0, 1e-15);
    // 15.49188396 revolutions per day is 2*pi*15.49188396 radians per 1440 minutes.
    EXPECT_NEAR(model->meanMotionKozai, 15.49188396 * 2.0 * kPi / 1440.0, 1e-15);
    EXPECT_DOUBLE_EQ(model->eccentricity, 0.0004820);
    EXPECT_NEAR(model->bstar, 0.14267e-3, 1e-15);
}

TEST(InitSgp4, EpochIsTheJulianDateOfTheTleEpoch) {
    const auto model = core::InitSgp4(Parse(Cases()[1]));
    ASSERT_TRUE(model.has_value());
    // 2000, day 179.78495062: January 0.0 2000 is JD 2451543.5.
    EXPECT_NEAR(model->epochJd, 2451543.5 + 179.78495062, 1e-9);
}

TEST(InitSgp4, DerivedTrigTermsAreConsistent) {
    const auto model = core::InitSgp4(Parse(Cases()[1]));
    ASSERT_TRUE(model.has_value());
    EXPECT_NEAR(model->cosInclination * model->cosInclination +
                    model->sinInclination * model->sinInclination,
                1.0, 1e-15);
    EXPECT_NEAR(model->omeosq, 1.0 - 0.1859667 * 0.1859667, 1e-15);
    // con41 = 3 cos^2(i) - 1 and con42 = 1 - 5 cos^2(i).
    EXPECT_NEAR(model->con41, 3.0 * model->cosInclination * model->cosInclination - 1.0, 1e-14);
    EXPECT_NEAR(model->con42, 1.0 - 5.0 * model->cosInclination * model->cosInclination, 1e-14);
}

TEST(InitSgp4, RejectsDeepSpaceOrbits) {
    // Same TLE format as the synthetic cases above, with periods of 228 and 1436 minutes.
    const std::string period228 =
        "1 90004U 98067A   26263.14255447  .00007470  00000+0  00000-0 0  9991\n"
        "2 90004   0.0500  80.0000 0002000  90.0000  10.0000  6.30000000123454\n";
    const std::string geo =
        "1 90004U 98067A   26263.14255447  .00007470  00000+0  00000-0 0  9991\n"
        "2 90004   0.0500  80.0000 0002000  90.0000  10.0000  1.00270000123455\n";

    const auto slow = core::ParseTle(period228);
    const auto geostationary = core::ParseTle(geo);
    ASSERT_TRUE(slow.has_value());
    ASSERT_TRUE(geostationary.has_value());
    EXPECT_FALSE(core::InitSgp4(*slow).has_value());
    EXPECT_FALSE(core::InitSgp4(*geostationary).has_value());
}

TEST(InitSgp4, RejectsElementsThatAreNotABoundOrbit) {
    core::Tle tle = Parse(Cases()[0]);

    core::Tle parabolic = tle;
    parabolic.eccentricity = 1.0;
    EXPECT_FALSE(core::InitSgp4(parabolic).has_value());

    core::Tle negativeEccentricity = tle;
    negativeEccentricity.eccentricity = -0.1;
    EXPECT_FALSE(core::InitSgp4(negativeEccentricity).has_value());

    core::Tle stationary = tle;
    stationary.meanMotion = 0.0;
    EXPECT_FALSE(core::InitSgp4(stationary).has_value());
}

} // namespace
