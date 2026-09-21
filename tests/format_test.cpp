#include "app/format.h"

#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

TEST(FormatLatitude, NorthAndSouth) {
    EXPECT_EQ(app::FormatLatitude(Radians(40.8729)), "40.8729 N");
    EXPECT_EQ(app::FormatLatitude(Radians(-33.8688)), "33.8688 S");
}

TEST(FormatLatitude, ZeroAndThePoles) {
    EXPECT_EQ(app::FormatLatitude(0.0), "0.0000 N");
    EXPECT_EQ(app::FormatLatitude(kPi / 2.0), "90.0000 N");
    EXPECT_EQ(app::FormatLatitude(-kPi / 2.0), "90.0000 S");
}

TEST(FormatLongitude, EastAndWest) {
    EXPECT_EQ(app::FormatLongitude(Radians(151.2093)), "151.2093 E");
    EXPECT_EQ(app::FormatLongitude(Radians(-38.1415)), "38.1415 W");
}

TEST(FormatLongitude, ZeroAndTheAntimeridian) {
    EXPECT_EQ(app::FormatLongitude(0.0), "0.0000 E");
    EXPECT_EQ(app::FormatLongitude(kPi), "180.0000 E");
    EXPECT_EQ(app::FormatLongitude(-kPi), "180.0000 W");
}

TEST(FormatAngle, RoundsToFourDecimals) {
    EXPECT_EQ(app::FormatLatitude(Radians(12.34564)), "12.3456 N");
    EXPECT_EQ(app::FormatLatitude(Radians(12.34566)), "12.3457 N");
}

TEST(FormatAngle, NegativeValuesNeverShowAMinusSign) {
    EXPECT_EQ(app::FormatLongitude(Radians(-0.5)).find('-'), std::string::npos);
    EXPECT_EQ(app::FormatLatitude(Radians(-0.5)).find('-'), std::string::npos);
}

TEST(FormatAltitudeKm, TwoDecimalsAndUnit) {
    EXPECT_EQ(app::FormatAltitudeKm(420.157), "420.16 KM");
    EXPECT_EQ(app::FormatAltitudeKm(35786.0), "35786.00 KM");
    EXPECT_EQ(app::FormatAltitudeKm(0.0), "0.00 KM");
}

TEST(FormatSpeedKmPerSec, TwoDecimalsAndUnit) {
    EXPECT_EQ(app::FormatSpeedKmPerSec(7.6602), "7.66 KM/S");
    EXPECT_EQ(app::FormatSpeedKmPerSec(3.0747), "3.07 KM/S");
}

} // namespace
