#pragma once

#include "core/tle.h"
#include "core/vec3.h"

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

    // Secular rates, from the J2 and J4 gravity terms (radians per minute).
    double mdot = 0.0;    // mean anomaly
    double argpdot = 0.0; // argument of perigee
    double nodedot = 0.0; // right ascension of the ascending node

    // Drag coefficients. The terms d2..d4 and t3cof..t5cof stay zero for simple orbits.
    double cc1 = 0.0;
    double cc4 = 0.0;
    double cc5 = 0.0;
    double omgcof = 0.0;
    double xmcof = 0.0;
    double nodecf = 0.0;
    double t2cof = 0.0;
    double delmo = 0.0;
    double sinmao = 0.0;
    double d2 = 0.0;
    double d3 = 0.0;
    double d4 = 0.0;
    double t3cof = 0.0;
    double t4cof = 0.0;
    double t5cof = 0.0;

    // Coefficients for the long- and short-period corrections.
    double aycof = 0.0;
    double xlcof = 0.0;
    double x1mth2 = 0.0; // 1 - cos^2(i)
    double x7thm1 = 0.0; // 7 * cos^2(i) - 1
};

// Initializes SGP4 from a TLE. Returns nullopt for deep-space orbits (period of 225 minutes or
// more), which need the SDP4 model, and for elements that cannot describe a bound orbit.
std::optional<Sgp4Model> InitSgp4(const Tle& tle);

// Mean orbital elements at some time after (or before) the epoch, once secular gravity and drag
// effects are applied. Angles are in radians, in [0, 2*pi).
struct MeanElements {
    double semiMajorAxis = 0.0; // am, in Earth radii
    double eccentricity = 0.0;  // em
    double inclination = 0.0;   // inclm
    double raan = 0.0;          // nodem
    double argPerigee = 0.0;    // argpm
    double meanAnomaly = 0.0;   // mm
    double meanMotion = 0.0;    // nm, radians per minute
};

// Evolves the epoch elements by the given number of minutes (negative goes back in time).
// Returns nullopt if drag drives the eccentricity out of range or the mean motion to zero.
std::optional<MeanElements> PropagateSecular(const Sgp4Model& model, double minutesSinceEpoch);

// Position and velocity in the TEME frame (true equator, mean equinox of date): an inertial
// frame, not one that rotates with the Earth.
struct StateVector {
    Vec3 position; // km
    Vec3 velocity; // km/s
};

// Full SGP4: propagates the model to the given number of minutes since the TLE epoch (negative
// goes back in time). Returns nullopt if the orbit cannot be propagated to that time: drag has
// destroyed the elements, the semi-latus rectum went negative, or the satellite has decayed
// below the Earth's surface.
std::optional<StateVector> Propagate(const Sgp4Model& model, double minutesSinceEpoch);

} // namespace core
