#include "core/frames.h"

#include "core/time.h"

#include <cmath>

namespace core {

Vec3 TemeToEcefFromGmst(const Vec3& teme, double gmstRadians) {
    const double c = std::cos(gmstRadians);
    const double s = std::sin(gmstRadians);
    return {c * teme.x + s * teme.y, -s * teme.x + c * teme.y, teme.z};
}

Vec3 TemeToEcef(const Vec3& teme, double julianDate) {
    return TemeToEcefFromGmst(teme, GreenwichMeanSiderealTime(julianDate));
}

} // namespace core
