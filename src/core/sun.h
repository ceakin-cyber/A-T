#pragma once

#include "core/geodetic.h"
#include "core/vec3.h"

namespace core {

// The mean distance from the Earth to the Sun, in km (one astronomical unit).
inline constexpr double kAstronomicalUnitKm = 149597870.7;

// A unit vector from the Earth's center toward the Sun at Julian date `julianDate`, in the
// equatorial frame of date (close enough to TEME, the frame SGP4 works in, that the two can be
// used interchangeably here: they differ by well under a degree). Uses the Astronomical
// Almanac's low-precision solar formula -- the Sun's mean longitude and anomaly plus its two
// largest equation-of-center terms -- good to about 0.01 degrees between 1950 and 2050, far
// finer than needed to tell day from night or sunlight from shadow.
Vec3 SunDirectionEci(double julianDate);

// The same direction in Earth-fixed (ECEF) coordinates, rotated by the Earth's spin.
Vec3 SunDirectionEcef(double julianDate);

// The point on the Earth where the Sun is directly overhead (altitude 0). Latitude is the Sun's
// declination; longitude follows the time of day.
Geodetic SubsolarPoint(double julianDate);

// Whether a satellite at `satelliteEcef` (km) is in sunlight rather than in the Earth's shadow,
// given the Sun's direction `sunDirectionEcef` (a unit vector). The shadow is modeled as a
// cylinder the width of the Earth's equator trailing directly away from the Sun: no penumbra, no
// flattening, no atmosphere. For a satellite in low Earth orbit that puts the edge of the shadow
// within a few seconds of the real one, which is plenty to say whether it can be seen.
bool IsSunlit(const Vec3& satelliteEcef, const Vec3& sunDirectionEcef);

} // namespace core
