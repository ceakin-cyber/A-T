#include "app/relay_queue.h"

#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <vector>

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

using namespace std::chrono_literals;

net::Clock::time_point At(int seconds) {
    return net::Clock::time_point(std::chrono::seconds(seconds));
}

std::vector<std::string> Labels(const std::vector<app::RelayItem>& items) {
    std::vector<std::string> labels;
    for (const app::RelayItem& item : items) {
        labels.push_back(item.label);
    }
    return labels;
}

std::vector<app::RelayState> States(const std::vector<app::RelayItem>& items) {
    std::vector<app::RelayState> states;
    for (const app::RelayItem& item : items) {
        states.push_back(item.state);
    }
    return states;
}

using enum app::RelayState;

TEST(RelayQueue, StartsWithTheFirstMessageArmedAndTheRestOnHold) {
    const app::RelayQueue queue({"A", "B", "C", "D"}, At(1000), 20s, 120s);
    const auto items = queue.Items(2);
    EXPECT_EQ(Labels(items), (std::vector<std::string>{"A", "B", "C"}));
    EXPECT_EQ(States(items), (std::vector<app::RelayState>{Armed, Hold, Hold}));
    EXPECT_FALSE(queue.LastSentAt().has_value());
    EXPECT_EQ(queue.NextAt(), At(1020));
}

TEST(RelayQueue, SendsNothingBeforeTheFirstDelay) {
    app::RelayQueue queue({"A", "B"}, At(1000), 20s, 120s);
    EXPECT_FALSE(queue.Update(At(1000)));
    EXPECT_FALSE(queue.Update(At(1019)));
    EXPECT_EQ(States(queue.Items(5)), (std::vector<app::RelayState>{Armed, Hold}));
}

TEST(RelayQueue, SendingMovesTheArmedMessageToSentAndArmsTheNext) {
    app::RelayQueue queue({"A", "B", "C", "D"}, At(1000), 20s, 120s);
    EXPECT_TRUE(queue.Update(At(1020)));
    const auto items = queue.Items(2);
    EXPECT_EQ(Labels(items), (std::vector<std::string>{"A", "B", "C", "D"}));
    EXPECT_EQ(States(items), (std::vector<app::RelayState>{Sent, Armed, Hold, Hold}));
    EXPECT_EQ(queue.LastSentAt(), At(1020));
    EXPECT_EQ(queue.NextAt(), At(1140));
}

TEST(RelayQueue, SendsEachMessageOnlyOnceThenOneEachInterval) {
    app::RelayQueue queue({"A", "B", "C"}, At(1000), 20s, 120s);
    EXPECT_TRUE(queue.Update(At(1020)));
    EXPECT_FALSE(queue.Update(At(1020)));
    EXPECT_FALSE(queue.Update(At(1139)));
    EXPECT_TRUE(queue.Update(At(1140)));
    EXPECT_EQ(queue.Items(0)[0].label, "B");
    EXPECT_EQ(queue.Items(0)[0].state, Sent);
}

TEST(RelayQueue, StartsOverAfterTheLastMessage) {
    app::RelayQueue queue({"A", "B"}, At(1000), 20s, 120s);
    queue.Update(At(1020)); // A sent, B armed
    queue.Update(At(1140)); // B sent, A armed again
    const auto items = queue.Items(5);
    EXPECT_EQ(Labels(items), (std::vector<std::string>{"B", "A"}));
    EXPECT_EQ(States(items), (std::vector<app::RelayState>{Sent, Armed}));
}

TEST(RelayQueue, NeverListsAMessageTwice) {
    app::RelayQueue queue({"A", "B", "C"}, At(1000), 20s, 120s);
    queue.Update(At(1020));
    // Asking for more on hold than there are messages left only lists each once.
    EXPECT_EQ(Labels(queue.Items(10)), (std::vector<std::string>{"A", "B", "C"}));
}

TEST(RelayQueue, ASingleMessageIsListedOnceEvenAfterSending) {
    app::RelayQueue queue({"A"}, At(1000), 20s, 120s);
    queue.Update(At(1020));
    const auto items = queue.Items(3);
    ASSERT_EQ(items.size(), 1U);
    EXPECT_EQ(items[0].state, Armed);
}

TEST(RelayQueue, SendsJustOneAfterALongStall) {
    app::RelayQueue queue({"A", "B", "C"}, At(1000), 20s, 120s);
    EXPECT_TRUE(queue.Update(At(4600))); // an hour late
    EXPECT_FALSE(queue.Update(At(4601)));
    EXPECT_EQ(queue.NextAt(), At(4720));
}

TEST(RelayQueue, NoMessagesMeansNoItemsAndNothingSent) {
    app::RelayQueue queue({}, At(1000), 20s, 120s);
    EXPECT_TRUE(queue.Empty());
    EXPECT_FALSE(queue.Update(At(99999)));
    EXPECT_TRUE(queue.Items(5).empty());
}

TEST(LoadRelayMessages, ReadsTheRealCommittedMessages) {
    const auto messages = app::LoadRelayMessages("assets/relay_messages.txt");
    ASSERT_FALSE(messages.empty());
    for (const std::string& message : messages) {
        EXPECT_NE(message.front(), '#');           // the header comment is not a message
        EXPECT_LE(message.size(), 60U) << message; // short enough for one row of the panel
        for (const char c : message) {
            EXPECT_GE(c, ' ') << message; // the terminal font only covers printable ASCII
            EXPECT_LE(c, '~') << message;
        }
    }
}

TEST(LoadRelayMessages, MissingFileGivesNoMessages) {
    EXPECT_TRUE(app::LoadRelayMessages("/nonexistent/relay_messages.txt").empty());
}

} // namespace
