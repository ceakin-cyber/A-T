#include "app/signal_quality.h"

#include <gtest/gtest.h>

namespace {

TEST(ClassifyKp, StableBelowFour) {
    EXPECT_EQ(app::ClassifyKp(0.0), app::SignalQuality::Stable);
    EXPECT_EQ(app::ClassifyKp(2.33), app::SignalQuality::Stable);
    EXPECT_EQ(app::ClassifyKp(3.99), app::SignalQuality::Stable);
}

TEST(ClassifyKp, DegradedFromFourToSixInclusive) {
    EXPECT_EQ(app::ClassifyKp(4.0), app::SignalQuality::Degraded);
    EXPECT_EQ(app::ClassifyKp(5.0), app::SignalQuality::Degraded);
    EXPECT_EQ(app::ClassifyKp(6.0), app::SignalQuality::Degraded);
}

TEST(ClassifyKp, DisruptedAboveSix) {
    EXPECT_EQ(app::ClassifyKp(6.01), app::SignalQuality::Disrupted);
    EXPECT_EQ(app::ClassifyKp(7.67), app::SignalQuality::Disrupted);
    EXPECT_EQ(app::ClassifyKp(9.0), app::SignalQuality::Disrupted);
}

TEST(ClassifyKp, NegativeKpCountsAsStable) {
    // Kp is never really negative (0-9 by definition), but a malformed or extrapolated reading
    // should not crash or misclassify; anything below 4 is Stable.
    EXPECT_EQ(app::ClassifyKp(-1.0), app::SignalQuality::Stable);
}

// Known real-world Kp readings, not synthetic numbers: from the official GFZ Potsdam Kp index
// archive (https://kp.gfz.de/, CC BY 4.0), the definitive source the Kp index is computed from
// (the same figures NOAA's own feed -- see net::FetchKpIndex -- ultimately derives from), for the
// well-documented May 2024 "Gannon storm", the first G5 "Extreme" geomagnetic storm since 2003,
// plus a calm reading from ten days earlier for contrast.
TEST(ClassifyKp, StableDuringACalmPeriodBeforeTheMay2024Storm) {
    // 2024-05-01 09:00 UTC: Kp 0.333, an ordinary quiet day well before the storm.
    EXPECT_EQ(app::ClassifyKp(0.333), app::SignalQuality::Stable);
}

TEST(ClassifyKp, DegradedDuringTheStormsDecliningPhase) {
    // 2024-05-13 00:00 and 03:00 UTC: Kp 5.333 and exactly 6.0, as the storm was winding down.
    EXPECT_EQ(app::ClassifyKp(5.333), app::SignalQuality::Degraded);
    EXPECT_EQ(app::ClassifyKp(6.0), app::SignalQuality::Degraded);
}

TEST(ClassifyKp, DisruptedAtTheStormsPeak) {
    // 2024-05-11 00:00 UTC: Kp 9.0 exactly, the maximum possible reading, reported by NOAA and
    // GFZ alike as the peak of the storm (G5, Extreme).
    EXPECT_EQ(app::ClassifyKp(9.0), app::SignalQuality::Disrupted);
    // 2024-05-10 18:00 and 21:00 UTC, already well into G4 territory as the storm arrived.
    EXPECT_EQ(app::ClassifyKp(8.667), app::SignalQuality::Disrupted);
}

} // namespace
