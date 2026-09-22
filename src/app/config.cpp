#include "app/config.h"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numbers>
#include <sstream>

namespace app {

namespace fs = std::filesystem;

namespace {

std::string_view Trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

std::optional<double> ParseDouble(std::string_view s) {
    s = Trim(s);
    double value = 0.0;
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || end != s.data() + s.size()) {
        return std::nullopt;
    }
    return value;
}

// "key = value" -> {"key", "value"}, or nullopt if there is no '='.
std::optional<std::pair<std::string_view, std::string_view>>
SplitAssignment(std::string_view line) {
    const std::size_t eq = line.find('=');
    if (eq == std::string_view::npos) {
        return std::nullopt;
    }
    return std::make_pair(Trim(line.substr(0, eq)), Trim(line.substr(eq + 1)));
}

} // namespace

fs::path DefaultConfigDir() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && *xdg != '\0') {
        return fs::path(xdg) / "a-t";
    }
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return fs::path(home) / ".config" / "a-t";
    }
    return fs::temp_directory_path() / "a-t";
}

Config ParseConfig(const std::string& text) {
    const Config defaults;
    double latDeg = defaults.observer.latitude * 180.0 / std::numbers::pi;
    double lonDeg = defaults.observer.longitude * 180.0 / std::numbers::pi;
    double altKm = defaults.observer.altitudeKm;
    std::vector<WatchEntry> watchlist;
    bool watchlistStarted = false;

    std::istringstream stream(text);
    std::string rawLine;
    int lineNumber = 0;
    while (std::getline(stream, rawLine)) {
        ++lineNumber;
        const std::string_view line = Trim(rawLine);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        if (line.rfind("watch", 0) == 0 &&
            (line.size() == 5 || std::isspace(static_cast<unsigned char>(line[5])))) {
            const std::string_view rest = Trim(line.substr(5));
            const std::size_t space = rest.find_first_of(" \t");
            const std::string_view idText =
                space == std::string_view::npos ? rest : rest.substr(0, space);
            int noradId = 0;
            const auto [end, ec] =
                std::from_chars(idText.data(), idText.data() + idText.size(), noradId);
            if (ec != std::errc() || end != idText.data() + idText.size() || noradId <= 0) {
                std::cerr << "Config line " << lineNumber
                          << ": bad watch entry, skipping: " << rawLine << '\n';
                continue;
            }
            const std::string_view name =
                space == std::string_view::npos ? "" : Trim(rest.substr(space + 1));
            watchlistStarted = true;
            watchlist.push_back({noradId, std::string(name)});
            continue;
        }

        const auto assignment = SplitAssignment(line);
        if (!assignment) {
            std::cerr << "Config line " << lineNumber << ": not understood, skipping: " << rawLine
                      << '\n';
            continue;
        }
        const auto& [key, valueText] = *assignment;
        const std::optional<double> value = ParseDouble(valueText);
        if (!value) {
            std::cerr << "Config line " << lineNumber << ": " << key
                      << " is not a number, skipping: " << rawLine << '\n';
            continue;
        }

        if (key == "observer_lat_deg") {
            latDeg = *value;
        } else if (key == "observer_lon_deg") {
            lonDeg = *value;
        } else if (key == "observer_alt_km") {
            altKm = *value;
        } else {
            std::cerr << "Config line " << lineNumber << ": unknown key '" << key
                      << "', skipping\n";
        }
    }

    Config config;
    config.observer = core::GeodeticFromDegrees(latDeg, lonDeg, altKm);
    config.watchlist = watchlistStarted ? watchlist : defaults.watchlist;
    return config;
}

Config LoadConfig(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "No config file at " << path << ", using defaults\n";
        return Config{};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseConfig(buffer.str());
}

} // namespace app
