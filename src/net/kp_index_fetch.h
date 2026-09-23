#pragma once

#include <optional>
#include <string>

namespace net {

// Fetches the current planetary Kp index data from NOAA's Space Weather Prediction Center
// (https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json): an array of recent
// 3-hour readings, most recent last, as raw JSON text. Parsing that into the most recent Kp
// value is a separate concern (a later issue); this only fetches the response and does a light
// sanity check that it looks like Kp data, not an error page. Returns nullopt on any failure,
// with the reason written to stderr.
std::optional<std::string> FetchKpIndex();

} // namespace net
