#include "app/signal_quality.h"

namespace app {

SignalQuality ClassifyKp(double kp) {
    if (kp < 4.0) {
        return SignalQuality::Stable;
    }
    if (kp <= 6.0) {
        return SignalQuality::Degraded;
    }
    return SignalQuality::Disrupted;
}

const char* ToString(SignalQuality quality) {
    switch (quality) {
    case SignalQuality::Stable:
        return "STABLE";
    case SignalQuality::Degraded:
        return "DEGRADED";
    case SignalQuality::Disrupted:
        return "DISRUPTED";
    }
    return "UNKNOWN";
}

} // namespace app
