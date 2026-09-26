#pragma once

#include "net/tle_cache.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace app {

// How long after startup the first received message arrives, and how long between each one after
// that. Offset from the RELAY QUEUE's own schedule (see app::kFirstRelayDelay), which sends at
// the same interval, so replies land between the station's own messages rather than on top of
// them.
inline constexpr std::chrono::seconds kFirstReceivedDelay{80};
inline constexpr std::chrono::seconds kReceivedInterval{120};

// Messages the station receives on a regular schedule, for the INCOMING TRANSMISSION feed: the
// other half of the RELAY QUEUE's messages home. They arrive one at a time, in order, starting
// over from the first once the last has arrived.
//
// If the app is held up for longer than one interval (the machine sleeps, say), only one message
// arrives when it catches up, and the next is scheduled a full interval after that, so the feed
// never floods with every message it "missed".
class ReceivedTransmissions {
  public:
    ReceivedTransmissions(std::vector<std::string> messages, net::Clock::time_point start,
                          std::chrono::seconds firstDelay = kFirstReceivedDelay,
                          std::chrono::seconds interval = kReceivedInterval);

    // The message that has arrived by `now`, if any, which is then counted as received. Call once
    // per frame. Never returns more than one message per call, and never any if there are no
    // messages.
    std::optional<std::string> Due(net::Clock::time_point now);

  private:
    std::vector<std::string> messages_;
    std::chrono::seconds interval_;
    net::Clock::time_point nextAt_;
    std::size_t nextIndex_ = 0;
};

// Reads the received messages from a file in the same one-per-line format as the operating
// rules (see app::ParseOperatingRules and assets/received_messages.txt). Returns an empty list,
// after printing the reason to stderr, if the file cannot be read; the feed then just carries the
// station's own events.
std::vector<std::string> LoadReceivedMessages(const std::filesystem::path& path);

} // namespace app
