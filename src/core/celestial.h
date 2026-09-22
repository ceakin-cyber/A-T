#pragma once

#include "core/vec3.h"

namespace core {

// Converts a J2000 equatorial position (right ascension and declination, in radians) to a unit
// vector in the equatorial (ICRS/J2000-ish, ignoring precession) frame: x toward the vernal
// equinox, z toward the north celestial pole.
Vec3 EquatorialToUnitVector(double raRad, double decRad);

// Where an object appears in an observer's sky.
struct HorizontalPosition {
    double altitudeRad = 0.0; // above the horizon; negative if below it
    double azimuthRad = 0.0;  // radians from north, clockwise, in [0, 2*pi)
};

// Converts an equatorial position to altitude and azimuth for an observer at latitude
// observerLatRad, at the given local sidereal time (see LocalSiderealTime in core/time.h).
// Unlike the topocentric transform for satellites (core/topocentric.h), this has no range: a
// star is effectively at infinite distance, so only direction matters, and the observer's
// longitude does not appear separately here because it is already folded into the local
// sidereal time.
HorizontalPosition EquatorialToHorizontal(double raRad, double decRad, double observerLatRad,
                                          double lstRad);

} // namespace core
