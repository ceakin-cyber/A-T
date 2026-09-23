#include "core/constellation.h"

#include <gtest/gtest.h>
#include <numbers>

namespace {

double Radians(double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

double Degrees(double radians) {
    return radians * 180.0 / std::numbers::pi;
}

core::Star MakeStar(int hip, double raDeg, double decDeg) {
    core::Star star;
    star.id = hip;
    star.hip = hip;
    star.raRad = Radians(raDeg);
    star.decRad = Radians(decDeg);
    star.magnitude = 3.0;
    return star;
}

TEST(ParseConstellationLines, ReadsConHip1AndHip2) {
    const std::string text = "con,hip1,hip2\nOri,100,200\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].constellation, "Ori");
    EXPECT_EQ(lines[0].hip1, 100);
    EXPECT_EQ(lines[0].hip2, 200);
}

TEST(ParseConstellationLines, ReadsMultipleRowsInOrder) {
    const std::string text = "con,hip1,hip2\nOri,100,200\nOri,200,300\nUMa,400,500\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0].hip2, 200);
    EXPECT_EQ(lines[1].hip1, 200);
    EXPECT_EQ(lines[2].constellation, "UMa");
}

TEST(ParseConstellationLines, ColumnsAreFoundByNameNotPosition) {
    const std::string text = "hip2,con,hip1\n200,Ori,100\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].constellation, "Ori");
    EXPECT_EQ(lines[0].hip1, 100);
    EXPECT_EQ(lines[0].hip2, 200);
}

TEST(ParseConstellationLines, SkipsARowWithAnInvalidHipAndKeepsReadingTheRest) {
    const std::string text = "con,hip1,hip2\nOri,not-a-number,200\nOri,300,400\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].hip1, 300);
}

TEST(ParseConstellationLines, SkipsARowWithABlankConstellation) {
    const std::string text = "con,hip1,hip2\n,100,200\nOri,300,400\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].hip1, 300);
}

TEST(ParseConstellationLines, SkipsARowThatIsTooShort) {
    const std::string text = "con,hip1,hip2\nOri,100\nUMa,300,400\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].constellation, "UMa");
}

TEST(ParseConstellationLines, SkipsBlankLines) {
    const std::string text = "con,hip1,hip2\nOri,100,200\n\nUMa,300,400\n";
    const auto lines = core::ParseConstellationLines(text);
    EXPECT_EQ(lines.size(), 2U);
}

TEST(ParseConstellationLines, EmptyTextGivesAnEmptyList) {
    EXPECT_TRUE(core::ParseConstellationLines("").empty());
}

TEST(ParseConstellationLines, MissingARequiredColumnGivesAnEmptyList) {
    const std::string noHip2 = "con,hip1\nOri,100\n";
    EXPECT_TRUE(core::ParseConstellationLines(noHip2).empty());
}

TEST(ParseConstellationLines, HandlesCrlfLineEndings) {
    const std::string text = "con,hip1,hip2\r\nOri,100,200\r\n";
    const auto lines = core::ParseConstellationLines(text);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].hip1, 100);
}

TEST(LoadConstellationLines, ReturnsAnEmptyListWhenTheFileDoesNotExist) {
    EXPECT_TRUE(core::LoadConstellationLines("/nonexistent/path/lines.csv").empty());
}

// End-to-end against the real, committed data.
TEST(LoadConstellationLines, ReadsTheRealCommittedData) {
    const auto lines = core::LoadConstellationLines("assets/stars/constellation_lines.csv");
    EXPECT_EQ(lines.size(), 695U);
    for (const core::ConstellationLine& line : lines) {
        EXPECT_FALSE(line.constellation.empty());
        EXPECT_GT(line.hip1, 0);
        EXPECT_GT(line.hip2, 0);
    }
}

// The endpoints below have independently-verified altitudes, the same geometry used in
// tests/star_catalog_test.cpp's VisibleStars tests: a star above the horizon (dec 60, seen from
// lat 40, at 70 degrees altitude) and one below it (dec -80).

TEST(VisibleConstellationLines, KeepsASegmentWithBothEndpointsAboveTheHorizon) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, 60.0), MakeStar(2, 10.0, 60.0)};
    const std::vector<core::ConstellationLine> segments = {{"Test", 1, 2}};
    const auto visible = core::VisibleConstellationLines(stars, segments, Radians(40.0), 0.0);
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_NEAR(Degrees(visible[0].a.altitudeRad), 70.0, 1e-9);
}

TEST(VisibleConstellationLines, DropsASegmentWithEitherEndpointBelowTheHorizon) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, 60.0), MakeStar(2, 0.0, -80.0)};
    const std::vector<core::ConstellationLine> segments = {{"Test", 1, 2}};
    EXPECT_TRUE(core::VisibleConstellationLines(stars, segments, Radians(40.0), 0.0).empty());
}

TEST(VisibleConstellationLines, DropsASegmentWithAnEndpointMissingFromTheStarList) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, 60.0)};
    const std::vector<core::ConstellationLine> segments = {{"Test", 1, 999}};
    EXPECT_TRUE(core::VisibleConstellationLines(stars, segments, Radians(40.0), 0.0).empty());
}

TEST(VisibleConstellationLines, AStarWithHipZeroNeverMatchesAnEndpoint) {
    // hip 0 marks "no Hipparcos number" (see Star::hip); it must never be treated as a valid
    // match, even if a malformed line file somehow asked for endpoint 0.
    core::Star noHip = MakeStar(1, 0.0, 60.0);
    noHip.hip = 0;
    const std::vector<core::Star> stars = {noHip};
    const std::vector<core::ConstellationLine> segments = {{"Test", 0, 0}};
    EXPECT_TRUE(core::VisibleConstellationLines(stars, segments, Radians(40.0), 0.0).empty());
}

TEST(VisibleConstellationLines, KeepsMultipleSegmentsAndDropsOnlyTheInvisibleOne) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, 60.0), MakeStar(2, 10.0, 60.0),
                                           MakeStar(3, 0.0, -80.0)};
    const std::vector<core::ConstellationLine> segments = {{"A", 1, 2}, {"B", 1, 3}};
    const auto visible = core::VisibleConstellationLines(stars, segments, Radians(40.0), 0.0);
    EXPECT_EQ(visible.size(), 1U);
}

TEST(VisibleConstellationLines, EmptyInputsGiveEmptyOutput) {
    EXPECT_TRUE(core::VisibleConstellationLines({}, {}, Radians(40.0), 0.0).empty());
}

TEST(VisibleConstellationLines, ThePositionsMatchCallingTheTransformDirectly) {
    const std::vector<core::Star> stars = {MakeStar(5, 0.0, 60.0), MakeStar(6, 10.0, 60.0)};
    const std::vector<core::ConstellationLine> segments = {{"Test", 5, 6}};
    const auto visible = core::VisibleConstellationLines(stars, segments, Radians(40.0), 0.0);
    ASSERT_EQ(visible.size(), 1U);
    const auto expectedA =
        core::EquatorialToHorizontal(Radians(0.0), Radians(60.0), Radians(40.0), 0.0);
    const auto expectedB =
        core::EquatorialToHorizontal(Radians(10.0), Radians(60.0), Radians(40.0), 0.0);
    EXPECT_DOUBLE_EQ(visible[0].a.altitudeRad, expectedA.altitudeRad);
    EXPECT_DOUBLE_EQ(visible[0].a.azimuthRad, expectedA.azimuthRad);
    EXPECT_DOUBLE_EQ(visible[0].b.altitudeRad, expectedB.altitudeRad);
    EXPECT_DOUBLE_EQ(visible[0].b.azimuthRad, expectedB.azimuthRad);
}

} // namespace
