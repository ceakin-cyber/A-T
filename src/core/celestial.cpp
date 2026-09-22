#include "core/celestial.h"

#include <cmath>

namespace core {

Vec3 EquatorialToUnitVector(double raRad, double decRad) {
    const double cosDec = std::cos(decRad);
    return {cosDec * std::cos(raRad), cosDec * std::sin(raRad), std::sin(decRad)};
}

} // namespace core
