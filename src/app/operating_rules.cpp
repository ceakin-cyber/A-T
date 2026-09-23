#include "app/operating_rules.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace app {

namespace {

std::string_view Trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

} // namespace

std::vector<std::string> ParseOperatingRules(const std::string& text) {
    std::vector<std::string> rules;

    std::istringstream stream(text);
    std::string rawLine;
    while (std::getline(stream, rawLine)) {
        const std::string_view line = Trim(rawLine);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        rules.emplace_back(line);
    }
    return rules;
}

std::vector<std::string> LoadOperatingRules(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open operating rules at " << path << '\n';
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return ParseOperatingRules(buffer.str());
}

} // namespace app
