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

TEST(CarrierPing, IsLabelledCarrierPing) {
    EXPECT_EQ(app::CarrierPing({}).label, "CARRIER PING");
    EXPECT_EQ(app::CarrierPing({net::TleSource::Network}).label, "CARRIER PING");
}

TEST(CarrierPing, NoneWithNoWatchedSatellites) {
    EXPECT_EQ(app::CarrierPing({}).state, app::RelayState::None);
}

TEST(CarrierPing, SentWhenEveryFetchSucceeded) {
    EXPECT_EQ(app::CarrierPing({net::TleSource::Network}).state, app::RelayState::Sent);
    // A fresh cache is the result of a recent successful fetch, so it counts as sent too.
    EXPECT_EQ(app::CarrierPing({net::TleSource::FreshCache}).state, app::RelayState::Sent);
    EXPECT_EQ(app::CarrierPing({net::TleSource::Network, net::TleSource::FreshCache}).state,
              app::RelayState::Sent);
}

TEST(CarrierPing, HoldWhenAFetchFellBackToAStaleCache) {
    EXPECT_EQ(app::CarrierPing({net::TleSource::StaleCache}).state, app::RelayState::Hold);
    EXPECT_EQ(app::CarrierPing({net::TleSource::Network, net::TleSource::StaleCache}).state,
              app::RelayState::Hold);
}

TEST(CarrierPing, HoldWhenASatelliteFailedToLoadAtAll) {
    EXPECT_EQ(app::CarrierPing({std::nullopt}).state, app::RelayState::Hold);
    EXPECT_EQ(app::CarrierPing({net::TleSource::Network, std::nullopt}).state,
              app::RelayState::Hold);
}

} // namespace
