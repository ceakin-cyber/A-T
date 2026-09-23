#include "net/kp_index_cache.h"

#include <charconv>
#include <fstream>
#include <system_error>

namespace net {

namespace fs = std::filesystem;

namespace {

constexpr const char* kHeaderKey = "fetched_at=";

fs::path CacheFile(const fs::path& dir) {
    return dir / "kp_index.txt";
}

} // namespace

std::optional<CachedKp> ReadKpCache(const fs::path& dir) {
    std::ifstream file(CacheFile(dir));
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

    std::string valueLine;
    if (!std::getline(file, valueLine)) {
        return std::nullopt;
    }
    double kp = 0.0;
    const auto [end, ec] =
        std::from_chars(valueLine.data(), valueLine.data() + valueLine.size(), kp);
    if (ec != std::errc() || end != valueLine.data() + valueLine.size()) {
        return std::nullopt;
    }

    return CachedKp{kp, Clock::time_point(std::chrono::seconds(seconds))};
}

bool WriteKpCache(const fs::path& dir, double kp, Clock::time_point fetchedAt) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        return false;
    }

    // Write to a temporary file and rename, so a crash never leaves a half-written cache.
    const fs::path target = CacheFile(dir);
    fs::path temp = target;
    temp += ".tmp";
    {
        std::ofstream file(temp, std::ios::trunc);
        if (!file) {
            return false;
        }
        const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(fetchedAt);
        file << kHeaderKey << seconds.time_since_epoch().count() << '\n' << kp << '\n';
        if (!file) {
            return false;
        }
    }
    fs::rename(temp, target, ec);
    return !ec;
}

} // namespace net
