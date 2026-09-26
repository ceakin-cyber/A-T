#pragma once

#include "core/lunar.h"
#include "net/tle_cache.h"

#include <optional>

namespace ui {

// Draws the lunar alignment panel: a picture of the Moon in its current phase -- lit on the side
// it appears lit from the observer's hemisphere (`observerLatitudeRad`; see app::MoonLitOnRight)
// -- beside the Moon's phase name, illuminated fraction, age since the last new moon, and time
// until whichever of the next new/full moon comes first, as label/value rows in the same style as
// every other panel (see ui::LabelValueRow), plus when that data was last refreshed.
//
// These values are a manual snapshot, not continuously live: `phase`, `julianDateAsOf` (the
// moment `phase` was computed for, and what the countdown to `nextNewMoonJd`/`nextFullMoonJd` is
// measured from) and `refreshedAt` only change when the caller recomputes them, which this
// function never does on its own. A REFRESH button lets the operator request a new snapshot;
// this returns true on the frame it is clicked, so the caller knows to recompute those values
// from the current real time (and update `refreshedAt` to match) before the next frame.
// `nextNewMoonJd`/`nextFullMoonJd` are nullopt only if that search failed, which should not
// happen in practice.
bool DrawLunarAlignmentPanel(const core::LunarPhase& phase, double julianDateAsOf,
                            std::optional<double> nextNewMoonJd,
                            std::optional<double> nextFullMoonJd,
                            net::Clock::time_point refreshedAt, double observerLatitudeRad);

} // namespace ui
