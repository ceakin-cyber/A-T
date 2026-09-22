#pragma once

#include "core/vec3.h"

namespace core {

// Converts a J2000 equatorial position (right ascension and declination, in radians) to a unit
// vector in the equatorial (ICRS/J2000-ish, ignoring precession) frame: x toward the vernal
// equinox, z toward the north celestial pole.
Vec3 EquatorialToUnitVector(double raRad, double decRad);

} // namespace core
