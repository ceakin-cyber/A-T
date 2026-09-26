#include "app/relay_queue.h"

#include <gtest/gtest.h>

namespace {

TEST(RelayItem, DefaultsToNoState) {
    const app::RelayItem item;
    EXPECT_TRUE(item.label.empty());
    EXPECT_EQ(item.state, app::RelayState::None);
}

TEST(RelayItem, HoldsLabelAndState) {
    const app::RelayItem item{"ISS DOWNLINK", app::RelayState::Armed};
    EXPECT_EQ(item.label, "ISS DOWNLINK");
    EXPECT_EQ(item.state, app::RelayState::Armed);
}

TEST(RelayStateToString, NamesEachState) {
    EXPECT_STREQ(app::ToString(app::RelayState::Sent), "SENT");
    EXPECT_STREQ(app::ToString(app::RelayState::Hold), "HOLD");
    EXPECT_STREQ(app::ToString(app::RelayState::Armed), "ARMED");
    EXPECT_STREQ(app::ToString(app::RelayState::None), "NONE");
}

} // namespace
