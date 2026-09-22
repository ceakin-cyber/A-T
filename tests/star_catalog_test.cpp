#include "core/star_catalog.h"

#include <algorithm>
#include <gtest/gtest.h>

namespace {

const char* const kHeader = "id,hip,hd,hr,gl,bf,proper,ra,dec,dist,pmra,pmdec,rv,mag,absmag,"
                            "spect,ci,x,y,z,vx,vy,vz,rarad,decrad,pmrarad,pmdecrad,bayer,flam,"
                            "con,comp,comp_primary,base,lum,var,var_min,var_max";
constexpr int kColumnCount = 37; // number of fields in kHeader

// Column indices, matching kHeader exactly (checked by HeaderHasTheExpectedColumnCount below),
// so a row can be built by index instead of by counting commas in a literal string.
constexpr int kIdCol = 0;
constexpr int kProperCol = 6;
constexpr int kMagCol = 13;
constexpr int kCiCol = 16;
constexpr int kRaRadCol = 23;
constexpr int kDecRadCol = 24;

std::string JoinCsvFields(const std::vector<std::string>& fields) {
    std::string line;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) {
            line += ',';
        }
        line += fields[i];
    }
    return line;
}

// Builds a properly shaped data row: kColumnCount fields, all empty except the ones this parser
// reads, set at their real column index. proper is quoted, since the real catalog quotes it.
std::string Row(const std::string& id, const std::string& rarad, const std::string& decrad,
                const std::string& mag, const std::string& ci, const std::string& proper = "") {
    std::vector<std::string> fields(kColumnCount);
    fields[kIdCol] = id;
    fields[kRaRadCol] = rarad;
    fields[kDecRadCol] = decrad;
    fields[kMagCol] = mag;
    fields[kCiCol] = ci;
    fields[kProperCol] = "\"" + proper + "\"";
    return JoinCsvFields(fields);
}

TEST(TestHelper, HeaderMatchesTheDeclaredColumnCountAndIndices) {
    // Guards Row() itself: if kHeader is ever edited without updating kColumnCount and the
    // column index constants to match, every other test below would silently build misaligned
    // rows instead of failing clearly here.
    const std::vector<std::string> columns = [] {
        std::vector<std::string> result;
        std::string field;
        for (const char c : std::string(kHeader) + ",") {
            if (c == ',') {
                result.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
        return result;
    }();
    ASSERT_EQ(columns.size(), static_cast<std::size_t>(kColumnCount));
    EXPECT_EQ(columns[kIdCol], "id");
    EXPECT_EQ(columns[kProperCol], "proper");
    EXPECT_EQ(columns[kMagCol], "mag");
    EXPECT_EQ(columns[kCiCol], "ci");
    EXPECT_EQ(columns[kRaRadCol], "rarad");
    EXPECT_EQ(columns[kDecRadCol], "decrad");
}

TEST(ParseStarCatalog, ReadsIdRaDecMagAndColorIndex) {
    const std::string text = std::string(kHeader) + "\n" + Row("42", "1.5", "-0.3", "3.2", "0.65");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].id, 42);
    EXPECT_DOUBLE_EQ(stars[0].raRad, 1.5);
    EXPECT_DOUBLE_EQ(stars[0].decRad, -0.3);
    EXPECT_DOUBLE_EQ(stars[0].magnitude, 3.2);
    EXPECT_DOUBLE_EQ(stars[0].colorIndex, 0.65);
}

TEST(ParseStarCatalog, ReadsMultipleRowsInOrder) {
    const std::string text = std::string(kHeader) + "\n" + Row("1", "0.1", "0.1", "1.0", "0.1") +
                             "\n" + Row("2", "0.2", "0.2", "2.0", "0.2") + "\n";
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 2U);
    EXPECT_EQ(stars[0].id, 1);
    EXPECT_EQ(stars[1].id, 2);
}

TEST(ParseStarCatalog, ABlankColorIndexDefaultsToZero) {
    const std::string text = std::string(kHeader) + "\n" + Row("1", "0.1", "0.1", "1.0", "");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_DOUBLE_EQ(stars[0].colorIndex, 0.0);
}

TEST(ParseStarCatalog, AQuotedFieldWithACommaDoesNotShiftLaterColumns) {
    // The name field is quoted and contains a comma; the columns this parser reads come after
    // it, so a naive (non-quote-aware) comma split would misalign them.
    const std::string text =
        std::string(kHeader) + "\n" + Row("7", "0.5", "0.25", "4.0", "0.3", "Alpha, Prime");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].id, 7);
    EXPECT_DOUBLE_EQ(stars[0].raRad, 0.5);
    EXPECT_DOUBLE_EQ(stars[0].magnitude, 4.0);
}

TEST(ParseStarCatalog, SkipsARowWithAnInvalidIdAndKeepsReadingTheRest) {
    const std::string text = std::string(kHeader) + "\n" +
                             Row("not-a-number", "0.1", "0.1", "1.0", "0.1") + "\n" +
                             Row("2", "0.2", "0.2", "2.0", "0.2") + "\n";
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].id, 2);
}

TEST(ParseStarCatalog, SkipsARowWithAnInvalidRaAndKeepsReadingTheRest) {
    const std::string text = std::string(kHeader) + "\n" +
                             Row("1", "not-a-number", "0.1", "1.0", "0.1") + "\n" +
                             Row("2", "0.2", "0.2", "2.0", "0.2") + "\n";
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].id, 2);
}

TEST(ParseStarCatalog, SkipsARowThatIsTooShort) {
    const std::string text =
        std::string(kHeader) + "\n1,2,3\n" + Row("2", "0.2", "0.2", "2.0", "0.2") + "\n";
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].id, 2);
}

TEST(ParseStarCatalog, SkipsBlankLines) {
    const std::string text = std::string(kHeader) + "\n" + Row("1", "0.1", "0.1", "1.0", "0.1") +
                             "\n\n" + Row("2", "0.2", "0.2", "2.0", "0.2") + "\n";
    const auto stars = core::ParseStarCatalog(text);
    EXPECT_EQ(stars.size(), 2U);
}

TEST(ParseStarCatalog, EmptyTextGivesAnEmptyList) {
    EXPECT_TRUE(core::ParseStarCatalog("").empty());
}

TEST(ParseStarCatalog, MissingARequiredColumnGivesAnEmptyList) {
    const std::string noRarad = "id,decrad,mag,ci\n1,0.1,1.0,0.1\n";
    EXPECT_TRUE(core::ParseStarCatalog(noRarad).empty());
}

TEST(ParseStarCatalog, HandlesCrlfLineEndings) {
    const std::string text =
        std::string(kHeader) + "\r\n" + Row("1", "0.1", "0.1", "1.0", "0.1") + "\r\n";
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].id, 1);
}

TEST(LoadStarCatalog, ReturnsAnEmptyListWhenTheFileDoesNotExist) {
    EXPECT_TRUE(core::LoadStarCatalog("/nonexistent/path/catalog.csv").empty());
}

// End-to-end against the real, committed catalog (a filtered copy of the HYG Database, magnitude
// <= 6.0), not a synthetic one.
TEST(LoadStarCatalog, ReadsTheRealCommittedCatalog) {
    const auto stars = core::LoadStarCatalog("assets/stars/hygdata_mag6.csv");
    ASSERT_EQ(stars.size(), 5071U);

    for (const core::Star& star : stars) {
        EXPECT_LE(star.magnitude, 6.0) << star.id;
        EXPECT_GE(star.raRad, 0.0) << star.id;
        EXPECT_LE(star.raRad, 2.0 * 3.14159265358979323846) << star.id;
        EXPECT_GE(star.decRad, -3.14159265358979323846 / 2.0) << star.id;
        EXPECT_LE(star.decRad, 3.14159265358979323846 / 2.0) << star.id;
    }

    // The Sun, id 0: known values straight from the catalog file.
    const auto sol =
        std::find_if(stars.begin(), stars.end(), [](const core::Star& s) { return s.id == 0; });
    ASSERT_NE(sol, stars.end());
    EXPECT_DOUBLE_EQ(sol->raRad, 0.0);
    EXPECT_DOUBLE_EQ(sol->decRad, 0.0);
    EXPECT_DOUBLE_EQ(sol->magnitude, -26.7);
    EXPECT_DOUBLE_EQ(sol->colorIndex, 0.656);
}

} // namespace
