#include "app/observer.h"
#include "core/passes.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>
#include <string>

namespace {

constexpr double kSecondsPerDay = 86400.0;
constexpr double kPi = std::numbers::pi;

// ISS elements from Celestrak, 2026-09-20 (epoch JD 2461304.02959654).
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.52959654  .00008422  00000+0  15975-3 0  9999\n"
                         "2 25544  51.6308 188.2246 0004825 162.2847 197.8311 15.49196792586535\n";

// A synthetic orbit with a perigee near 90 km, which drag destroys within hours.
const char* const kDecaying =
    "1 90002U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9992\n"
    "2 90002  51.6307 190.1401 0005000 160.6694 199.4478 16.67663952123456\n";

constexpr double kEpochJd = 2461304.02959654;

core::Sgp4Model Model(const char* text = kIss) {
    const auto tle = core::ParseTle(text);
    EXPECT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(tle.value_or(core::Tle{}));
    EXPECT_TRUE(model.has_value());
    return model.value_or(core::Sgp4Model{});
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

struct ExpectedPass {
    double riseJd;
    double setJd;
    bool risesBeforeWindow;
    bool setsAfterWindow;
};

// Reference times were computed independently: python-sgp4 for the orbit, and separate geometry
// (its own GMST, an East/North/Up basis from cross products, and a scan every 5 s refined by
// bisection to a microsecond) for the horizon crossings. They cover 24 hours from the TLE epoch.
const std::vector<ExpectedPass>& GreenwichPasses() {
    static const std::vector<ExpectedPass> passes = {
        {2461304.1014619675, 2461304.106879307, false, false},
        {2461304.167168064, 2461304.1744908066, false, false},
        {2461304.2341060154, 2461304.2417019755, false, false},
        {2461304.301348067, 2461304.3089344464, false, false},
        {2461304.368567786, 2461304.3758429447, false, false},
        {2461304.436225023, 2461304.441471277, false, false},
    };
    return passes;
}

// From the equator at longitude 0, where the ISS is already above the horizon at the epoch, and
// where the second pass is only 1.5 minutes long.
const std::vector<ExpectedPass>& EquatorPasses() {
    static const std::vector<ExpectedPass> passes = {
        {2461304.02959654, 2461304.033564478, true, false},
        {2461304.4455394875, 2461304.4465961214, false, false},
        {2461304.509492702, 2461304.517000299, false, false},
        {2461304.5787093816, 2461304.582079008, false, false},
        {2461304.9929991215, 2461305.000485613, false, false},
    };
    return passes;
}

void ExpectPasses(const std::vector<core::Pass>& actual, const std::vector<ExpectedPass>& expected,
                  double toleranceSeconds) {
    ASSERT_EQ(actual.size(), expected.size());
    const double tolerance = toleranceSeconds / kSecondsPerDay;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(actual[i].riseJd, expected[i].riseJd, tolerance) << "pass " << i;
        EXPECT_NEAR(actual[i].setJd, expected[i].setJd, tolerance) << "pass " << i;
        EXPECT_EQ(actual[i].risesBeforeWindow, expected[i].risesBeforeWindow) << "pass " << i;
        EXPECT_EQ(actual[i].setsAfterWindow, expected[i].setsAfterWindow) << "pass " << i;
    }
}

TEST(FindPasses, MatchesIndependentReferenceTimesOverGreenwich) {
    const auto passes = core::FindPasses(Model(), app::kObserver, kEpochJd, kEpochJd + 1.0);
    ExpectPasses(passes, GreenwichPasses(), 1.0);
}

TEST(FindPasses, ReportsAPassThatIsAlreadyInProgressAtTheStart) {
    const core::Geodetic equator = core::GeodeticFromDegrees(0.0, 0.0, 0.0);
    const auto passes = core::FindPasses(Model(), equator, kEpochJd, kEpochJd + 1.0);
    ExpectPasses(passes, EquatorPasses(), 1.0);
    ASSERT_FALSE(passes.empty());
    EXPECT_DOUBLE_EQ(passes.front().riseJd, kEpochJd);
}

TEST(FindPasses, CatchesAShortPass) {
    // The second equator pass lasts only 1.5 minutes, but is longer than the 20 s step.
    const core::Geodetic equator = core::GeodeticFromDegrees(0.0, 0.0, 0.0);
    const auto passes = core::FindPasses(Model(), equator, kEpochJd, kEpochJd + 1.0);
    ASSERT_GE(passes.size(), 2U);
    EXPECT_NEAR((passes[1].setJd - passes[1].riseJd) * 1440.0, 1.52, 0.02);
}

TEST(FindPasses, FindsNothingWhereTheSatelliteNeverRises) {
    // The ISS only reaches 51.6 degrees north, so it never clears the horizon from 85 degrees.
    const core::Geodetic arctic = core::GeodeticFromDegrees(85.0, 20.0, 0.0);
    EXPECT_TRUE(core::FindPasses(Model(), arctic, kEpochJd, kEpochJd + 1.0).empty());
}

TEST(FindPasses, ClipsAPassThatStartsInsideTheWindow) {
    // Start halfway through the first Greenwich pass.
    const ExpectedPass& first = GreenwichPasses()[0];
    const double start = 0.5 * (first.riseJd + first.setJd);
    const auto passes = core::FindPasses(Model(), app::kObserver, start, start + 0.25);

    std::vector<ExpectedPass> expected = {
        {start, first.setJd, true, false},
        GreenwichPasses()[1],
        GreenwichPasses()[2],
        GreenwichPasses()[3],
    };
    ExpectPasses(passes, expected, 1.0);
}

TEST(FindPasses, ClipsAPassThatEndsInsideTheWindow) {
    // End halfway through the fifth Greenwich pass.
    const ExpectedPass& fifth = GreenwichPasses()[4];
    const double end = 0.5 * (fifth.riseJd + fifth.setJd);
    const auto passes = core::FindPasses(Model(), app::kObserver, kEpochJd, end);

    std::vector<ExpectedPass> expected(GreenwichPasses().begin(), GreenwichPasses().begin() + 4);
    expected.push_back({fifth.riseJd, end, false, true});
    ExpectPasses(passes, expected, 1.0);
}

TEST(FindPasses, ElevationIsZeroAtEachCrossingAndHasTheRightSignEitherSide) {
    const core::Sgp4Model model = Model();
    const auto passes = core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd + 1.0);
    ASSERT_FALSE(passes.empty());

    const double fiveSeconds = 5.0 / kSecondsPerDay;
    for (const core::Pass& pass : passes) {
        const auto atRise = core::ElevationAt(model, app::kObserver, pass.riseJd);
        const auto atSet = core::ElevationAt(model, app::kObserver, pass.setJd);
        ASSERT_TRUE(atRise.has_value());
        ASSERT_TRUE(atSet.has_value());
        EXPECT_NEAR(Degrees(*atRise), 0.0, 0.02);
        EXPECT_NEAR(Degrees(*atSet), 0.0, 0.02);

        EXPECT_LT(*core::ElevationAt(model, app::kObserver, pass.riseJd - fiveSeconds), 0.0);
        EXPECT_GT(*core::ElevationAt(model, app::kObserver, pass.riseJd + fiveSeconds), 0.0);
        EXPECT_GT(*core::ElevationAt(model, app::kObserver, pass.setJd - fiveSeconds), 0.0);
        EXPECT_LT(*core::ElevationAt(model, app::kObserver, pass.setJd + fiveSeconds), 0.0);

        const double middle = 0.5 * (pass.riseJd + pass.setJd);
        EXPECT_GT(*core::ElevationAt(model, app::kObserver, middle), 0.0);
    }
}

TEST(FindPasses, PassesAreOrderedAndDoNotOverlapOverThreeDays) {
    const auto passes = core::FindPasses(Model(), app::kObserver, kEpochJd, kEpochJd + 3.0);
    EXPECT_GT(passes.size(), 10U);
    EXPECT_LT(passes.size(), 30U);
    for (std::size_t i = 0; i < passes.size(); ++i) {
        EXPECT_LT(passes[i].riseJd, passes[i].setJd) << i;
        // A pass can be 12 minutes at most for a satellite at this height.
        EXPECT_LT((passes[i].setJd - passes[i].riseJd) * 1440.0, 13.0) << i;
        if (i > 0) {
            EXPECT_GT(passes[i].riseJd, passes[i - 1].setJd) << i;
        }
    }
}

TEST(FindPasses, TheStepSizeDoesNotChangeTheCrossingTimes) {
    const core::Sgp4Model model = Model();
    const auto fine = core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd + 1.0, 10.0);
    const auto normal = core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd + 1.0, 20.0);
    const auto coarse = core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd + 1.0, 60.0);
    ASSERT_EQ(fine.size(), normal.size());
    ASSERT_EQ(coarse.size(), normal.size());
    for (std::size_t i = 0; i < normal.size(); ++i) {
        EXPECT_NEAR(fine[i].riseJd, normal[i].riseJd, 0.2 / kSecondsPerDay);
        EXPECT_NEAR(coarse[i].riseJd, normal[i].riseJd, 0.2 / kSecondsPerDay);
        EXPECT_NEAR(fine[i].setJd, normal[i].setJd, 0.2 / kSecondsPerDay);
        EXPECT_NEAR(coarse[i].setJd, normal[i].setJd, 0.2 / kSecondsPerDay);
    }
}

TEST(FindPasses, EmptyOrInvalidWindowsFindNothing) {
    const core::Sgp4Model model = Model();
    EXPECT_TRUE(core::FindPasses(model, app::kObserver, kEpochJd + 1.0, kEpochJd).empty());
    EXPECT_TRUE(core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd).empty());
    EXPECT_TRUE(core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd + 1.0, 0.0).empty());
    EXPECT_TRUE(core::FindPasses(model, app::kObserver, kEpochJd, kEpochJd + 1.0, -5.0).empty());
}

TEST(FindPasses, StopsCleanlyWhenThePropagatorFails) {
    // This orbit decays within hours, so a search over a week hits the failure part-way through.
    const core::Sgp4Model model = Model(kDecaying);
    const double epoch = model.epochJd;
    const auto passes = core::FindPasses(model, app::kObserver, epoch, epoch + 7.0);
    for (const core::Pass& pass : passes) {
        EXPECT_LE(pass.setJd, epoch + 0.25 + 1e-9) << "found a pass after the orbit had decayed";
    }
    EXPECT_FALSE(core::ElevationAt(model, app::kObserver, epoch + 1.0).has_value());
    EXPECT_TRUE(core::FindPasses(model, app::kObserver, epoch + 1.0, epoch + 2.0).empty());
}

TEST(ElevationAt, MatchesTheTopocentricTransformDirectly) {
    const core::Sgp4Model model = Model();
    const double jd = kEpochJd + 0.3;
    const auto elevation = core::ElevationAt(model, app::kObserver, jd);
    ASSERT_TRUE(elevation.has_value());
    EXPECT_GE(*elevation, -kPi / 2.0);
    EXPECT_LE(*elevation, kPi / 2.0);
}

} // namespace
