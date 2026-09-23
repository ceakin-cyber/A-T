#pragma once

#include "core/lunar.h"

#include <optional>

namespace ui {

// Draws the lunar alignment panel: the Moon's current phase name, illuminated fraction, age
// since the last new moon, and time until whichever of the next new/full moon comes first, as
// label/value rows in the same style as every other panel (see ui::LabelValueRow). `julianDate`
// is the moment `phase` was computed for (see core::LunarPhaseAt), used to measure the countdown
// to `nextNewMoonJd`/`nextFullMoonJd` (see core::NextNewMoon/NextFullMoon); either is nullopt
// only if that search failed, which should not happen in practice.
void DrawLunarAlignmentPanel(const core::LunarPhase& phase, double julianDate,
                            std::optional<double> nextNewMoonJd,
                            std::optional<double> nextFullMoonJd);

} // namespace ui
