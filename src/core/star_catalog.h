#pragma once

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

// Parses a star catalog in the HYG Database's CSV format. The header row is required, and
// columns are found by name ("id", "rarad", "decrad", "mag", "ci"), not fixed position, so a
// future catalog revision that reorders columns is not silently misread. A row that cannot be
// parsed, or is missing one of the required columns, is skipped, with a reason printed to
// stderr; it does not stop the rest of the file from loading.
std::vector<Star> ParseStarCatalog(const std::string& text);

// Reads and parses the catalog from a file. Returns an empty list, after printing the reason to
// stderr, if the file cannot be read.
std::vector<Star> LoadStarCatalog(const std::filesystem::path& path);

} // namespace core
