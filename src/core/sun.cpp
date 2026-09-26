#include "core/sun.h"

#include "core/frames.h"

#include <cmath>
#include <numbers>

namespace core {

namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;
constexpr double kJ2000 = 2451545.0;

} // namespace

Vec3 SunDirectionEci(double julianDate) {
    const double n = julianDate - kJ2000; // days since J2000.0
    const double meanLongitude = (280.460 + 0.9856474 * n) * kDegToRad;
    const double meanAnomaly = (357.528 + 0.9856003 * n) * kDegToRad;
    const double eclipticLongitude =
        meanLongitude +
        (1.915 * std::sin(meanAnomaly) + 0.020 * std::sin(2.0 * meanAnomaly)) * kDegToRad;
    const double obliquity = (23.439 - 0.0000004 * n) * kDegToRad;
    return {std::cos(eclipticLongitude), std::cos(obliquity) * std::sin(eclipticLongitude),
            std::sin(obliquity) * std::sin(eclipticLongitude)};
}

Vec3 SunDirectionEcef(double julianDate) {
    return TemeToEcef(SunDirectionEci(julianDate), julianDate);
}

Geodetic SubsolarPoint(double julianDate) {
    const Vec3 sun = SunDirectionEcef(julianDate);
    return {std::asin(sun.z), std::atan2(sun.y, sun.x), 0.0};
}

bool IsSunlit(const Vec3& satelliteEcef, const Vec3& sunDirectionEcef) {
    const double towardSun = satelliteEcef.x * sunDirectionEcef.x +
                             satelliteEcef.y * sunDirectionEcef.y +
                             satelliteEcef.z * sunDirectionEcef.z;
    if (towardSun >= 0.0) {
        return true; // on the day side of the Earth's center: nothing between it and the Sun
    }
    // On the night side: in shadow only if within the Earth's radius of the Earth-Sun line.
    const double r2 = satelliteEcef.x * satelliteEcef.x + satelliteEcef.y * satelliteEcef.y +
                      satelliteEcef.z * satelliteEcef.z;
    const double offAxis2 = r2 - towardSun * towardSun;
    return offAxis2 > wgs84::kSemiMajorAxisKm * wgs84::kSemiMajorAxisKm;
}

} // namespace core
