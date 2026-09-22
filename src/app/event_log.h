#pragma once

#include "net/tle_cache.h"

#include <deque>
#include <optional>
#include <string>

namespace app {

struct LogEntry {
    net::Clock::time_point time;
    std::string message;
};

// A bounded, oldest-first log of short status messages. Once it holds `capacity` entries, adding
// another drops the oldest. The default of 100 matches the event log this will grow into
// (Milestone 16), so that issue can build on this without changing the buffer's shape.
class EventLog {
  public:
    explicit EventLog(std::size_t capacity = 100) : capacity_(capacity) {}

    void Add(net::Clock::time_point time, std::string message);

    // Compares `isAboveHorizon` with the state from the previous call and logs "SIGNAL ACQUIRED"
    // on a rise or "SIGNAL LOST" on a set. Call once per frame with the satellite's current
    // above-horizon state. The first call only records the state; it never logs, so starting the
    // app mid-pass does not claim a rise just happened. Returns true if it logged something.
    bool LogVisibilityChange(net::Clock::time_point now, bool isAboveHorizon);

    const std::deque<LogEntry>& Entries() const { return entries_; }

  private:
    std::size_t capacity_;
    std::deque<LogEntry> entries_;
    std::optional<bool> lastAboveHorizon_;
};

} // namespace app
