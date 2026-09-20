#include "net/tle_fetch.h"

#include <cpr/cpr.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace net {

namespace {

// Celestrak answers unknown IDs with a 200 and a plain message, so check the body's shape.
bool LooksLikeTle(const std::string& body) {
    std::vector<std::string> lines;
    std::istringstream stream(body);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines.size() >= 3 && lines[1].rfind("1 ", 0) == 0 && lines[2].rfind("2 ", 0) == 0;
}

} // namespace

std::optional<std::string> FetchTle(int noradId) {
    const cpr::Response response = cpr::Get(
        cpr::Url{"https://celestrak.org/NORAD/elements/gp.php"},
        cpr::Parameters{{"CATNR", std::to_string(noradId)}, {"FORMAT", "TLE"}}, cpr::Timeout{5000});

    if (response.error) {
        std::cerr << "TLE fetch failed: " << response.error.message << '\n';
        return std::nullopt;
    }
    if (response.status_code != 200) {
        std::cerr << "TLE fetch failed: HTTP " << response.status_code << '\n';
        return std::nullopt;
    }
    if (!LooksLikeTle(response.text)) {
        std::cerr << "TLE fetch failed: no TLE data for catalog number " << noradId << '\n';
        return std::nullopt;
    }
    return response.text;
}

} // namespace net
