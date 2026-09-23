#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace app {

// Parses this project's operating rules format (see assets/operating_rules.txt): one rule per
// line, verbatim (surrounding whitespace trimmed), in file order. Blank lines and lines starting
// with '#' are ignored, so the file's own header comment does not itself become a rule. There is
// no further structure to a line -- no required columns, no per-row validation -- so nothing
// here can fail the way, say, ParseStarCatalog's numeric fields can; every remaining line becomes
// a rule.
std::vector<std::string> ParseOperatingRules(const std::string& text);

// Reads and parses the rules from a file. Returns an empty list, after printing the reason to
// stderr, if the file cannot be read -- there is no sensible default list of operating rules to
// fall back to, unlike Config's observer/watchlist defaults.
std::vector<std::string> LoadOperatingRules(const std::filesystem::path& path);

} // namespace app
