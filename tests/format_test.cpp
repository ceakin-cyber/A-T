#include "app/format.h"

#include <chrono>
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

TEST(FormatElevation, OneDecimalAndUnit) {
    EXPECT_EQ(app::FormatElevation(Radians(33.24)), "33.2 DEG");
    EXPECT_EQ(app::FormatElevation(Radians(0.19)), "0.2 DEG");
    EXPECT_EQ(app::FormatElevation(kPi / 2.0), "90.0 DEG");
}

TEST(FormatIlluminatedFraction, WholeNumberPercent) {
    EXPECT_EQ(app::FormatIlluminatedFraction(0.783), "78%");
    EXPECT_EQ(app::FormatIlluminatedFraction(0.0), "0%");
    EXPECT_EQ(app::FormatIlluminatedFraction(1.0), "100%");
    EXPECT_EQ(app::FormatIlluminatedFraction(0.006), "1%"); // rounds, not truncates
}

TEST(FormatUtcTime, MonthDayAndTime) {
    // 2026-09-21 14:32:10 UTC is 1790001130 seconds after the Unix epoch.
    const std::chrono::system_clock::time_point t{std::chrono::seconds(1790001130)};
    EXPECT_EQ(app::FormatUtcTime(t), "09-21 14:32:10");
}

TEST(FormatUtcTime, PadsSingleDigitsAndIgnoresFractionalSeconds) {
    // 2026-01-05 03:04:05.9 UTC.
    const std::chrono::system_clock::time_point t{std::chrono::milliseconds(1767582245900)};
    EXPECT_EQ(app::FormatUtcTime(t), "01-05 03:04:05");
}

TEST(FormatUtcTime, TheUnixEpoch) {
    EXPECT_EQ(app::FormatUtcTime(std::chrono::system_clock::time_point{}), "01-01 00:00:00");
}

TEST(FormatCountdown, SecondsOnly) {
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(0)), "0S");
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(45)), "45S");
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(59)), "59S");
}

TEST(FormatCountdown, MinutesAndPaddedSeconds) {
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(60)), "1M 00S");
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(12 * 60 + 5)), "12M 05S");
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(59 * 60 + 59)), "59M 59S");
}

TEST(FormatCountdown, HoursAndMinutes) {
    EXPECT_EQ(app::FormatCountdown(std::chrono::hours(1)), "1H 0M");
    EXPECT_EQ(app::FormatCountdown(std::chrono::hours(1) + std::chrono::minutes(23) +
                                   std::chrono::seconds(50)),
              "1H 23M");
}

TEST(FormatCountdown, DaysAndHours) {
    EXPECT_EQ(app::FormatCountdown(std::chrono::hours(24)), "1D 0H");
    EXPECT_EQ(app::FormatCountdown(std::chrono::hours(51)), "2D 3H");
}

TEST(FormatCountdown, NegativeShowsAsZero) {
    EXPECT_EQ(app::FormatCountdown(std::chrono::seconds(-30)), "0S");
}

TEST(FormatCountdown, AcceptsFractionalSeconds) {
    EXPECT_EQ(app::FormatCountdown(std::chrono::duration<double>(59.9)), "59S");
}

TEST(FormatNodeMode, NamesEachTleSource) {
    EXPECT_EQ(app::FormatNodeMode(net::TleSource::Network), "LIVE");
    EXPECT_EQ(app::FormatNodeMode(net::TleSource::FreshCache), "CACHED");
    EXPECT_EQ(app::FormatNodeMode(net::TleSource::StaleCache), "LOW-VISIBILITY");
}

} // namespace
