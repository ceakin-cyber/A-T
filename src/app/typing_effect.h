#pragma once

#include "app/event_log.h"

#include <chrono>
#include <deque>
#include <string>
#include <vector>

namespace app {

// How many characters of the incoming-transmission log's typing effect appear per second.
inline constexpr double kTypingCharsPerSecond = 40.0;

// The prefix of `fullText` that should be visible after `elapsed` time, for a typewriter/teletype
// effect: none of it while elapsed is zero or negative (a clock that runs backward, however
// unlikely, must not crash or wrap around, just show nothing), more of it as elapsed grows, and
// all of it once elapsed * charsPerSecond reaches fullText's length. charsPerSecond at or below
// zero is treated as "no effect" and shows the full text immediately, rather than getting stuck
// showing nothing.
std::string TypingEffect(const std::string& fullText, std::chrono::duration<double> elapsed,
                         double charsPerSecond);

// The time each entry in `entries` (oldest first, the order app::EventLog::Entries() returns)
// begins typing under a sequential, one-line-at-a-time typewriter effect: the first entry begins
// at its own timestamp; each one after begins at the later of its own timestamp and the time the
// previous entry finished typing (its own start plus however long its full message takes at
// charsPerSecond). This means entries logged in the very same instant -- as, for example, "TLE
// FETCHED" and "PROPAGATOR INITIALIZED" are -- still type out one after another, never at once,
// and a slow typing speed can push a later entry's start past its own real timestamp. A caller
// combines this with TypingEffect (elapsed = now - this entry's start time) to animate each line
// in turn; an entry whose start time is still in the future has nothing to show yet.
std::vector<net::Clock::time_point> TypingStartTimes(const std::deque<LogEntry>& entries,
                                                      double charsPerSecond);

} // namespace app
