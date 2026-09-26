#pragma once

#include "core/geodetic.h"
#include "core/passes.h"
#include "core/sgp4.h"
#include "core/topocentric.h"

#include <numbers>
#include <optional>
#include <vector>

namespace app {

// Where the satellite appears in the observer's sky at Julian date `julianDate`, or nullopt if
// the propagator fails at that time.
std::optional<core::LookAngles> LookAnglesAt(const core::Sgp4Model& model,
                                             const core::Geodetic& observer, double julianDate);

// The Sun's elevation above the observer's horizon at Julian date `julianDate`, in radians.
double SunElevationAt(const core::Geodetic& observer, double julianDate);

// Whether the satellite can be seen with the naked eye from the observer's position, and if
// not, why not. Seeing a satellite by eye takes all three of: the satellite above the horizon,
// the observer's sky dark enough (the Sun at least kTwilightSunElevationRad below the horizon),
// and the satellite itself still in sunlight, so there is something for it to reflect -- which
// is why satellites like the ISS are seen in the hours after dusk and before dawn, and not in
// the middle of the night, when they are usually in the Earth's shadow too.
enum class Visibility {
    Visible,      // all three hold: look up and it is there
    BelowHorizon, // not in the observer's sky at all
    Daylight,     // up, but the sky is too bright to pick it out
    InShadow,     // up in a dark sky, but in the Earth's shadow, so not lit
};

// Civil twilight: once the Sun is this far below the horizon, the sky is dark enough to pick out
// a bright satellite. (Brighter ones, like the ISS, can be seen a little earlier; this is the
// usual cutoff for "visible pass" predictions.)
inline constexpr double kTwilightSunElevationRad = -6.0 * std::numbers::pi / 180.0;

// Classifies a moment from its three ingredients (see Visibility): the satellite's elevation
// and the Sun's, both in radians, and whether the satellite is in sunlight. Checked in that
// order, so the reason given is the first one that fails.
Visibility ClassifyVisibility(double satelliteElevationRad, double sunElevationRad, bool sunlit);

// The same, worked out for the satellite and observer at Julian date `julianDate`. Returns
// nullopt if the propagator fails.
std::optional<Visibility> VisibilityAt(const core::Sgp4Model& model, const core::Geodetic& observer,
                                       double julianDate);

// A short display name: "VISIBLE", "BELOW HORIZON", "DAYLIGHT" or "IN SHADOW".
const char* ToString(Visibility visibility);

// The part of a pass during which the satellite can be seen by eye (see Visibility), if any.
struct VisibleWindow {
    double startJd = 0.0;
    double endJd = 0.0;
    double maxElevationRad = 0.0; // the highest it gets while visible, not over the whole pass
};

// Samples `pass` every `stepSeconds` for the stretch during which the satellite is Visible.
// Returns nullopt if it never is (a daylight pass, or one entirely in the Earth's shadow), or if
// the propagator fails. A pass can be visible at the start and fade into shadow part-way through
// (or the reverse), but not come and go more than once, so a single window covers it.
std::optional<VisibleWindow> FindVisibleWindow(const core::Sgp4Model& model,
                                               const core::Geodetic& observer,
                                               const core::Pass& pass, double stepSeconds = 5.0);

// A pass that can be seen by eye, and the part of it that can be.
struct VisiblePass {
    core::Pass pass;
    VisibleWindow window;
};

// The first pass after `fromJd`, within the next `days`, that can be seen by eye (see
// FindVisibleWindow): often not the very next pass, since most happen in daylight or with the
// satellite in the Earth's shadow. A pass whose visible part is still going on at `fromJd`
// counts; one whose visible part is already over does not. Returns nullopt if there is none in
// that time, or if the propagator fails. This scans every pass in the span, so it is far too slow
// to call every frame; cache the result.
std::optional<VisiblePass> FindNextVisiblePass(const core::Sgp4Model& model,
                                               const core::Geodetic& observer, double fromJd,
                                               double days = 3.0);

// The nearest of the 8 principal compass points to azimuth `azimuthRad` (radians clockwise from
// north): "N", "NE", "E", "SE", "S", "SW", "W" or "NW".
const char* CompassPoint(double azimuthRad);

} // namespace app
