#pragma once

#include "core/vec3.h"

namespace core {

// WGS-84 reference ellipsoid, used for latitude, longitude and altitude. (SGP4 itself uses the
// slightly different WGS-72 constants; the difference is far below what tracking needs.)
namespace wgs84 {
inline constexpr double kSemiMajorAxisKm = 6378.137;
inline constexpr double kFlattening = 1.0 / 298.257223563;
inline constexpr double kSemiMinorAxisKm = kSemiMajorAxisKm * (1.0 - kFlattening);
} // namespace wgs84

// A position relative to the WGS-84 ellipsoid.
struct Geodetic {
    double latitude = 0.0;   // radians, [-pi/2, pi/2], positive north
    double longitude = 0.0;  // radians, (-pi, pi], positive east
    double altitudeKm = 0.0; // height above the ellipsoid
};

// Converts an ECEF position in km to geodetic coordinates.
Geodetic EcefToGeodetic(const Vec3& ecef);

// Converts geodetic coordinates to an ECEF position in km.
Vec3 GeodeticToEcef(const Geodetic& geodetic);

} // namespace core
