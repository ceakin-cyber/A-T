#include "core/constellation.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace core {

namespace {

// Splits one CSV line on commas. Unlike the star catalog's parser (core/star_catalog.cpp), this
// format's fields -- a three-letter constellation abbreviation and two Hipparcos numbers -- never
// contain a comma or a quote, so no quote-handling is needed here.
std::vector<std::string> SplitCsvLine(std::string_view line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string_view::npos) {
            fields.emplace_back(line.substr(start));
            break;
        }
        fields.emplace_back(line.substr(start, comma - start));
        start = comma + 1;
    }
    return fields;
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

std::vector<ConstellationLine> ParseConstellationLines(const std::string& text) {
    std::vector<ConstellationLine> lines;

    std::istringstream stream(text);
    std::string headerLine;
    if (!std::getline(stream, headerLine)) {
        std::cerr << "Constellation line data is empty\n";
        return lines;
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

    static const char* const kRequiredColumns[] = {"con", "hip1", "hip2"};
    for (const char* column : kRequiredColumns) {
        if (columnIndex.find(column) == columnIndex.end()) {
            std::cerr << "Constellation line data is missing the required column '" << column
                      << "'\n";
            return lines;
        }
    }
    const std::size_t conCol = columnIndex["con"];
    const std::size_t hip1Col = columnIndex["hip1"];
    const std::size_t hip2Col = columnIndex["hip2"];
    const std::size_t minColumns = 1 + std::max({conCol, hip1Col, hip2Col});

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
            std::cerr << "Constellation line data line " << lineNumber
                      << ": too few columns, skipping\n";
            continue;
        }

        const std::optional<int> hip1 = ParseInt(fields[hip1Col]);
        const std::optional<int> hip2 = ParseInt(fields[hip2Col]);
        if (fields[conCol].empty() || !hip1 || !hip2) {
            std::cerr << "Constellation line data line " << lineNumber
                      << ": missing or invalid con, hip1 or hip2, skipping\n";
            continue;
        }

        lines.push_back({fields[conCol], *hip1, *hip2});
    }
    return lines;
}

std::vector<ConstellationLine> LoadConstellationLines(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open constellation line data at " << path << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseConstellationLines(buffer.str());
}

std::vector<VisibleConstellationLine> VisibleConstellationLines(
    const std::vector<Star>& stars, const std::vector<ConstellationLine>& lines,
    double observerLatRad, double lstRad) {
    std::unordered_map<int, const Star*> starByHip;
    for (const Star& star : stars) {
        if (star.hip != 0) {
            starByHip.emplace(star.hip, &star);
        }
    }

    std::vector<VisibleConstellationLine> visible;
    for (const ConstellationLine& segment : lines) {
        const auto itA = starByHip.find(segment.hip1);
        const auto itB = starByHip.find(segment.hip2);
        if (itA == starByHip.end() || itB == starByHip.end()) {
            continue;
        }
        const HorizontalPosition posA = EquatorialToHorizontal(
            itA->second->raRad, itA->second->decRad, observerLatRad, lstRad);
        const HorizontalPosition posB = EquatorialToHorizontal(
            itB->second->raRad, itB->second->decRad, observerLatRad, lstRad);
        if (posA.altitudeRad < 0.0 || posB.altitudeRad < 0.0) {
            continue;
        }
        visible.push_back({posA, posB});
    }
    return visible;
}

} // namespace core
