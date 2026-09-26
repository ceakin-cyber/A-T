#include "core/land.h"

#include <gtest/gtest.h>
#include <numbers>

namespace {

double Degrees(double radians) {
    return radians * 180.0 / std::numbers::pi;
}

TEST(ParseLandOutlines, ReadsOneRingPerLine) {
    const auto rings = core::ParseLandOutlines("# a comment\n"
                                               "\n"
                                               "0,0 10,0 10,10\n"
                                               "-170.5,-20.25 -160,-20 -165,-30 -170,-25\n");
    ASSERT_EQ(rings.size(), 2U);
    ASSERT_EQ(rings[0].points.size(), 3U);
    ASSERT_EQ(rings[1].points.size(), 4U);
    EXPECT_NEAR(Degrees(rings[1].points[0].longitude), -170.5, 1e-9);
    EXPECT_NEAR(Degrees(rings[1].points[0].latitude), -20.25, 1e-9);
    EXPECT_DOUBLE_EQ(rings[1].points[0].altitudeKm, 0.0);
    EXPECT_FALSE(rings[0].hole);
}

TEST(ParseLandOutlines, ReadsAHole) {
    const auto rings = core::ParseLandOutlines("hole 0,0 10,0 10,10\n");
    ASSERT_EQ(rings.size(), 1U);
    EXPECT_TRUE(rings[0].hole);
    EXPECT_EQ(rings[0].points.size(), 3U);
}

TEST(ParseLandOutlines, SkipsARingWithAnUnreadablePointButKeepsTheRest) {
    const auto rings = core::ParseLandOutlines("0,0 10,x 10,10\n"
                                               "0,0 10,0 10,10\n");
    EXPECT_EQ(rings.size(), 1U);
}

TEST(ParseLandOutlines, SkipsARingWithAPointOutOfRange) {
    EXPECT_TRUE(core::ParseLandOutlines("0,0 190,0 10,10\n").empty());
    EXPECT_TRUE(core::ParseLandOutlines("0,0 10,95 10,10\n").empty());
}

TEST(ParseLandOutlines, SkipsARingTooShortToEncloseAnything) {
    EXPECT_TRUE(core::ParseLandOutlines("0,0 10,0\n").empty());
}

TEST(LoadLandOutlines, ReadsTheRealCommittedOutlines) {
    const auto rings = core::LoadLandOutlines("assets/world/land_110m.txt");
    EXPECT_EQ(rings.size(), 128U); // see assets/world/SOURCE.md
    std::size_t points = 0;
    int holes = 0;
    for (const core::LandRing& ring : rings) {
        points += ring.points.size();
        holes += ring.hole ? 1 : 0;
    }
    EXPECT_EQ(points, 5015U);
    EXPECT_EQ(holes, 1); // the Caspian Sea
}

TEST(LoadLandOutlines, MissingFileGivesNoOutlines) {
    EXPECT_TRUE(core::LoadLandOutlines("/nonexistent/land.txt").empty());
}

} // namespace
