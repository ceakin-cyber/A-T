#include "app/system_status.h"

namespace app {

std::string SystemState(bool anySatelliteLoaded) {
    return anySatelliteLoaded ? "ONLINE" : "OFFLINE";
}

} // namespace app
