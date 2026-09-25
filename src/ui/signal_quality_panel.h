#pragma once

#include "net/kp_index_source.h"

#include <optional>

namespace ui {

// Draws the signal quality panel: the station's signal tier (see app::ClassifyKp), driven by the
// real planetary Kp index it is classified from, as label/value rows in the same style as every
// other panel (see ui::LabelValueRow). The tier stands out in the warning color when DEGRADED and
// the critical color when DISRUPTED, like the header's own MODE/NODE coloring. `kp` is nullopt
// when no reading could be loaded at all (no network and no cache), which shows as NO DATA.
void DrawSignalQualityPanel(const std::optional<net::LoadedKp>& kp);

} // namespace ui
