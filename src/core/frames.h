#pragma once

#include "core/vec3.h"

namespace core {

// Rotates a TEME position (inertial) into ECEF (rotating with the Earth) about the Z axis by the
// given Greenwich sidereal angle, in radians.
Vec3 TemeToEcefFromGmst(const Vec3& teme, double gmstRadians);

// Same, using the Greenwich mean sidereal time at the given Julian date (UTC, with UT1 taken to
// equal UTC). Use the same instant that the position was propagated to.
//
// Polar motion, which shifts a point on the ground by up to about 10 m, is ignored.
Vec3 TemeToEcef(const Vec3& teme, double julianDate);

} // namespace core
