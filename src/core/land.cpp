#include "core/land.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numbers>
#include <optional>
#include <sstream>

namespace core {

namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;

// Reads a whole string as a number, or nullopt if any of it is not part of one.
std::optional<double> ParseDouble(const std::string& text) {
    if (text.empty()) {
        return std::nullopt;
    }
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (end != text.c_str() + text.size()) {
        return std::nullopt;
    }
    return value;
}

// Parses one "longitude,latitude" pair, in degrees, into a ground point.
std::optional<Geodetic> ParsePoint(const std::string& pair) {
    const std::size_t comma = pair.find(',');
    if (comma == std::string::npos) {
        return std::nullopt;
    }
    const std::optional<double> lon = ParseDouble(pair.substr(0, comma));
    const std::optional<double> lat = ParseDouble(pair.substr(comma + 1));
    if (!lon || !lat || *lon < -180.0 || *lon > 180.0 || *lat < -90.0 || *lat > 90.0) {
        return std::nullopt;
    }
    return Geodetic{*lat * kDegToRad, *lon * kDegToRad, 0.0};
}

} // namespace

std::vector<LandRing> ParseLandOutlines(const std::string& text) {
    std::vector<LandRing> rings;
    std::istringstream lines(text);
    std::string line;
    int lineNumber = 0;
    while (std::getline(lines, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') {
            continue;
        }
        LandRing ring;
        std::istringstream pairs(line);
        std::string pair;
        bool ok = true;
        bool first = true;
        while (pairs >> pair) {
            if (first && pair == "hole") {
                ring.hole = true;
                first = false;
                continue;
            }
            first = false;
            const std::optional<Geodetic> point = ParsePoint(pair);
            if (!point) {
                std::cerr << "Land outlines line " << lineNumber << ": cannot read \"" << pair
                          << "\"; skipping this ring\n";
                ok = false;
                break;
            }
            ring.points.push_back(*point);
        }
        if (!ok) {
            continue;
        }
        if (ring.points.size() < 3) {
            if (!ring.points.empty()) {
                std::cerr << "Land outlines line " << lineNumber
                          << ": fewer than 3 points; skipping this ring\n";
            }
            continue;
        }
        rings.push_back(std::move(ring));
    }
    return rings;
}

std::vector<LandRing> LoadLandOutlines(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open land outlines at " << path << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseLandOutlines(buffer.str());
}

} // namespace core
