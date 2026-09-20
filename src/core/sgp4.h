#pragma once

#include "core/tle.h"

#include <cmath>
#include <optional>

namespace core {

// WGS-72 gravity constants. SGP4 is defined with these (not WGS-84), so the propagator must use
// them for its results to match those of other implementations.
namespace wgs72 {
inline constexpr double kMu = 398600.8;            // km^3 / s^2
inline constexpr double kRadiusEarthKm = 6378.135; // km
inline constexpr double kJ2 = 0.001082616;
inline constexpr double kJ3 = -0.00000253881;
inline constexpr double kJ4 = -0.00000165597;
inline constexpr double kJ3OverJ2 = kJ3 / kJ2;
// sqrt(mu) in units of Earth radii^1.5 per minute.
inline const double kXke = 60.0 / std::sqrt(kRadiusEarthKm * kRadiusEarthKm * kRadiusEarthKm / kMu);
} // namespace wgs72

// Everything SGP4 derives from a TLE once, before propagating. Distances are in Earth radii,
// angles in radians and mean motions in radians per minute, as in the reference implementation
// (Spacetrack Report #3 as revised by Vallado et al., 2006). Names in comments are theirs.
struct Sgp4Model {
    // Elements at epoch, converted to SGP4's units.
    double epochJd = 0.0; // Julian date of the TLE epoch
    double bstar = 0.0;
    double eccentricity = 0.0;    // ecco
    double inclination = 0.0;     // inclo
    double raan = 0.0;            // nodeo
    double argPerigee = 0.0;      // argpo
    double meanAnomaly = 0.0;     // mo
    double meanMotionKozai = 0.0; // no_kozai: the mean motion as published in the TLE

    // Recovered from the elements.
    double meanMotion = 0.0;     // no_unkozai: SGP4's own mean motion
    double semiMajorAxis = 0.0;  // ao / a
    double cosInclination = 0.0; // cosio
    double sinInclination = 0.0; // sinio
    double omeosq = 0.0;         // 1 - e^2
    double posq = 0.0;           // (a * (1 - e^2))^2
    double con41 = 0.0;          // 3 * cos^2(i) - 1
    double con42 = 0.0;          // 1 - 5 * cos^2(i)
    double perigeeKm = 0.0;      // perigee height above the Earth's surface
    double gsto = 0.0;           // Greenwich sidereal time at epoch

    // Drag model setup.
    bool isSimple = false; // isimp: perigee below 220 km, so higher-order drag terms are dropped
    double s = 0.0;        // sfour: atmospheric density parameter, adjusted for low perigees
    double qoms24 = 0.0;   // qzms24: (120 km - s)^4 in Earth radii
    double tsi = 0.0;      // 1 / (a - s)
    double eta = 0.0;      // a * e * tsi
};

// Initializes SGP4 from a TLE. Returns nullopt for deep-space orbits (period of 225 minutes or
// more), which need the SDP4 model, and for elements that cannot describe a bound orbit.
std::optional<Sgp4Model> InitSgp4(const Tle& tle);

} // namespace core
