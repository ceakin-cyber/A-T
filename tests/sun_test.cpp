#include "core/sun.h"
#include "core/time.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

double Degrees(double radians) {
    return radians * 180.0 / std::numbers::pi;
}

double Jd(int year, int month, int day, int hour, int minute) {
    return core::JulianDate({year, month, day, hour, minute, 0.0});
}

TEST(SunDirectionEci, IsAUnitVector) {
    const core::Vec3 sun = core::SunDirectionEci(Jd(2026, 9, 25, 12, 0));
    EXPECT_NEAR(std::sqrt(sun.x * sun.x + sun.y * sun.y + sun.z * sun.z), 1.0, 1e-12);
}

// The subsolar latitude is the Sun's declination: 0 at the equinoxes and the obliquity of the
// ecliptic (about 23.44 degrees) at the solstices. Times are the published instants of each
// (US Naval Observatory, 2024).
TEST(SubsolarPoint, LatitudeIsZeroAtTheMarchEquinox) {
    EXPECT_NEAR(Degrees(core::SubsolarPoint(Jd(2024, 3, 20, 3, 6)).latitude), 0.0, 0.02);
}

TEST(SubsolarPoint, LatitudeIsTheObliquityAtTheJuneSolstice) {
    EXPECT_NEAR(Degrees(core::SubsolarPoint(Jd(2024, 6, 20, 20, 51)).latitude), 23.44, 0.02);
}

TEST(SubsolarPoint, LatitudeIsMinusTheObliquityAtTheDecemberSolstice) {
    EXPECT_NEAR(Degrees(core::SubsolarPoint(Jd(2024, 12, 21, 9, 21)).latitude), -23.44, 0.02);
}

// At 12:00 UTC the Sun would be over Greenwich if it kept perfect time; the equation of time
// says how far off it runs. Around 3 November it is about 16.4 minutes fast (so already 4.1
// degrees west of Greenwich at noon), and around 11 February about 14.2 minutes slow (3.5 degrees
// still to the east).
TEST(SubsolarPoint, LongitudeFollowsTheEquationOfTime) {
    EXPECT_NEAR(Degrees(core::SubsolarPoint(Jd(2024, 11, 3, 12, 0)).longitude), -4.1, 0.1);
    EXPECT_NEAR(Degrees(core::SubsolarPoint(Jd(2024, 2, 11, 12, 0)).longitude), 3.55, 0.1);
}

TEST(SubsolarPoint, MovesWestFifteenDegreesAnHour) {
    const double noon = Degrees(core::SubsolarPoint(Jd(2026, 9, 25, 12, 0)).longitude);
    const double onePm = Degrees(core::SubsolarPoint(Jd(2026, 9, 25, 13, 0)).longitude);
    EXPECT_NEAR(noon - onePm, 15.0, 0.01);
}

const core::Vec3 kSunAlongX = {1.0, 0.0, 0.0};

TEST(IsSunlit, OnTheDaySideIsLit) {
    EXPECT_TRUE(core::IsSunlit({7000.0, 0.0, 0.0}, kSunAlongX));
    EXPECT_TRUE(core::IsSunlit({0.0, 7000.0, 0.0}, kSunAlongX)); // over the day/night line
}

TEST(IsSunlit, DirectlyBehindTheEarthIsInShadow) {
    EXPECT_FALSE(core::IsSunlit({-7000.0, 0.0, 0.0}, kSunAlongX));
    EXPECT_FALSE(core::IsSunlit({-7000.0, 6000.0, 0.0}, kSunAlongX)); // inside the shadow's edge
}

TEST(IsSunlit, BehindTheEarthButOutsideItsShadowIsLit) {
    EXPECT_TRUE(core::IsSunlit({-7000.0, 6500.0, 0.0}, kSunAlongX)); // just outside its edge
    EXPECT_TRUE(core::IsSunlit({-7000.0, 0.0, 7000.0}, kSunAlongX));
}

} // namespace
