#pragma once

#include "app/event_log.h"

#include <optional>

namespace ui {

// Draws a scrolling table of timestamped log entries: `title` names the window, and `placeholder`
// is shown instead while `log` is empty. Docked into the dashboard's dockspace by default. Shared
// by every panel that shows an app::EventLog -- they differ only in title, which log, their
// placeholder text, and whether new lines type themselves in (see event_log_panel.h and
// incoming_transmission_panel.h).
//
// `typingEffectNow`, when given, animates each entry in with a typewriter effect (see
// app::TypingEffect), computed from that entry's own timestamp against this "now" -- so an entry
// already older than the typing duration just renders in full, no extra state needed. Left at
// its default, nullopt, every entry always renders in full immediately, as before this effect
// existed.
void DrawLogPanel(const char* title, const app::EventLog& log, const char* placeholder,
                  std::optional<net::Clock::time_point> typingEffectNow = std::nullopt);

} // namespace ui
