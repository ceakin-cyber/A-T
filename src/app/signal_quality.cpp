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

} // namespace app
