#pragma once

#include <optional>
#include <string>

namespace net {

// Fetches the current planetary Kp index data from NOAA's Space Weather Prediction Center
// (https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json): an array of recent
// 3-hour readings, most recent last, as raw JSON text. Parsing that into the most recent Kp
// value is a separate concern (see ParseMostRecentKp below); this only fetches the response and
// does a light sanity check that it looks like Kp data, not an error page. Returns nullopt on
// any failure, with the reason written to stderr.
std::optional<std::string> FetchKpIndex();

// Parses the most recent Kp value out of the JSON FetchKpIndex returns: an array of
// {"time_tag": ..., "Kp": <number>, ...} objects ordered oldest to newest, so the most recent
// reading is the last object in the array. This is a minimal, purpose-built reader for that one
// known, flat response shape (isolates the last top-level object, then reads its own "Kp" field),
// not a general JSON parser -- there is no other data in this project's own format that looks
// like it, so nothing here is meant to be reused for anything else. Returns nullopt if the text
// is not shaped like that (not a JSON array, no object with a numeric "Kp" field, or the array is
// empty).
std::optional<double> ParseMostRecentKp(const std::string& json);

} // namespace net
