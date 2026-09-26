#include "app/moon_disk.h"

#include "core/lunar.h"

#include <algorithm>
#include <cmath>

namespace app {

bool MoonLitOnRight(double ageDays, double observerLatitudeRad) {
    const bool waxing = ageDays < core::SynodicMonthDays() / 2.0;
    const bool southern = observerLatitudeRad < 0.0;
    return waxing != southern;
}

LitSpan MoonLitSpan(double y, double illuminatedFraction, bool litOnRight) {
    const double halfWidth = std::sqrt(std::max(0.0, 1.0 - y * y));
    const double fraction = std::clamp(illuminatedFraction, 0.0, 1.0);
    // Where the terminator crosses this slice, measured from the middle toward the lit side: on
    // the lit edge itself at k = 0 (nothing lit), through the middle at k = 0.5, and out on the
    // opposite edge at k = 1 (all lit).
    const double terminator = halfWidth * (1.0 - 2.0 * fraction);
    if (litOnRight) {
        return {terminator, halfWidth};
    }
    return {-halfWidth, -terminator};
}

} // namespace app
