#include "net/kp_index_fetch.h"

#include <cctype>
#include <cpr/cpr.h>
#include <iostream>
#include <string_view>

namespace net {

namespace {

constexpr const char* kKpIndexUrl =
    "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json";

// NOAA answers with a JSON array of {"time_tag": ..., "Kp": ..., ...} objects; a proxy failure or
// maintenance notice would not have this shape. Not a real JSON validator -- parsing the response
// is a separate, later concern -- just enough to catch an obviously-wrong response early.
bool LooksLikeKpIndex(const std::string& body) {
    std::string_view trimmed = body;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
        trimmed.remove_prefix(1);
    }
    return trimmed.starts_with('[') && body.find("\"Kp\"") != std::string::npos;
}

} // namespace

std::optional<std::string> FetchKpIndex() {
    const cpr::Response response = cpr::Get(cpr::Url{kKpIndexUrl}, cpr::Timeout{5000});

    if (response.error) {
        std::cerr << "Kp index fetch failed: " << response.error.message << '\n';
        return std::nullopt;
    }
    if (response.status_code != 200) {
        std::cerr << "Kp index fetch failed: HTTP " << response.status_code << '\n';
        return std::nullopt;
    }
    if (!LooksLikeKpIndex(response.text)) {
        std::cerr << "Kp index fetch failed: response did not look like Kp index data\n";
        return std::nullopt;
    }
    return response.text;
}

} // namespace net
