#pragma once

#include <optional>
#include <string>

namespace core {

// Orbital elements from a two-line element set. Angles are in degrees.
struct Tle {
    std::string name; // empty if the input had no name line
    int catalogNumber = 0;

    int epochYear = 0;     // full four-digit year
    double epochDay = 0.0; // day of year with fraction, 1.0 = Jan 1 00:00 UTC

    double inclination = 0.0;
    double raan = 0.0; // right ascension of the ascending node
    double eccentricity = 0.0;
    double argPerigee = 0.0;
    double meanAnomaly = 0.0;
    double meanMotion = 0.0; // revolutions per day
    double bstar = 0.0;      // drag term, in inverse Earth radii
};

// Parses a TLE given as two lines, or as a name line followed by two lines.
// Returns nullopt if the text is malformed: wrong line length or number, mismatched catalog
// numbers, a bad checksum, or a field that isn't a number.
std::optional<Tle> ParseTle(const std::string& text);

} // namespace core
