#pragma once

#include "core/geodetic.h"
#include "core/vec3.h"

namespace core {

// Where a satellite appears in an observer's sky.
struct LookAngles {
    double azimuth = 0.0;   // radians from north, clockwise, in [0, 2*pi)
    double elevation = 0.0; // radians above the local horizon, in [-pi/2, pi/2]
    double rangeKm = 0.0;   // straight-line distance to the satellite
};

// The satellite's position relative to the observer, in the observer's local East, North and Up
// axes (km). "Up" is the direction of the ellipsoid normal, which is what defines the horizon.
Vec3 EcefToEnu(const Geodetic& observer, const Vec3& satelliteEcef);

// Azimuth, elevation and range of a satellite from an observer. Atmospheric refraction, which
// lifts the apparent elevation by up to about half a degree at the horizon, is not applied.
LookAngles EcefToLookAngles(const Geodetic& observer, const Vec3& satelliteEcef);

} // namespace core
