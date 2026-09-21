#pragma once

#include "core/geodetic.h"
#include "core/sgp4.h"

#include <optional>
#include <vector>

namespace core {

// The stretch of time during which a satellite is above an observer's horizon.
struct Pass {
    double riseJd = 0.0; // Julian date when the elevation crosses above 0 degrees
    double setJd = 0.0;  // Julian date when it crosses back below

    // The satellite was already above the horizon when the search window began, so the real rise
    // was earlier and riseJd is the start of the window.
    bool risesBeforeWindow = false;
    // The search window ended while the satellite was still up, so the real set is later and
    // setJd is the end of the window.
    bool setsAfterWindow = false;

    // The highest point of the pass, within the search window. If the pass was cut off by the
    // window, this is the highest point inside it, which may be at the edge.
    double maxElevationJd = 0.0;
    double maxElevation = 0.0; // radians
};

// The highest elevation reached in an interval, and when.
struct ElevationPeak {
    double julianDate = 0.0;
    double elevation = 0.0; // radians
};

// Elevation in radians of the satellite above the observer's horizon at a Julian date (UTC, with
// UT1 taken to equal UTC), or nullopt if the propagator fails at that time.
std::optional<double> ElevationAt(const Sgp4Model& model, const Geodetic& observer,
                                  double julianDate);

// Finds the highest elevation the satellite reaches between startJd and endJd, and when, to about
// 0.05 s. It assumes a single peak in the interval, which holds within one pass. Returns nullopt
// if the interval is inverted or the propagator fails inside it.
std::optional<ElevationPeak> FindMaxElevation(const Sgp4Model& model, const Geodetic& observer,
                                              double startJd, double endJd);

// Finds every pass between startJd and endJd by stepping through time and watching for the
// elevation to change sign, then narrowing each crossing to about 0.05 s by bisection. Each pass
// also carries the time and elevation of its peak.
//
// A pass that stays above the horizon for less than one step can be missed. The default of 20 s
// only affects grazing passes of a few seconds. If the propagator fails part-way through, the
// scan stops and returns the passes found so far.
std::vector<Pass> FindPasses(const Sgp4Model& model, const Geodetic& observer, double startJd,
                             double endJd, double stepSeconds = 20.0);

} // namespace core
