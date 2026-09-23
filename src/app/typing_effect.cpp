#include "app/typing_effect.h"

#include <algorithm>

namespace app {

std::string TypingEffect(const std::string& fullText, std::chrono::duration<double> elapsed,
                         double charsPerSecond) {
    if (charsPerSecond <= 0.0) {
        return fullText;
    }
    const double visibleChars = elapsed.count() * charsPerSecond;
    if (visibleChars <= 0.0) {
        return "";
    }
    if (visibleChars >= static_cast<double>(fullText.size())) {
        return fullText;
    }
    return fullText.substr(0, static_cast<std::size_t>(visibleChars));
}

std::vector<net::Clock::time_point> TypingStartTimes(const std::deque<LogEntry>& entries,
                                                      double charsPerSecond) {
    std::vector<net::Clock::time_point> startTimes;
    startTimes.reserve(entries.size());

    net::Clock::time_point previousFinish{};
    bool first = true;
    for (const LogEntry& entry : entries) {
        const net::Clock::time_point start =
            first ? entry.time : std::max(entry.time, previousFinish);
        startTimes.push_back(start);

        const double secondsToType =
            charsPerSecond > 0.0 ? static_cast<double>(entry.message.size()) / charsPerSecond
                                 : 0.0;
        previousFinish = start + std::chrono::duration_cast<net::Clock::duration>(
                                     std::chrono::duration<double>(secondsToType));
        first = false;
    }
    return startTimes;
}

} // namespace app
