#include "core/celestial.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace core {

Vec3 EquatorialToUnitVector(double raRad, double decRad) {
    const double cosDec = std::cos(decRad);
    return {cosDec * std::cos(raRad), cosDec * std::sin(raRad), std::sin(decRad)};
}

HorizontalPosition EquatorialToHorizontal(double raRad, double decRad, double observerLatRad,
                                          double lstRad) {
    const Vec3 equatorial = EquatorialToUnitVector(raRad, decRad);

    // Rotate about the polar axis by the local sidereal time (the same kind of rotation as
    // TemeToEcefFromGmst in core/frames.h, just by LST instead of GMST): this puts the x axis on
    // the observer's meridian, so the observer's longitude does not need to appear again below.
    const double c = std::cos(lstRad);
    const double s = std::sin(lstRad);
    const double mx = c * equatorial.x + s * equatorial.y;
    const double my = -s * equatorial.x + c * equatorial.y;
    const double mz = equatorial.z;

    // Tilt by the observer's latitude onto local East, North and Up (compare with EcefToEnu in
    // core/topocentric.h, which does the same East/North/Up projection for a satellite's ECEF
    // position; here there is no longitude term, since the rotation above already accounts for
    // it, and no position to subtract, since a star has no meaningful range).
    const double sinLat = std::sin(observerLatRad);
    const double cosLat = std::cos(observerLatRad);
    const double east = my;
    const double north = -sinLat * mx + cosLat * mz;
    const double up = cosLat * mx + sinLat * mz;

    HorizontalPosition position;
    position.altitudeRad = std::asin(std::clamp(up, -1.0, 1.0));
    position.azimuthRad = std::atan2(east, north);
    if (position.azimuthRad < 0.0) {
        position.azimuthRad += 2.0 * std::numbers::pi;
    }
    return position;
}

} // namespace core
