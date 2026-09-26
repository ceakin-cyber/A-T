#include "app/received_transmissions.h"
#include "app/relay_queue.h"

#include <chrono>
#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

net::Clock::time_point At(int seconds) {
    return net::Clock::time_point(std::chrono::seconds(seconds));
}

TEST(ReceivedTransmissions, NothingBeforeTheFirstDelay) {
    app::ReceivedTransmissions feed({"A", "B"}, At(1000), 80s, 120s);
    EXPECT_FALSE(feed.Due(At(1000)).has_value());
    EXPECT_FALSE(feed.Due(At(1079)).has_value());
}

TEST(ReceivedTransmissions, TheFirstArrivesAfterTheFirstDelay) {
    app::ReceivedTransmissions feed({"A", "B"}, At(1000), 80s, 120s);
    EXPECT_EQ(feed.Due(At(1080)), "A");
}

TEST(ReceivedTransmissions, EachArrivesOnlyOnceThenOneEachInterval) {
    app::ReceivedTransmissions feed({"A", "B", "C"}, At(1000), 80s, 120s);
    EXPECT_EQ(feed.Due(At(1080)), "A");
    EXPECT_FALSE(feed.Due(At(1080)).has_value());
    EXPECT_FALSE(feed.Due(At(1199)).has_value());
    EXPECT_EQ(feed.Due(At(1200)), "B");
    EXPECT_EQ(feed.Due(At(1320)), "C");
}

TEST(ReceivedTransmissions, StartsOverAfterTheLastMessage) {
    app::ReceivedTransmissions feed({"A", "B"}, At(1000), 80s, 120s);
    EXPECT_EQ(feed.Due(At(1080)), "A");
    EXPECT_EQ(feed.Due(At(1200)), "B");
    EXPECT_EQ(feed.Due(At(1320)), "A");
}

TEST(ReceivedTransmissions, JustOneArrivesAfterALongStall) {
    app::ReceivedTransmissions feed({"A", "B", "C"}, At(1000), 80s, 120s);
    EXPECT_EQ(feed.Due(At(4600)), "A"); // an hour late
    EXPECT_FALSE(feed.Due(At(4601)).has_value());
    EXPECT_FALSE(feed.Due(At(4719)).has_value());
    EXPECT_EQ(feed.Due(At(4720)), "B");
}

TEST(ReceivedTransmissions, NoMessagesMeansNothingEverArrives) {
    app::ReceivedTransmissions feed({}, At(1000), 80s, 120s);
    EXPECT_FALSE(feed.Due(At(1080)).has_value());
    EXPECT_FALSE(feed.Due(At(99999)).has_value());
}

// The default schedule keeps received messages from landing on the same moment as the relay
// queue's sends: same interval, first arrival partway between two sends.
TEST(ReceivedTransmissions, DefaultScheduleFallsBetweenTheRelayQueuesSends) {
    EXPECT_EQ(app::kReceivedInterval, app::kRelayInterval);
    const auto offset = (app::kFirstReceivedDelay - app::kFirstRelayDelay) % app::kRelayInterval;
    EXPECT_GT(offset, 0s);
    EXPECT_LT(offset, app::kRelayInterval);
}

TEST(LoadReceivedMessages, ReadsTheRealCommittedMessages) {
    const auto messages = app::LoadReceivedMessages("assets/received_messages.txt");
    ASSERT_FALSE(messages.empty());
    for (const std::string& message : messages) {
        EXPECT_NE(message.front(), '#'); // the header comment is not a message
        for (const char c : message) {
            EXPECT_GE(c, ' ') << message; // the terminal font only covers printable ASCII
            EXPECT_LE(c, '~') << message;
        }
    }
}

TEST(LoadReceivedMessages, MissingFileGivesNoMessages) {
    EXPECT_TRUE(app::LoadReceivedMessages("/nonexistent/received_messages.txt").empty());
}

} // namespace
