#pragma once

#include "app/relay_queue.h"
#include "net/tle_cache.h"

namespace ui {

// Draws the RELAY QUEUE panel: what the station is sending out. At the top, how long until the
// next message goes and when the last one did; then the CARRIER PING (`carrierPing`; see
// app::CarrierPing), then the queue of messages home (`queue`): the last one sent, the one armed
// to go next (highlighted), and a few waiting on hold after it, each with its state on the
// right. `now` is this frame's real time, for the countdown.
void DrawRelayQueuePanel(const app::RelayItem& carrierPing, const app::RelayQueue& queue,
                         net::Clock::time_point now);

} // namespace ui
