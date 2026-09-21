#include "app/observer.h"
#include "app/pass_planner.h"
#include "core/time.h"

#include <gtest/gtest.h>

namespace {

constexpr double kSecondsPerDay = 86400.0;

// ISS elements from Celestrak, 2026-09-20 (epoch JD 2461304.02959654).
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.52959654  .00008422  00000+0  15975-3 0  9999\n"
                         "2 25544  51.6308 188.2246 0004825 162.2847 197.8311 15.49196792586535\n";

const char* const kDecaying =
    "1 90002U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9992\n"
    "2 90002  51.6307 190.1401 0005000 160.6694 199.4478 16.67663952123456\n";

constexpr double kEpochJd = 2461304.02959654;

// Rise and set times of the first Greenwich passes after the epoch, computed independently (see
// passes_test.cpp).
struct Reference {
    double riseJd;
    double setJd;
};
const Reference kPass1{2461304.1014619675, 2461304.106879307};
const Reference kPass2{2461304.167168064, 2461304.1744908066};
const Reference kPass3{2461304.2341060154, 2461304.2417019755};

core::Sgp4Model Model(const char* text = kIss) {
    const auto tle = core::ParseTle(text);
    EXPECT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(tle.value_or(core::Tle{}));
    EXPECT_TRUE(model.has_value());
    return model.value_or(core::Sgp4Model{});
}

net::Clock::time_point At(double julianDate) {
    return core::TimePointFromJulianDate(julianDate);
}

void ExpectPass(const std::optional<core::Pass>& pass, const Reference& expected) {
    ASSERT_TRUE(pass.has_value());
    EXPECT_NEAR(pass->riseJd, expected.riseJd, 1.0 / kSecondsPerDay);
    EXPECT_NEAR(pass->setJd, expected.setJd, 1.0 / kSecondsPerDay);
    EXPECT_FALSE(pass->risesBeforeWindow);
    EXPECT_FALSE(pass->setsAfterWindow);
}

TEST(PassPlanner, ReturnsTheFirstPassAfterTheEpoch) {
    app::PassPlanner planner(Model(), app::kObserver);
    ExpectPass(planner.Next(At(kEpochJd)), kPass1);
}

TEST(PassPlanner, ReturnsAPassInProgressWithItsRealRiseTime) {
    app::PassPlanner planner(Model(), app::kObserver);
    const double middle = 0.5 * (kPass2.riseJd + kPass2.setJd);
    // Not clipped to the scan window, even though the scan starts only 72 minutes earlier.
    ExpectPass(planner.Next(At(middle)), kPass2);
}

TEST(PassPlanner, MovesToTheFollowingPassOnceOneHasSet) {
    app::PassPlanner planner(Model(), app::kObserver);
    ExpectPass(planner.Next(At(kPass1.setJd + 10.0 / kSecondsPerDay)), kPass2);
}

TEST(PassPlanner, SwitchesExactlyAroundASetTime) {
    app::PassPlanner planner(Model(), app::kObserver);
    ExpectPass(planner.Next(At(kPass2.setJd - 2.0 / kSecondsPerDay)), kPass2);
    ExpectPass(planner.Next(At(kPass2.setJd + 2.0 / kSecondsPerDay)), kPass3);
}

TEST(PassPlanner, AnswersFromTheSameScanForFramesThatFollowEachOther) {
    // Frames arrive every ~16 ms; the answer must not change until a pass sets.
    app::PassPlanner planner(Model(), app::kObserver);
    for (int frame = 0; frame < 2000; ++frame) {
        const double jd = kEpochJd + frame * (0.016 / kSecondsPerDay);
        ExpectPass(planner.Next(At(jd)), kPass1);
    }
}

TEST(PassPlanner, ScansAgainWhenTheClockJumpsWellAhead) {
    const core::Sgp4Model model = Model();
    app::PassPlanner planner(model, app::kObserver);
    ASSERT_TRUE(planner.Next(At(kEpochJd)).has_value());

    const double later = kEpochJd + 3.0;
    const auto pass = planner.Next(At(later));
    ASSERT_TRUE(pass.has_value());
    EXPECT_GT(pass->setJd, later);

    // The same answer as a scan made directly for that moment.
    const auto direct = core::FindPasses(model, app::kObserver, later - 0.05, later + 2.0);
    for (const core::Pass& candidate : direct) {
        if (candidate.setJd > later) {
            EXPECT_NEAR(pass->riseJd, candidate.riseJd, 1e-9);
            EXPECT_NEAR(pass->setJd, candidate.setJd, 1e-9);
            break;
        }
    }
}

TEST(PassPlanner, ScansAgainWhenTheClockGoesBackwards) {
    app::PassPlanner planner(Model(), app::kObserver);
    ASSERT_TRUE(planner.Next(At(kEpochJd + 3.0)).has_value());
    ExpectPass(planner.Next(At(kEpochJd + 0.05)), kPass1);
}

TEST(PassPlanner, RefreshesAsTimeApproachesTheEndOfItsWindow) {
    // Step through three days in half-hour frames. The pass returned must always still be ahead
    // of the clock, and must never go backwards.
    app::PassPlanner planner(Model(), app::kObserver);
    double previousSet = 0.0;
    for (double jd = kEpochJd; jd < kEpochJd + 3.0; jd += 1.0 / 48.0) {
        const auto pass = planner.Next(At(jd));
        ASSERT_TRUE(pass.has_value()) << jd - kEpochJd;
        EXPECT_GT(pass->setJd, jd) << jd - kEpochJd;
        EXPECT_GE(pass->setJd, previousSet) << jd - kEpochJd;
        EXPECT_FALSE(pass->setsAfterWindow);
        previousSet = pass->setJd;
    }
}

TEST(PassPlanner, ReturnsNothingWhereTheSatelliteNeverRises) {
    app::PassPlanner planner(Model(), core::GeodeticFromDegrees(85.0, 20.0, 0.0));
    EXPECT_FALSE(planner.Next(At(kEpochJd)).has_value());
    EXPECT_FALSE(planner.Next(At(kEpochJd + 0.5)).has_value());
}

TEST(PassPlanner, ReturnsNothingWhenThePropagatorFails) {
    const core::Sgp4Model model = Model(kDecaying);
    app::PassPlanner planner(model, app::kObserver);
    EXPECT_FALSE(planner.Next(At(model.epochJd + 5.0)).has_value());
}

TEST(PassPlanner, FindsAPassJustBeyondTheEndOfAnOldWindow) {
    // Passes come in clusters. Scan at the epoch, then ask 1.4 days later, when the first window
    // has no pass left ahead of the clock but the next cluster starts soon after it ends.
    const core::Sgp4Model model = Model();
    app::PassPlanner planner(model, app::kObserver);
    ASSERT_TRUE(planner.Next(At(kEpochJd)).has_value());

    const double later = kEpochJd + 1.3958;
    const auto pass = planner.Next(At(later));
    ASSERT_TRUE(pass.has_value());
    EXPECT_GT(pass->setJd, later);
    EXPECT_GT(pass->riseJd, kEpochJd + 2.0 - 0.05)
        << "expected the pass to lie past the old window";
}

} // namespace
