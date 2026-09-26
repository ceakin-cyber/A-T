#include "app/relay_queue.h"

#include "app/operating_rules.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace app {

const char* ToString(RelayState state) {
    switch (state) {
    case RelayState::Sent:
        return "SENT";
    case RelayState::Hold:
        return "HOLD";
    case RelayState::Armed:
        return "ARMED";
    case RelayState::None:
        return "NONE";
    }
    return "UNKNOWN";
}

RelayItem CarrierPing(const std::vector<std::optional<net::TleSource>>& sources) {
    RelayItem item{"CARRIER PING", RelayState::None};
    if (sources.empty()) {
        return item;
    }
    item.state = RelayState::Sent;
    for (const std::optional<net::TleSource>& source : sources) {
        if (!source || *source == net::TleSource::StaleCache) {
            item.state = RelayState::Hold;
            break; // nothing worse than a failed fetch; no need to keep looking
        }
    }
    return item;
}

RelayQueue::RelayQueue(std::vector<std::string> messages, net::Clock::time_point start,
                       std::chrono::seconds firstDelay, std::chrono::seconds interval)
    : messages_(std::move(messages)), interval_(interval), nextAt_(start + firstDelay) {}

bool RelayQueue::Update(net::Clock::time_point now) {
    if (messages_.empty() || now < nextAt_) {
        return false;
    }
    lastSentIndex_ = armedIndex_;
    lastSentAt_ = now;
    armedIndex_ = (armedIndex_ + 1) % messages_.size();
    // From now, not from when this one was due, so a long stall sends one message, not a burst.
    nextAt_ = now + interval_;
    return true;
}

std::vector<RelayItem> RelayQueue::Items(std::size_t holdCount) const {
    std::vector<RelayItem> items;
    if (messages_.empty()) {
        return items;
    }
    const std::size_t count = messages_.size();
    // With a single message, the one just sent is also the one armed again; list it once.
    const bool showSent = lastSentIndex_.has_value() && *lastSentIndex_ != armedIndex_;
    if (showSent) {
        items.push_back({messages_[*lastSentIndex_], RelayState::Sent});
    }
    items.push_back({messages_[armedIndex_], RelayState::Armed});
    const std::size_t others = count - (showSent ? 2 : 1);
    for (std::size_t i = 1; i <= std::min(holdCount, others); ++i) {
        items.push_back({messages_[(armedIndex_ + i) % count], RelayState::Hold});
    }
    return items;
}

std::vector<std::string> LoadRelayMessages(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open relay messages at " << path << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseOperatingRules(buffer.str());
}

} // namespace app
