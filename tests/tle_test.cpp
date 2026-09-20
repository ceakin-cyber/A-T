#include "core/tle.h"

#include <gtest/gtest.h>

namespace {

// Real TLE published with Celestrak's format documentation (ISS, 2008).
const char* const kIss2008 =
    "ISS (ZARYA)\n"
    "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n"
    "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n";

// ISS elements fetched from Celestrak on 2026-09-20, with a trailing space on the name line.
const char* const kIss2026 =
    "ISS (ZARYA)             \r\n"
    "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\r\n"
    "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\r\n";

// Vallado's SGP4 verification satellite 00005, epoch in 2000.
const char* const kSat00005 =
    "1 00005U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753\n"
    "2 00005  34.2682 348.7242 1859667 331.7664  19.3264 10.82419157413667\n";

TEST(ParseTle, ReadsAllFieldsFromIss2008) {
    const auto tle = core::ParseTle(kIss2008);
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->name, "ISS (ZARYA)");
    EXPECT_EQ(tle->catalogNumber, 25544);
    EXPECT_EQ(tle->epochYear, 2008);
    EXPECT_DOUBLE_EQ(tle->epochDay, 264.51782528);
    EXPECT_DOUBLE_EQ(tle->inclination, 51.6416);
    EXPECT_DOUBLE_EQ(tle->raan, 247.4627);
    EXPECT_DOUBLE_EQ(tle->eccentricity, 0.0006703);
    EXPECT_DOUBLE_EQ(tle->argPerigee, 130.5360);
    EXPECT_DOUBLE_EQ(tle->meanAnomaly, 325.0288);
    EXPECT_DOUBLE_EQ(tle->meanMotion, 15.72125391);
    EXPECT_NEAR(tle->bstar, -0.11606e-4, 1e-12);
}

TEST(ParseTle, ReadsCurrentIssWithCrlfAndPaddedName) {
    const auto tle = core::ParseTle(kIss2026);
    ASSERT_TRUE(tle.has_value());
    EXPECT_EQ(tle->name, "ISS (ZARYA)");
    EXPECT_EQ(tle->epochYear, 2026);
    EXPECT_DOUBLE_EQ(tle->epochDay, 263.14255447);
    EXPECT_DOUBLE_EQ(tle->eccentricity, 0.0004820);
    EXPECT_NEAR(tle->bstar, 0.14267e-3, 1e-12);
}

TEST(ParseTle, ReadsTwoLineInputWithoutNameAndOldEpochYear) {
    const auto tle = core::ParseTle(kSat00005);
    ASSERT_TRUE(tle.has_value());
    EXPECT_TRUE(tle->name.empty());
    EXPECT_EQ(tle->catalogNumber, 5);
    EXPECT_EQ(tle->epochYear, 2000);
    EXPECT_DOUBLE_EQ(tle->epochDay, 179.78495062);
    EXPECT_DOUBLE_EQ(tle->eccentricity, 0.1859667);
    EXPECT_NEAR(tle->bstar, 0.28098e-4, 1e-12);
    EXPECT_DOUBLE_EQ(tle->meanMotion, 10.82419157);
}

TEST(ParseTle, RejectsEmptyInput) {
    EXPECT_FALSE(core::ParseTle("").has_value());
}

TEST(ParseTle, RejectsCelestrakNoDataMessage) {
    EXPECT_FALSE(core::ParseTle("No GP data found").has_value());
}

TEST(ParseTle, RejectsBadChecksum) {
    std::string text = kIss2008;
    text[text.find("2927") + 3] = '8'; // line 1 checksum 7 -> 8
    EXPECT_FALSE(core::ParseTle(text).has_value());
}

TEST(ParseTle, RejectsTruncatedLine) {
    const std::string text =
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  292\n"
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n";
    EXPECT_FALSE(core::ParseTle(text).has_value());
}

TEST(ParseTle, RejectsSwappedLines) {
    const std::string text =
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n"
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n";
    EXPECT_FALSE(core::ParseTle(text).has_value());
}

} // namespace
