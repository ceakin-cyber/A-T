#include "net/tle_cache.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace net {

namespace fs = std::filesystem;

namespace {

constexpr const char* kHeaderKey = "fetched_at=";

fs::path CacheFile(const fs::path& dir, int noradId) {
    return dir / ("tle_" + std::to_string(noradId) + ".txt");
}

} // namespace

fs::path DefaultCacheDir() {
    if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg != nullptr && *xdg != '\0') {
        return fs::path(xdg) / "a-t";
    }
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return fs::path(home) / ".cache" / "a-t";
    }
    return fs::temp_directory_path() / "a-t";
}

std::optional<CachedTle> ReadTleCache(const fs::path& dir, int noradId) {
    std::ifstream file(CacheFile(dir, noradId));
    if (!file) {
        return std::nullopt;
    }

    std::string header;
    if (!std::getline(file, header) || header.rfind(kHeaderKey, 0) != 0) {
        return std::nullopt;
    }
    long long seconds = 0;
    try {
        std::size_t used = 0;
        seconds = std::stoll(header.substr(std::string(kHeaderKey).size()), &used);
        if (used + std::string(kHeaderKey).size() != header.size()) {
            return std::nullopt;
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }

    std::ostringstream body;
    body << file.rdbuf();
    if (body.str().empty()) {
        return std::nullopt;
    }
    return CachedTle{body.str(), Clock::time_point(std::chrono::seconds(seconds))};
}

bool WriteTleCache(const fs::path& dir, int noradId, const std::string& text,
                   Clock::time_point fetchedAt) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        return false;
    }

    // Write to a temporary file and rename, so a crash never leaves a half-written cache.
    const fs::path target = CacheFile(dir, noradId);
    fs::path temp = target;
    temp += ".tmp";
    {
        std::ofstream file(temp, std::ios::trunc);
        if (!file) {
            return false;
        }
        const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(fetchedAt);
        file << kHeaderKey << seconds.time_since_epoch().count() << '\n' << text;
        if (!file) {
            return false;
        }
    }
    fs::rename(temp, target, ec);
    return !ec;
}

bool IsFresh(Clock::time_point fetchedAt, Clock::time_point now, Clock::duration maxAge) {
    const Clock::duration age = now - fetchedAt;
    return age >= Clock::duration::zero() && age < maxAge;
}

} // namespace net
