#include "core/star_catalog.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace core {

namespace {

// Splits one CSV line, honoring double-quoted fields (which may contain commas) and the
// standard "" escape for a literal quote inside a quoted field.
std::vector<std::string> SplitCsvLine(std::string_view line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

std::optional<double> ParseDouble(const std::string& s) {
    if (s.empty()) {
        return std::nullopt;
    }
    double value = 0.0;
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || end != s.data() + s.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> ParseInt(const std::string& s) {
    if (s.empty()) {
        return std::nullopt;
    }
    int value = 0;
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || end != s.data() + s.size()) {
        return std::nullopt;
    }
    return value;
}

} // namespace

std::vector<Star> ParseStarCatalog(const std::string& text) {
    std::vector<Star> stars;

    std::istringstream stream(text);
    std::string headerLine;
    if (!std::getline(stream, headerLine)) {
        std::cerr << "Star catalog is empty\n";
        return stars;
    }
    // A trailing carriage return, if the file has CRLF line endings.
    if (!headerLine.empty() && headerLine.back() == '\r') {
        headerLine.pop_back();
    }
    const std::vector<std::string> header = SplitCsvLine(headerLine);

    std::unordered_map<std::string, std::size_t> columnIndex;
    for (std::size_t i = 0; i < header.size(); ++i) {
        columnIndex[header[i]] = i;
    }

    static const char* const kRequiredColumns[] = {"id", "rarad", "decrad", "mag", "ci"};
    for (const char* column : kRequiredColumns) {
        if (columnIndex.find(column) == columnIndex.end()) {
            std::cerr << "Star catalog is missing the required column '" << column << "'\n";
            return stars;
        }
    }
    const std::size_t idCol = columnIndex["id"];
    const std::size_t raCol = columnIndex["rarad"];
    const std::size_t decCol = columnIndex["decrad"];
    const std::size_t magCol = columnIndex["mag"];
    const std::size_t ciCol = columnIndex["ci"];
    const std::size_t minColumns = 1 + std::max({idCol, raCol, decCol, magCol, ciCol});

    std::string line;
    int lineNumber = 1;
    while (std::getline(stream, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = SplitCsvLine(line);
        if (fields.size() < minColumns) {
            std::cerr << "Star catalog line " << lineNumber << ": too few columns, skipping\n";
            continue;
        }

        const std::optional<int> id = ParseInt(fields[idCol]);
        const std::optional<double> ra = ParseDouble(fields[raCol]);
        const std::optional<double> dec = ParseDouble(fields[decCol]);
        const std::optional<double> mag = ParseDouble(fields[magCol]);
        if (!id || !ra || !dec || !mag) {
            std::cerr << "Star catalog line " << lineNumber
                      << ": missing or invalid id, rarad, decrad or mag, skipping\n";
            continue;
        }

        Star star;
        star.id = *id;
        star.raRad = *ra;
        star.decRad = *dec;
        star.magnitude = *mag;
        star.colorIndex = ParseDouble(fields[ciCol]).value_or(0.0);
        stars.push_back(star);
    }
    return stars;
}

std::vector<Star> LoadStarCatalog(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open star catalog at " << path << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseStarCatalog(buffer.str());
}

std::vector<VisibleStar> VisibleStars(const std::vector<Star>& stars, double observerLatRad,
                                      double lstRad) {
    std::vector<VisibleStar> visible;
    for (const Star& star : stars) {
        const HorizontalPosition position =
            EquatorialToHorizontal(star.raRad, star.decRad, observerLatRad, lstRad);
        if (position.altitudeRad >= 0.0) {
            visible.push_back({star, position});
        }
    }
    return visible;
}

StarPointStyle MagnitudeToPointStyle(double magnitude) {
    constexpr float kMinRadiusPx = 0.5F;
    constexpr float kMaxRadiusPx = 3.0F;
    constexpr float kMinBrightness = 0.3F;
    constexpr float kMaxBrightness = 1.0F;

    // 0 at the faintest styled magnitude, 1 at the brightest; clamped outside that range.
    const double span = kFaintestStyledMagnitude - kBrightestStyledMagnitude;
    double t = (kFaintestStyledMagnitude - magnitude) / span;
    t = std::clamp(t, 0.0, 1.0);

    StarPointStyle style;
    style.radiusPx = kMinRadiusPx + static_cast<float>(t) * (kMaxRadiusPx - kMinRadiusPx);
    style.brightness = kMinBrightness + static_cast<float>(t) * (kMaxBrightness - kMinBrightness);
    return style;
}

} // namespace core
