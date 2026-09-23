#include "app/format.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
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

std::string FormatElevation(double radians) {
    return Format("%.1f DEG", radians * 180.0 / std::numbers::pi);
}

std::string FormatAltitudeKm(double kilometers) {
    return Format("%.2f KM", kilometers);
}

std::string FormatSpeedKmPerSec(double kilometersPerSecond) {
    return Format("%.2f KM/S", kilometersPerSecond);
}

std::string FormatUtcTime(std::chrono::system_clock::time_point time) {
    const std::time_t seconds = std::chrono::system_clock::to_time_t(
        std::chrono::time_point_cast<std::chrono::seconds>(time));
    std::tm parts{};
    gmtime_r(&seconds, &parts);
    char buffer[32];
    std::strftime(buffer, sizeof buffer, "%m-%d %H:%M:%S", &parts);
    return buffer;
}

std::string FormatCountdown(std::chrono::duration<double> duration) {
    const std::int64_t total = static_cast<std::int64_t>(std::max(0.0, duration.count()));
    const std::int64_t days = total / 86400;
    const std::int64_t hours = (total % 86400) / 3600;
    const std::int64_t minutes = (total % 3600) / 60;
    const std::int64_t seconds = total % 60;

    char buffer[32];
    if (days > 0) {
        std::snprintf(buffer, sizeof buffer, "%lldD %lldH", static_cast<long long>(days),
                      static_cast<long long>(hours));
    } else if (hours > 0) {
        std::snprintf(buffer, sizeof buffer, "%lldH %lldM", static_cast<long long>(hours),
                      static_cast<long long>(minutes));
    } else if (minutes > 0) {
        std::snprintf(buffer, sizeof buffer, "%lldM %02lldS", static_cast<long long>(minutes),
                      static_cast<long long>(seconds));
    } else {
        std::snprintf(buffer, sizeof buffer, "%lldS", static_cast<long long>(seconds));
    }
    return buffer;
}

std::string FormatIlluminatedFraction(double fraction) {
    return Format("%.0f%%", fraction * 100.0);
}

std::string FormatNodeMode(net::TleSource source) {
    switch (source) {
    case net::TleSource::Network:
        return "LIVE";
    case net::TleSource::FreshCache:
        return "CACHED";
    case net::TleSource::StaleCache:
        return "LOW-VISIBILITY";
    }
    return "UNKNOWN";
}

} // namespace app
