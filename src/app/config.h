#pragma once

#include "app/observer.h"
#include "core/geodetic.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace app {

struct WatchEntry {
    int noradId = 0;
    std::string name; // optional; empty if the line gave no name
};

struct Config {
    core::Geodetic observer = kObserver;
    // Defaults to the ISS. Only watchlist.front() is tracked today; more entries are for when
    // the app can track several satellites at once.
    std::vector<WatchEntry> watchlist = {{25544, "ISS (ZARYA)"}};
};

// Per-user config directory: $XDG_CONFIG_HOME/a-t, else ~/.config/a-t.
std::filesystem::path DefaultConfigDir();

// Parses the config file format:
//   observer_lat_deg = 51.4779
//   observer_lon_deg = 0.0
//   observer_alt_km = 0.062
//   watch 25544 ISS (ZARYA)
// Blank lines and lines starting with '#' are ignored. Any field or the whole watchlist may be
// left out, in which case that part of the default Config is kept. A malformed line (not one of
// the forms above, or a number that fails to parse) is skipped, with a reason printed to stderr;
// it does not stop the rest of the file from loading.
Config ParseConfig(const std::string& text);

// Reads the config from `path`, or returns the default Config (with a note to stderr, not an
// error) if the file does not exist or cannot be read.
Config LoadConfig(const std::filesystem::path& path);

} // namespace app
