#include "app/received_transmissions.h"

#include "app/operating_rules.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace app {

ReceivedTransmissions::ReceivedTransmissions(std::vector<std::string> messages,
                                             net::Clock::time_point start,
                                             std::chrono::seconds firstDelay,
                                             std::chrono::seconds interval)
    : messages_(std::move(messages)), interval_(interval), nextAt_(start + firstDelay) {}

std::optional<std::string> ReceivedTransmissions::Due(net::Clock::time_point now) {
    if (messages_.empty() || now < nextAt_) {
        return std::nullopt;
    }
    std::string message = messages_[nextIndex_];
    nextIndex_ = (nextIndex_ + 1) % messages_.size();
    // From now, not from when this one was due, so a long stall brings one message, not a burst.
    nextAt_ = now + interval_;
    return message;
}

std::vector<std::string> LoadReceivedMessages(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open received messages at " << path << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseOperatingRules(buffer.str());
}

} // namespace app
