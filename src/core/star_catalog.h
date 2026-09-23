#pragma once

#include "core/celestial.h"

#include <filesystem>
#include <string>
#include <vector>

namespace core {

struct Star {
    int id = 0;
    int hip = 0;              // Hipparcos catalog number; 0 if the catalog left it blank (as for
                               // the Sun, which has none). Used to match this star against
                               // constellation line data (see core/constellation.h).
    double raRad = 0.0;       // right ascension, radians
    double decRad = 0.0;      // declination, radians
    double magnitude = 0.0;   // apparent magnitude; smaller is brighter
    double colorIndex = 0.0;  // B-V color index; 0.0 (neutral/white) if the catalog left it blank
    std::string properName;   // common name, e.g. "Sirius"; empty if the catalog left it blank
                               // (true of most stars -- only the brightest and best-known have
                               // one)
};

// A star together with where it currently sits in an observer's sky.
struct VisibleStar {
    Star star;
    HorizontalPosition position;
};

// Parses a star catalog in the HYG Database's CSV format. The header row is required, and
// columns are found by name ("id", "hip", "rarad", "decrad", "mag", "ci", "proper"), not fixed
// position, so a future catalog revision that reorders columns is not silently misread. A row
// that cannot be parsed, or is missing one of the required columns, is skipped, with a reason
// printed to stderr; it does not stop the rest of the file from loading.
std::vector<Star> ParseStarCatalog(const std::string& text);

// Reads and parses the catalog from a file. Returns an empty list, after printing the reason to
// stderr, if the file cannot be read.
std::vector<Star> LoadStarCatalog(const std::filesystem::path& path);

// The stars from `stars` that are currently above the horizon (altitude >= 0 degrees) for an
// observer at latitude observerLatRad, at the given local sidereal time (see LocalSiderealTime
// in core/time.h), each paired with its computed position. Order is not otherwise changed.
std::vector<VisibleStar> VisibleStars(const std::vector<Star>& stars, double observerLatRad,
                                      double lstRad);

// How a star should be drawn: its point radius in pixels, and a 0-1 brightness multiplier on
// its draw color. A magnitude of kBrightestStyledMagnitude or brighter (numerically smaller,
// even negative) gives the largest, fullest-brightness style; kFaintestStyledMagnitude or
// dimmer gives the smallest, dimmest style. Magnitude outside that range is clamped to the
// nearer endpoint, so an unusually bright or faint object still gets a sensible, bounded style.
struct StarPointStyle {
    float radiusPx = 1.0F;
    float brightness = 1.0F;
};

inline constexpr double kBrightestStyledMagnitude = -1.5; // comfortably covers Sirius, -1.46
inline constexpr double kFaintestStyledMagnitude = 6.0;   // matches the catalog's own cutoff

StarPointStyle MagnitudeToPointStyle(double magnitude);

// A color, components in [0, 1].
struct Rgb {
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;
};

inline constexpr double kBluestColorIndex = -0.4; // hottest real stars (spectral type O)
inline constexpr double kReddestColorIndex = 2.0; // coolest real stars (spectral type M)

// An approximate color for a star's B-V color index: blue for the hottest stars, through white
// near a color index of about 0.4, to red for the coolest. This is a stylistic approximation
// hand-picked to look reasonable, not a value taken from a verified scientific color table;
// colorIndex outside [kBluestColorIndex, kReddestColorIndex] is clamped to the nearer endpoint.
Rgb ColorIndexToRgb(double colorIndex);

// The faintest magnitude worth labeling on the star map, named or not. Every star in the shipped
// catalog has a name or not more or less independently of magnitude, and there are far more named
// stars (in the hundreds) than there is room to label without the sky turning into a wall of
// overlapping text (there is no label collision handling), so this gates both cases rather than
// only the unnamed one -- roughly the four dozen best-known naked-eye stars (Sirius, Vega,
// Betelgeuse, and the like), not every star the catalog happens to have a name for.
inline constexpr double kLabelMagnitude = 2.0;

// The text to label a star with on the star map, or empty if it should not be labeled. A star
// fainter than kLabelMagnitude is never labeled, name or not; one at or brighter than it is
// labeled with its proper name if the catalog gave it one, else its magnitude to one decimal
// place.
std::string StarLabel(const Star& star);

} // namespace core
