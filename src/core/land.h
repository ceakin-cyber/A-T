#pragma once

#include "core/geodetic.h"

#include <filesystem>
#include <string>
#include <vector>

namespace core {

// One land mass (or island) outline: a closed ring of points on the ground, in order, the last
// joining back to the first. Altitude is always 0. A hole is a stretch of water enclosed by the
// land around it (a lake or inland sea, such as the Caspian), to be left unfilled.
struct LandRing {
    std::vector<Geodetic> points;
    bool hole = false;
};

// Parses this project's land outline format (see assets/world/SOURCE.md): one ring per line, as
// space-separated "longitude,latitude" pairs in degrees, with "hole" first on the line for a
// hole. Blank lines and lines starting with '#' are ignored. A line with a pair that cannot be
// read, or with fewer than three points (not enough to enclose anything), is skipped, with a reason
// printed to stderr; it does not stop the rest of the file from loading.
std::vector<LandRing> ParseLandOutlines(const std::string& text);

// Reads and parses the outlines from a file. Returns an empty list, after printing the reason to
// stderr, if the file cannot be read; the map is then drawn without land.
std::vector<LandRing> LoadLandOutlines(const std::filesystem::path& path);

} // namespace core
