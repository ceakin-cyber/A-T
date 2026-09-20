#include "core/tle.h"

#include <cctype>
#include <charconv>
#include <string_view>
#include <vector>

namespace core {

namespace {

constexpr std::size_t kLineLength = 69;

std::string_view Trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

std::vector<std::string_view> SplitLines(std::string_view text) {
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        const std::size_t end = text.find('\n');
        const std::string_view line = Trim(text.substr(0, end));
        if (!line.empty()) {
            lines.push_back(line);
        }
        if (end == std::string_view::npos) {
            break;
        }
        text.remove_prefix(end + 1);
    }
    return lines;
}

// Column numbers below are 1-based, as in the TLE format specification.
std::string_view Field(std::string_view line, std::size_t firstColumn, std::size_t lastColumn) {
    return Trim(line.substr(firstColumn - 1, lastColumn - firstColumn + 1));
}

bool ParseInt(std::string_view s, int& out) {
    s = Trim(s);
    if (!s.empty() && s.front() == '+') {
        s.remove_prefix(1);
    }
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc() && end == s.data() + s.size();
}

bool ParseDouble(std::string_view s, double& out) {
    s = Trim(s);
    if (!s.empty() && s.front() == '+') {
        s.remove_prefix(1);
    }
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc() && end == s.data() + s.size();
}

// Eccentricity is stored without its decimal point: "0004820" means 0.0004820.
bool ParseImpliedDecimal(std::string_view s, double& out) {
    s = Trim(s);
    if (s.empty()) {
        return false;
    }
    for (const char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return ParseDouble("0." + std::string(s), out);
}

// BSTAR is stored as a signed mantissa with an implied leading decimal point, then a signed
// exponent: " 14267-3" means 0.14267e-3 and "-11606-4" means -0.11606e-4.
bool ParseExponentField(std::string_view s, double& out) {
    s = Trim(s);
    if (s.size() < 3) {
        return false;
    }

    double sign = 1.0;
    if (s.front() == '-' || s.front() == '+') {
        sign = s.front() == '-' ? -1.0 : 1.0;
        s.remove_prefix(1);
    }
    if (s.size() < 3) {
        return false;
    }

    const std::string_view mantissa = s.substr(0, s.size() - 2);
    int exponent = 0;
    if (!ParseInt(s.substr(s.size() - 2), exponent)) {
        return false;
    }

    double fraction = 0.0;
    if (!ParseImpliedDecimal(mantissa, fraction)) {
        return false;
    }

    double scale = 1.0;
    for (int i = 0; i < (exponent < 0 ? -exponent : exponent); ++i) {
        scale *= 10.0;
    }
    out = sign * (exponent < 0 ? fraction / scale : fraction * scale);
    return true;
}

// Checksum: sum of all digits in the first 68 columns, counting each '-' as 1, modulo 10.
bool ChecksumMatches(std::string_view line) {
    int sum = 0;
    for (std::size_t i = 0; i < kLineLength - 1; ++i) {
        const char c = line[i];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            sum += c - '0';
        } else if (c == '-') {
            sum += 1;
        }
    }
    const char expected = line[kLineLength - 1];
    return std::isdigit(static_cast<unsigned char>(expected)) && expected - '0' == sum % 10;
}

} // namespace

std::optional<Tle> ParseTle(const std::string& text) {
    const std::vector<std::string_view> lines = SplitLines(text);
    if (lines.size() != 2 && lines.size() != 3) {
        return std::nullopt;
    }

    Tle tle;
    if (lines.size() == 3) {
        tle.name = std::string(lines[0]);
    }
    const std::string_view line1 = lines[lines.size() - 2];
    const std::string_view line2 = lines[lines.size() - 1];

    if (line1.size() != kLineLength || line2.size() != kLineLength) {
        return std::nullopt;
    }
    if (line1[0] != '1' || line2[0] != '2') {
        return std::nullopt;
    }
    if (!ChecksumMatches(line1) || !ChecksumMatches(line2)) {
        return std::nullopt;
    }

    int catalogNumber2 = 0;
    int yearTwoDigit = 0;
    const bool ok = ParseInt(Field(line1, 3, 7), tle.catalogNumber) &&
                    ParseInt(Field(line2, 3, 7), catalogNumber2) &&
                    ParseInt(Field(line1, 19, 20), yearTwoDigit) &&
                    ParseDouble(Field(line1, 21, 32), tle.epochDay) &&
                    ParseExponentField(Field(line1, 54, 61), tle.bstar) &&
                    ParseDouble(Field(line2, 9, 16), tle.inclination) &&
                    ParseDouble(Field(line2, 18, 25), tle.raan) &&
                    ParseImpliedDecimal(Field(line2, 27, 33), tle.eccentricity) &&
                    ParseDouble(Field(line2, 35, 42), tle.argPerigee) &&
                    ParseDouble(Field(line2, 44, 51), tle.meanAnomaly) &&
                    ParseDouble(Field(line2, 53, 63), tle.meanMotion);
    if (!ok || tle.catalogNumber != catalogNumber2) {
        return std::nullopt;
    }

    // Two-digit years follow the convention 57-99 -> 1957-1999, 00-56 -> 2000-2056.
    tle.epochYear = yearTwoDigit < 57 ? 2000 + yearTwoDigit : 1900 + yearTwoDigit;
    return tle;
}

} // namespace core
