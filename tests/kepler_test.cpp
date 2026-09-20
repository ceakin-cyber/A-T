#include "core/kepler.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;
constexpr double kTwoPi = 2.0 * std::numbers::pi;

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

// The expected values below were computed independently, by bisection on the equation, and
// agree with the worked examples in Meeus (chapter 30) and Vallado (example 2-1).

TEST(SolveKepler, MeeusExample) {
    // e = 0.1, M = 5 degrees.
    EXPECT_NEAR(core::SolveKepler(Radians(5.0), 0.1), Radians(5.554589253872315), 1e-11);
}

TEST(SolveKepler, ValladoExample) {
    // e = 0.4, M = 235.4 degrees.
    EXPECT_NEAR(core::SolveKepler(Radians(235.4), 0.4), Radians(220.51207476752214), 1e-11);
}

TEST(SolveKepler, HighEccentricity) {
    // e = 0.9, M = 90 degrees.
    EXPECT_NEAR(core::SolveKepler(Radians(90.0), 0.9), Radians(129.6841328804072), 1e-11);
}

TEST(SolveKepler, CircularOrbitHasEqualAnomalies) {
    for (double m = 0.0; m < kTwoPi; m += 0.37) {
        EXPECT_NEAR(core::SolveKepler(m, 0.0), m, 1e-15);
    }
}

TEST(SolveKepler, PerigeeAndApogeeAreFixedPoints) {
    for (const double e : {0.0, 0.1, 0.5, 0.9}) {
        EXPECT_NEAR(core::SolveKepler(0.0, e), 0.0, 1e-12) << "e=" << e;
        EXPECT_NEAR(core::SolveKepler(kPi, e), kPi, 1e-12) << "e=" << e;
    }
}

TEST(SolveKepler, SatisfiesTheEquationAcrossTheOrbit) {
    for (const double e : {0.0, 0.001, 0.01, 0.1, 0.3, 0.5, 0.7, 0.9}) {
        for (double m = 0.0; m < kTwoPi; m += 0.05) {
            const double eccentricAnomaly = core::SolveKepler(m, e);
            EXPECT_NEAR(eccentricAnomaly - e * std::sin(eccentricAnomaly), m, 1e-11)
                << "e=" << e << " M=" << m;
        }
    }
}

TEST(SolveKepler, IsSymmetricAboutPerigee) {
    // E(2*pi - M) = 2*pi - E(M).
    for (const double e : {0.2, 0.6, 0.85}) {
        for (double m = 0.1; m < kPi; m += 0.2) {
            EXPECT_NEAR(core::SolveKepler(kTwoPi - m, e), kTwoPi - core::SolveKepler(m, e), 1e-11)
                << "e=" << e << " M=" << m;
        }
    }
}

TEST(SolveKepler, EccentricAnomalyIsAheadOfMeanAnomalyBeforeApogee) {
    // For 0 < M < pi the true position runs ahead of the mean position: E > M.
    EXPECT_GT(core::SolveKepler(1.0, 0.3), 1.0);
    EXPECT_LT(core::SolveKepler(kTwoPi - 1.0, 0.3), kTwoPi - 1.0);
}

TEST(SolveKeplerSgp4, ReducesToTheClassicalFormWhenTheArgumentOfPerigeeIsZero) {
    EXPECT_DOUBLE_EQ(core::SolveKeplerSgp4(1.234, 0.3, 0.0), core::SolveKepler(1.234, 0.3));
}

TEST(SolveKeplerSgp4, SatisfiesTheGeneralEquation) {
    // Sweep argument of perigee, eccentricity and mean longitude.
    for (const double e : {0.0005, 0.02, 0.2, 0.6, 0.85}) {
        for (double w = 0.0; w < kTwoPi; w += 0.7) {
            const double axnl = e * std::cos(w);
            const double aynl = e * std::sin(w);
            for (double u = 0.0; u < kTwoPi; u += 0.31) {
                const double x = core::SolveKeplerSgp4(u, axnl, aynl);
                const double residual = x - axnl * std::sin(x) + aynl * std::cos(x) - u;
                EXPECT_NEAR(residual, 0.0, 1e-11) << "e=" << e << " w=" << w << " u=" << u;
            }
        }
    }
}

TEST(SolveKeplerSgp4, MatchesTheClassicalSolutionOnceTheArgumentOfPerigeeIsRemoved) {
    // With u = M + w, the eccentric longitude E' equals E + w.
    const double e = 0.35;
    const double w = 1.1;
    const double m = 2.0;
    const double longitude = core::SolveKeplerSgp4(m + w, e * std::cos(w), e * std::sin(w));
    EXPECT_NEAR(longitude - w, core::SolveKepler(m, e), 1e-11);
}

} // namespace
