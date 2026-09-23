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

} // namespace
