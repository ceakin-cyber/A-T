#include "app/moon_disk.h"
#include "core/lunar.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kNorth = 51.5 * std::numbers::pi / 180.0;
constexpr double kSouth = -33.9 * std::numbers::pi / 180.0;

TEST(MoonLitOnRight, WaxingIsLitOnTheRightFromTheNorth) {
    EXPECT_TRUE(app::MoonLitOnRight(3.0, kNorth));
    EXPECT_TRUE(app::MoonLitOnRight(10.0, kNorth));
}

TEST(MoonLitOnRight, WaningIsLitOnTheLeftFromTheNorth) {
    EXPECT_FALSE(app::MoonLitOnRight(18.0, kNorth));
    EXPECT_FALSE(app::MoonLitOnRight(26.0, kNorth));
}

TEST(MoonLitOnRight, TheSouthernHemisphereSeesItTheOtherWayRound) {
    EXPECT_FALSE(app::MoonLitOnRight(3.0, kSouth));
    EXPECT_TRUE(app::MoonLitOnRight(26.0, kSouth));
}

TEST(MoonLitOnRight, SwitchesSidesAtFullMoon) {
    const double half = core::SynodicMonthDays() / 2.0;
    EXPECT_TRUE(app::MoonLitOnRight(half - 0.01, kNorth));
    EXPECT_FALSE(app::MoonLitOnRight(half + 0.01, kNorth));
}

void ExpectSpan(app::LitSpan span, double left, double right) {
    EXPECT_NEAR(span.left, left, 1e-12);
    EXPECT_NEAR(span.right, right, 1e-12);
}

TEST(MoonLitSpan, NothingLitAtNewMoon) {
    const app::LitSpan span = app::MoonLitSpan(0.0, 0.0, true);
    EXPECT_NEAR(span.right - span.left, 0.0, 1e-12);
}

TEST(MoonLitSpan, TheWholeSliceLitAtFullMoon) {
    ExpectSpan(app::MoonLitSpan(0.0, 1.0, true), -1.0, 1.0);
    ExpectSpan(app::MoonLitSpan(0.0, 1.0, false), -1.0, 1.0);
}

TEST(MoonLitSpan, HalfLitFromTheMiddleAtAQuarterMoon) {
    ExpectSpan(app::MoonLitSpan(0.0, 0.5, true), 0.0, 1.0);
    ExpectSpan(app::MoonLitSpan(0.0, 0.5, false), -1.0, 0.0);
}

TEST(MoonLitSpan, ACrescentHugsTheLitEdge) {
    // A quarter lit: the terminator crosses the middle slice halfway out toward the lit edge.
    ExpectSpan(app::MoonLitSpan(0.0, 0.25, true), 0.5, 1.0);
    ExpectSpan(app::MoonLitSpan(0.0, 0.25, false), -1.0, -0.5);
}

TEST(MoonLitSpan, AGibbousMoonReachesPastTheMiddle) {
    ExpectSpan(app::MoonLitSpan(0.0, 0.75, true), -0.5, 1.0);
}

TEST(MoonLitSpan, NarrowsTowardThePoles) {
    // Up at y = 0.6 the disk is only 0.8 across each side of the middle, and the terminator
    // scales with it.
    ExpectSpan(app::MoonLitSpan(0.6, 0.25, true), 0.4, 0.8);
    ExpectSpan(app::MoonLitSpan(-0.6, 0.25, true), 0.4, 0.8);
    const app::LitSpan pole = app::MoonLitSpan(1.0, 0.25, true);
    EXPECT_NEAR(pole.left, 0.0, 1e-12);
    EXPECT_NEAR(pole.right, 0.0, 1e-12);
}

// Summing the lit slices across the whole disk should give back the illuminated fraction.
TEST(MoonLitSpan, LitAreaMatchesTheIlluminatedFraction) {
    for (const double fraction : {0.1, 0.3, 0.5, 0.8, 0.95}) {
        constexpr int kSlices = 20000;
        double lit = 0.0;
        double disk = 0.0;
        for (int i = 0; i < kSlices; ++i) {
            const double y = -1.0 + (i + 0.5) * 2.0 / kSlices;
            const app::LitSpan span = app::MoonLitSpan(y, fraction, false);
            lit += span.right - span.left;
            disk += 2.0 * std::sqrt(1.0 - y * y);
        }
        EXPECT_NEAR(lit / disk, fraction, 1e-6);
    }
}

TEST(MoonLitSpan, ClampsAFractionOutOfRange) {
    ExpectSpan(app::MoonLitSpan(0.0, 1.5, true), -1.0, 1.0);
    const app::LitSpan none = app::MoonLitSpan(0.0, -0.5, true);
    EXPECT_NEAR(none.right - none.left, 0.0, 1e-12);
}

} // namespace
