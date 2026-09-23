#pragma once

namespace app {

// The station's signal quality tier, derived from the planetary Kp index (see
// net::ParseMostRecentKp): how much geomagnetic activity is likely disrupting radio propagation.
enum class SignalQuality {
    Stable,    // Kp < 4: quiet geomagnetic conditions, normal propagation expected
    Degraded,  // 4 <= Kp <= 6: minor to moderate storm, some disruption possible
    Disrupted, // Kp > 6: strong geomagnetic storm, significant disruption likely
};

// Classifies a Kp index reading into a SignalQuality tier: Stable below 4, Degraded from 4 to 6
// inclusive, Disrupted above 6.
SignalQuality ClassifyKp(double kp);

} // namespace app
