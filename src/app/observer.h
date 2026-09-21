#pragma once

#include "core/geodetic.h"

namespace app {

// Where the observer is standing, for pass predictions and sky views. This is a placeholder at
// the Royal Observatory in Greenwich; change it to your own location. A config file will
// replace it later.
inline constexpr core::Geodetic kObserver = core::GeodeticFromDegrees(51.4779, 0.0, 0.062);

} // namespace app
