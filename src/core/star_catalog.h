#pragma once

#include "core/celestial.h"

#include <filesystem>
#include <string>
#include <vector>

namespace core {

struct Star {
    int id = 0;
    double raRad = 0.0;      // right ascension, radians
    double decRad = 0.0;     // declination, radians
    double magnitude = 0.0;  // apparent magnitude; smaller is brighter
    double colorIndex = 0.0; // B-V color index; 0.0 (neutral/white) if the catalog left it blank
};

// A star together with where it currently sits in an observer's sky.
struct VisibleStar {
    Star star;
    HorizontalPosition position;
};

// Parses a star catalog in the HYG Database's CSV format. The header row is required, and
// columns are found by name ("id", "rarad", "decrad", "mag", "ci"), not fixed position, so a
// future catalog revision that reorders columns is not silently misread. A row that cannot be
// parsed, or is missing one of the required columns, is skipped, with a reason printed to
// stderr; it does not stop the rest of the file from loading.
std::vector<Star> ParseStarCatalog(const std::string& text);

// Reads and parses the catalog from a file. Returns an empty list, after printing the reason to
// stderr, if the file cannot be read.
std::vector<Star> LoadStarCatalog(const std::filesystem::path& path);

// The stars from `stars` that are currently above the horizon (altitude >= 0 degrees) for an
// observer at latitude observerLatRad, at the given local sidereal time (see LocalSiderealTime
// in core/time.h), each paired with its computed position. Order is not otherwise changed.
std::vector<VisibleStar> VisibleStars(const std::vector<Star>& stars, double observerLatRad,
                                      double lstRad);

} // namespace core
