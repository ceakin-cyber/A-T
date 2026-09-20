#include "core/geodetic.h"

#include <cmath>
#include <numbers>

namespace core {

namespace {

constexpr double kEccentricitySquared = wgs84::kFlattening * (2.0 - wgs84::kFlattening);

// Radius of curvature in the prime vertical at the given latitude.
double PrimeVerticalRadius(double sinLatitude) {
    return wgs84::kSemiMajorAxisKm /
           std::sqrt(1.0 - kEccentricitySquared * sinLatitude * sinLatitude);
}

} // namespace

Geodetic EcefToGeodetic(const Vec3& ecef) {
    const double p = std::hypot(ecef.x, ecef.y);

    // Iterate on the latitude, starting from the value for a point on the surface.
    double latitude = std::atan2(ecef.z, p * (1.0 - kEccentricitySquared));
    for (int iteration = 0; iteration < 10; ++iteration) {
        const double n = PrimeVerticalRadius(std::sin(latitude));
        const double next = std::atan2(ecef.z + kEccentricitySquared * n * std::sin(latitude), p);
        const bool converged = std::fabs(next - latitude) < 1e-13;
        latitude = next;
        if (converged) {
            break;
        }
    }

    Geodetic result;
    result.latitude = latitude;
    result.longitude = std::atan2(ecef.y, ecef.x);

    // p / cos(latitude) loses accuracy near the poles, where the cosine goes to zero, so use the
    // formula in z there instead.
    const double sinLatitude = std::sin(latitude);
    const double n = PrimeVerticalRadius(sinLatitude);
    if (std::fabs(latitude) < std::numbers::pi / 4.0) {
        result.altitudeKm = p / std::cos(latitude) - n;
    } else {
        result.altitudeKm = ecef.z / sinLatitude - n * (1.0 - kEccentricitySquared);
    }
    return result;
}

Vec3 GeodeticToEcef(const Geodetic& geodetic) {
    const double sinLatitude = std::sin(geodetic.latitude);
    const double cosLatitude = std::cos(geodetic.latitude);
    const double n = PrimeVerticalRadius(sinLatitude);
    const double radial = (n + geodetic.altitudeKm) * cosLatitude;
    return {radial * std::cos(geodetic.longitude), radial * std::sin(geodetic.longitude),
            (n * (1.0 - kEccentricitySquared) + geodetic.altitudeKm) * sinLatitude};
}

} // namespace core
