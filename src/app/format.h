#pragma once

#include <string>

namespace app {

// Text for the tracker panel. Angles come in as radians and go out in degrees, with the
// hemisphere spelled as a letter, for example "40.8729 N" or "38.1415 W".
std::string FormatLatitude(double radians);
std::string FormatLongitude(double radians);

// "420.16 KM" and "7.66 KM/S".
std::string FormatAltitudeKm(double kilometers);
std::string FormatSpeedKmPerSec(double kilometersPerSecond);

} // namespace app
