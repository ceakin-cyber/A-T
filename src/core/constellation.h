#pragma once

#include "core/celestial.h"
#include "core/star_catalog.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace core {

// One line segment of a constellation's stick figure: the two endpoint stars it connects,
// identified by their Hipparcos catalog number (see Star::hip), not this project's own catalog
// id.
struct ConstellationLine {
    std::string constellation; // IAU three-letter abbreviation, e.g. "Ori"
    int hip1 = 0;
    int hip2 = 0;
};

// Parses constellation line data in this project's own CSV format (see
// assets/stars/CONSTELLATION_LINES_SOURCE.md): one row per segment, columns "con", "hip1" and
// "hip2". The header row is required, and columns are found by name, not fixed position. A row
// that cannot be parsed, or is missing one of the required columns, is skipped, with a reason
// printed to stderr; it does not stop the rest of the file from loading.
std::vector<ConstellationLine> ParseConstellationLines(const std::string& text);

// Reads and parses the file. Returns an empty list, after printing the reason to stderr, if the
// file cannot be read.
std::vector<ConstellationLine> LoadConstellationLines(const std::filesystem::path& path);

// An index of a star catalog by Hipparcos number, for resolving constellation line data against
// it. Build once from a loaded catalog and reuse it every frame: the catalog is loaded once at
// startup and never changes afterward, so rebuilding a lookup from scratch on every call would
// redo the same O(number of stars) work every single frame for no reason.
//
// Points into `stars` rather than copying it, so `stars` must outlive the index (and must not be
// modified in a way that invalidates its elements' addresses, such as a push_back past its
// capacity) -- do not build this from a temporary.
class StarHipIndex {
  public:
    explicit StarHipIndex(const std::vector<Star>& stars);

    // The star with this Hipparcos number, or null if none is indexed: hip is 0 ("no Hipparcos
    // number", see Star::hip), or the star isn't in the catalog this index was built from (for
    // example, fainter than that catalog's magnitude cutoff).
    const Star* Find(int hip) const;

  private:
    std::unordered_map<int, const Star*> byHip_;
};

// A constellation line segment resolved to where its two endpoint stars currently sit in an
// observer's sky.
struct VisibleConstellationLine {
    HorizontalPosition a;
    HorizontalPosition b;
};

// The segments of `lines` whose two endpoint stars are both found (by Hipparcos number) in
// `stars` and both currently above the horizon (altitude >= 0 degrees) for an observer at
// latitude observerLatRad, at the given local sidereal time (see LocalSiderealTime in
// core/time.h), each resolved to its endpoints' horizontal positions. A segment is dropped
// entirely, not clipped, if either endpoint is missing from `stars` (for example, fainter than
// this catalog's magnitude cutoff) or below the horizon.
std::vector<VisibleConstellationLine> VisibleConstellationLines(
    const StarHipIndex& stars, const std::vector<ConstellationLine>& lines,
    double observerLatRad, double lstRad);

} // namespace core
