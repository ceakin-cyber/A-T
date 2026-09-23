#pragma once

#include "core/passes.h"

#include <optional>

namespace app {

// Whether `current` is a meaningfully different pass than `previous`: true if exactly one of the
// two is present, or if both are present but their rise times differ. Used to notice when
// PassPlanner::Next (via SatelliteRoster) has produced a genuinely new answer -- worth a "PASS
// COMPUTED" incoming-transmission event -- as opposed to a frame that just reused the same pass
// as before.
bool PassChanged(const std::optional<core::Pass>& previous, const std::optional<core::Pass>& current);

} // namespace app
