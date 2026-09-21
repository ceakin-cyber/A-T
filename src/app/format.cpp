#include "app/format.h"

#include <cmath>
#include <cstdio>
#include <numbers>

namespace app {

namespace {

std::string Format(const char* pattern, double value, char suffix = '\0') {
    char buffer[32];
    if (suffix != '\0') {
        std::snprintf(buffer, sizeof buffer, pattern, value, suffix);
    } else {
        std::snprintf(buffer, sizeof buffer, pattern, value);
    }
    return buffer;
}

std::string FormatAngle(double radians, char positive, char negative) {
    const double degrees = radians * 180.0 / std::numbers::pi;
    return Format("%.4f %c", std::fabs(degrees), degrees >= 0.0 ? positive : negative);
}

} // namespace

std::string FormatLatitude(double radians) {
    return FormatAngle(radians, 'N', 'S');
}

std::string FormatLongitude(double radians) {
    return FormatAngle(radians, 'E', 'W');
}

std::string FormatAltitudeKm(double kilometers) {
    return Format("%.2f KM", kilometers);
}

std::string FormatSpeedKmPerSec(double kilometersPerSecond) {
    return Format("%.2f KM/S", kilometersPerSecond);
}

} // namespace app
