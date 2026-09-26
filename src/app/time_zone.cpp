#include "app/time_zone.h"

#include <cstdlib>
#include <filesystem>
#include <optional>

namespace app {

namespace {

std::string& CurrentName() {
    static std::string name = kUtcTimeZone;
    return name;
}

// The TZ environment variable as it was before this app first changed it, so "LOCAL" can put it
// back: unset (nullopt) means the computer's own zone, as usual.
const std::optional<std::string>& OriginalTz() {
    static const std::optional<std::string> original = [] {
        const char* tz = std::getenv("TZ");
        return tz != nullptr ? std::optional<std::string>(tz) : std::nullopt;
    }();
    return original;
}

std::filesystem::path ZoneInfoDir() {
    const char* dir = std::getenv("TZDIR");
    return dir != nullptr && *dir != '\0' ? std::filesystem::path(dir)
                                          : std::filesystem::path("/usr/share/zoneinfo");
}

std::time_t ToTimeT(std::chrono::system_clock::time_point time) {
    return std::chrono::system_clock::to_time_t(
        std::chrono::time_point_cast<std::chrono::seconds>(time));
}

bool IsUtc() {
    return CurrentName() == kUtcTimeZone;
}

} // namespace

bool IsKnownTimeZone(const std::string& name) {
    if (name == kUtcTimeZone || name == kLocalTimeZone) {
        return true;
    }
    // Only plain relative names like "Europe/Paris": nothing that could step outside the zone
    // directory, and nothing starting with ':' or a digit, which TZ would read as something else.
    if (name.empty() || name.front() == '/' || name.front() == ':' ||
        name.find("..") != std::string::npos || (name.front() >= '0' && name.front() <= '9')) {
        return false;
    }
    std::error_code error;
    return std::filesystem::is_regular_file(ZoneInfoDir() / name, error);
}

bool SetDisplayTimeZone(const std::string& name) {
    if (!IsKnownTimeZone(name)) {
        return false;
    }
    OriginalTz(); // captured before the first change below
    if (name == kLocalTimeZone) {
        if (OriginalTz()) {
            setenv("TZ", OriginalTz()->c_str(), 1);
        } else {
            unsetenv("TZ");
        }
    } else if (name != kUtcTimeZone) {
        setenv("TZ", name.c_str(), 1);
    }
    tzset();
    CurrentName() = name;
    return true;
}

const std::string& DisplayTimeZone() {
    return CurrentName();
}

std::tm ToDisplayTime(std::chrono::system_clock::time_point time) {
    const std::time_t seconds = ToTimeT(time);
    std::tm parts{};
    if (IsUtc()) {
        gmtime_r(&seconds, &parts);
    } else {
        localtime_r(&seconds, &parts);
    }
    return parts;
}

std::string DisplayTimeZoneAbbreviation(std::chrono::system_clock::time_point time) {
    if (IsUtc()) {
        return kUtcTimeZone;
    }
    const std::tm parts = ToDisplayTime(time);
    char buffer[16];
    std::strftime(buffer, sizeof buffer, "%Z", &parts);
    std::string abbreviation = buffer;
    // Zones with no letters of their own report a bare offset such as "-03" or "+0530"; show
    // those all the same way, as "-0300" or "+0530".
    if (abbreviation.empty() || abbreviation.front() == '+' || abbreviation.front() == '-') {
        std::strftime(buffer, sizeof buffer, "%z", &parts);
        abbreviation = buffer;
    }
    return abbreviation;
}

} // namespace app
