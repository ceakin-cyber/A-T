#pragma once

#include <chrono>
#include <ctime>
#include <string>

namespace app {

// The time zone every time in the app is shown in -- log timestamps, pass times, the star map's
// sky time, and the rest (see app::FormatTime and app::FormatClock). One of:
//   "UTC"   -- the default;
//   "LOCAL" -- whatever zone this computer is set to;
//   an IANA time zone name, such as "America/New_York" or "Asia/Tokyo", looked up in the system's
//   own time zone database, so daylight saving time is handled as it is everywhere else.
// Only the display changes: every calculation keeps working in UTC throughout.
inline constexpr const char* kUtcTimeZone = "UTC";
inline constexpr const char* kLocalTimeZone = "LOCAL";

// Whether `name` is a zone SetDisplayTimeZone would accept: "UTC", "LOCAL", or a name found in
// the system's time zone database (under $TZDIR, else /usr/share/zoneinfo).
bool IsKnownTimeZone(const std::string& name);

// Switches the display to `name` and returns true, or leaves it as it was and returns false if
// `name` is not a known zone (see IsKnownTimeZone). The app starts out in UTC. This works through
// the C library's own TZ setting, so it is process-wide and not thread-safe; call it only from
// the thread that draws the UI.
bool SetDisplayTimeZone(const std::string& name);

// The zone currently in use, as it was given to SetDisplayTimeZone ("UTC" until then).
const std::string& DisplayTimeZone();

// `time` broken down into calendar fields in the display zone.
std::tm ToDisplayTime(std::chrono::system_clock::time_point time);

// The display zone's short name at `time`, such as "UTC", "EDT" or "JST" -- or, for a zone with
// no letters of its own, its UTC offset, such as "+0530". Can change with daylight saving time.
std::string DisplayTimeZoneAbbreviation(std::chrono::system_clock::time_point time);

} // namespace app
