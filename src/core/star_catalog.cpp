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

namespace {

struct ColorAnchor {
    double colorIndex;
    Rgb color;
};

// Anchor points of the blue-to-red ramp, in ascending color index order. Chosen to look
// reasonable, not taken from a verified scientific color table (see the header comment on
// ColorIndexToRgb).
constexpr ColorAnchor kColorAnchors[] = {
    {-0.4, {0.61F, 0.70F, 1.00F}}, // hottest, blue
    {0.0, {0.80F, 0.85F, 1.00F}},  // blue-white
    {0.4, {1.00F, 0.98F, 0.95F}},  // white
    {0.7, {1.00F, 0.92F, 0.80F}},  // yellow-white (the Sun's own color index is 0.656)
    {1.0, {1.00F, 0.80F, 0.60F}},  // orange
    {1.6, {1.00F, 0.65F, 0.45F}},  // red-orange
    {2.0, {1.00F, 0.50F, 0.40F}},  // coolest, red
};

} // namespace

Rgb ColorIndexToRgb(double colorIndex) {
    colorIndex = std::clamp(colorIndex, kBluestColorIndex, kReddestColorIndex);

    constexpr std::size_t kAnchorCount = sizeof(kColorAnchors) / sizeof(kColorAnchors[0]);
    for (std::size_t i = 0; i + 1 < kAnchorCount; ++i) {
        const ColorAnchor& a = kColorAnchors[i];
        const ColorAnchor& b = kColorAnchors[i + 1];
        if (colorIndex <= b.colorIndex) {
            const double t = (colorIndex - a.colorIndex) / (b.colorIndex - a.colorIndex);
            const auto lerp = [t](float from, float to) {
                return static_cast<float>(from + t * (to - from));
            };
            return {lerp(a.color.r, b.color.r), lerp(a.color.g, b.color.g),
                    lerp(a.color.b, b.color.b)};
        }
    }
    return kColorAnchors[kAnchorCount - 1].color;
}

} // namespace core
