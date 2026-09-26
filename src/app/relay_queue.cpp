#include "app/relay_queue.h"

namespace app {

const char* ToString(RelayState state) {
    switch (state) {
    case RelayState::Sent:
        return "SENT";
    case RelayState::Hold:
        return "HOLD";
    case RelayState::Armed:
        return "ARMED";
    case RelayState::None:
        return "NONE";
    }
    return "UNKNOWN";
}

} // namespace app
