#pragma once

#include "net/tle_source.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace app {

// Where a relay queue item stands, shown beside its label in the RELAY QUEUE panel.
enum class RelayState {
    Sent,  // already relayed
    Hold,  // queued but held back
    Armed, // ready to relay
    None,  // no state to show
};

// One line of the RELAY QUEUE panel: a label and its state.
struct RelayItem {
    std::string label;
    RelayState state = RelayState::None;
};

// The state's display name for the RELAY QUEUE panel: "SENT", "HOLD", "ARMED" or "NONE".
const char* ToString(RelayState state);

// The CARRIER PING relay item, its state taken from the last real TLE fetch for each watched
// satellite: one entry per satellite, holding the source its TLE loaded from, or nullopt if it
// failed to load (fetch failed with no cache to fall back on). Like SystemMode (see
// app/system_status.h), the worst case across the roster wins:
//   - HOLD if any satellite's fetch failed (a stale cache, or nothing at all);
//   - SENT if every one succeeded -- fetched just now, or a fresh cache left by a recent
//     successful fetch;
//   - NONE if there are no watched satellites, so nothing was ever fetched.
// ARMED is not produced yet: nothing currently queues a ping to go out later.
RelayItem CarrierPing(const std::vector<std::optional<net::TleSource>>& sources);

// How long after startup the first queued message is sent, and how long between each one after.
inline constexpr std::chrono::seconds kFirstRelayDelay{20};
inline constexpr std::chrono::seconds kRelayInterval{120};

// The station's outgoing messages home, for the RELAY QUEUE panel: sent one at a time on a
// regular schedule, in order, starting over from the first once the last has gone. At any moment
// one message is ARMED (next to go), the ones after it wait on HOLD, and the one sent most
// recently shows as SENT.
//
// If the app is held up for longer than one interval (the machine sleeps, say), only one message
// is sent when it catches up, and the next is armed for a full interval after that, so the queue
// never races through everything it "missed".
class RelayQueue {
  public:
    RelayQueue(std::vector<std::string> messages, net::Clock::time_point start,
               std::chrono::seconds firstDelay = kFirstRelayDelay,
               std::chrono::seconds interval = kRelayInterval);

    // Sends the armed message if its time has come by `now`: it becomes the last one sent, and
    // the next is armed. Sends at most one per call; call once per frame. Returns whether one
    // was sent.
    bool Update(net::Clock::time_point now);

    // The queue as panel rows, in order: the last message sent (SENT; left out until one has
    // been), the armed one (ARMED), then up to `holdCount` of those after it (HOLD) -- never
    // listing any message twice, however few there are. Empty if there are no messages.
    std::vector<RelayItem> Items(std::size_t holdCount) const;

    // When the last message was sent, or nullopt if none has been yet.
    std::optional<net::Clock::time_point> LastSentAt() const { return lastSentAt_; }
    // When the armed message is due to be sent.
    net::Clock::time_point NextAt() const { return nextAt_; }
    bool Empty() const { return messages_.empty(); }

  private:
    std::vector<std::string> messages_;
    std::chrono::seconds interval_;
    net::Clock::time_point nextAt_;
    std::size_t armedIndex_ = 0;
    std::optional<std::size_t> lastSentIndex_;
    std::optional<net::Clock::time_point> lastSentAt_;
};

// Reads the outgoing messages from a file in the same one-per-line format as the operating rules
// (see app::ParseOperatingRules and assets/relay_messages.txt). Returns an empty list, after
// printing the reason to stderr, if the file cannot be read; the queue then shows only the
// CARRIER PING.
std::vector<std::string> LoadRelayMessages(const std::filesystem::path& path);

} // namespace app
