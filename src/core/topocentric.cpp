#include "core/topocentric.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace core {

Vec3 EcefToEnu(const Geodetic& observer, const Vec3& satelliteEcef) {
    const Vec3 site = GeodeticToEcef(observer);
    const double dx = satelliteEcef.x - site.x;
    const double dy = satelliteEcef.y - site.y;
    const double dz = satelliteEcef.z - site.z;

    const double sinLat = std::sin(observer.latitude);
    const double cosLat = std::cos(observer.latitude);
    const double sinLon = std::sin(observer.longitude);
    const double cosLon = std::cos(observer.longitude);

    return {
        -sinLon * dx + cosLon * dy,
        -sinLat * cosLon * dx - sinLat * sinLon * dy + cosLat * dz,
        cosLat * cosLon * dx + cosLat * sinLon * dy + sinLat * dz,
    };
}

LookAngles EcefToLookAngles(const Geodetic& observer, const Vec3& satelliteEcef) {
    const Vec3 enu = EcefToEnu(observer, satelliteEcef);

    LookAngles look;
    look.rangeKm = std::sqrt(enu.x * enu.x + enu.y * enu.y + enu.z * enu.z);
    if (look.rangeKm == 0.0) {
        return look;
    }

    look.elevation = std::asin(std::clamp(enu.z / look.rangeKm, -1.0, 1.0));
    look.azimuth = std::atan2(enu.x, enu.y);
    if (look.azimuth < 0.0) {
        look.azimuth += 2.0 * std::numbers::pi;
    }
    return look;
}

} // namespace core
