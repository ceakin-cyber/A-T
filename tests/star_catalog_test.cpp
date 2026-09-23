#include "core/star_catalog.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <numbers>

namespace {

double Radians(double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

double Degrees(double radians) {
    return radians * 180.0 / std::numbers::pi;
}

const char* const kHeader = "id,hip,hd,hr,gl,bf,proper,ra,dec,dist,pmra,pmdec,rv,mag,absmag,"
                            "spect,ci,x,y,z,vx,vy,vz,rarad,decrad,pmrarad,pmdecrad,bayer,flam,"
                            "con,comp,comp_primary,base,lum,var,var_min,var_max";
constexpr int kColumnCount = 37; // number of fields in kHeader

// Column indices, matching kHeader exactly (checked by HeaderHasTheExpectedColumnCount below),
// so a row can be built by index instead of by counting commas in a literal string.
constexpr int kIdCol = 0;
constexpr int kHipCol = 1;
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
// reads, set at their real column index. proper is quoted, since the real catalog quotes it. hip
// defaults to blank, matching every existing call site that doesn't care about it.
std::string Row(const std::string& id, const std::string& rarad, const std::string& decrad,
                const std::string& mag, const std::string& ci, const std::string& proper = "",
                const std::string& hip = "") {
    std::vector<std::string> fields(kColumnCount);
    fields[kIdCol] = id;
    fields[kHipCol] = hip;
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
    EXPECT_EQ(columns[kHipCol], "hip");
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

TEST(ParseStarCatalog, ReadsHip) {
    const std::string text = std::string(kHeader) + "\n" +
                             Row("42", "1.5", "-0.3", "3.2", "0.65", "", "32349");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].hip, 32349);
}

TEST(ParseStarCatalog, ABlankHipDefaultsToZero) {
    const std::string text = std::string(kHeader) + "\n" + Row("1", "0.1", "0.1", "1.0", "0.1");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].hip, 0);
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
    EXPECT_EQ(stars[0].properName, "Alpha, Prime");
}

TEST(ParseStarCatalog, ReadsProperName) {
    const std::string text = std::string(kHeader) + "\n" +
                             Row("1", "0.1", "0.1", "1.0", "0.1", "Sirius");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_EQ(stars[0].properName, "Sirius");
}

TEST(ParseStarCatalog, ABlankProperNameDefaultsToEmpty) {
    const std::string text = std::string(kHeader) + "\n" + Row("1", "0.1", "0.1", "1.0", "0.1");
    const auto stars = core::ParseStarCatalog(text);
    ASSERT_EQ(stars.size(), 1U);
    EXPECT_TRUE(stars[0].properName.empty());
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

TEST(ParseStarCatalog, MissingTheHipColumnGivesAnEmptyList) {
    const std::string noHip = "id,rarad,decrad,mag,ci\n1,0.1,0.1,1.0,0.1\n";
    EXPECT_TRUE(core::ParseStarCatalog(noHip).empty());
}

TEST(ParseStarCatalog, MissingTheProperColumnGivesAnEmptyList) {
    const std::string noProper = "id,hip,rarad,decrad,mag,ci\n1,1,0.1,0.1,1.0,0.1\n";
    EXPECT_TRUE(core::ParseStarCatalog(noProper).empty());
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
    EXPECT_EQ(sol->hip, 0); // the Sun has no Hipparcos number
    EXPECT_EQ(sol->properName, "Sol");

    // Tau Phe, id 88: known values straight from the catalog file, hip equal to id for this row.
    const auto tauPhe =
        std::find_if(stars.begin(), stars.end(), [](const core::Star& s) { return s.id == 88; });
    ASSERT_NE(tauPhe, stars.end());
    EXPECT_EQ(tauPhe->hip, 88);
    EXPECT_TRUE(tauPhe->properName.empty()); // most stars have no common name

    // Sirius, the brightest real star: known to have the proper name "Sirius" in this catalog.
    const auto sirius = std::find_if(stars.begin(), stars.end(), [](const core::Star& s) {
        return s.properName == "Sirius";
    });
    ASSERT_NE(sirius, stars.end());
    EXPECT_NEAR(sirius->magnitude, -1.44, 1e-6);
}

// The three cases below have independently-verified altitudes (see tests/celestial_test.cpp for
// the same underlying geometry): a star below the horizon, one above it, and one exactly on it
// (the north celestial pole, dec 90, seen from the equator, lat 0 -- its altitude always equals
// the observer's latitude).

core::Star MakeStar(int id, double raDeg, double decDeg) {
    core::Star star;
    star.id = id;
    star.raRad = Radians(raDeg);
    star.decRad = Radians(decDeg);
    star.magnitude = 3.0;
    return star;
}

TEST(VisibleStars, KeepsAStarAboveTheHorizon) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, 60.0)};
    const auto visible = core::VisibleStars(stars, Radians(40.0), 0.0);
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_EQ(visible[0].star.id, 1);
    EXPECT_NEAR(Degrees(visible[0].position.altitudeRad), 70.0, 1e-9);
}

TEST(VisibleStars, DropsAStarBelowTheHorizon) {
    const std::vector<core::Star> stars = {MakeStar(2, 0.0, -80.0)};
    EXPECT_TRUE(core::VisibleStars(stars, Radians(40.0), 0.0).empty());
}

TEST(VisibleStars, KeepsAStarRightAtTheCutoffAndDropsOneJustBelowIt) {
    // "Below the horizon" means strictly negative altitude, so exactly 0 degrees should be kept.
    // In floating point, trig never lands on an exact mathematical 0 (dec 90 seen from the
    // equator is 0 degrees altitude in theory, but computes to about 1.3e-17, not 0.0), so the
    // boundary is exercised here with two stars a controlled 0.01 degrees to either side of it
    // instead, independently verified in Python: dec 89.99 is +0.01 degrees (kept), dec 90.01 is
    // -0.01 degrees (dropped).
    const std::vector<core::Star> stars = {MakeStar(3, 0.0, 89.99), MakeStar(4, 0.0, 90.01)};
    const auto visible = core::VisibleStars(stars, Radians(0.0), 0.0);
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_EQ(visible[0].star.id, 3);
    EXPECT_NEAR(Degrees(visible[0].position.altitudeRad), 0.01, 1e-9);
}

TEST(VisibleStars, FiltersAMixOfStarsKeepingOnlyThoseAboveOrOnTheHorizon) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, 60.0), MakeStar(2, 0.0, -80.0),
                                           MakeStar(4, 10.0, 70.0)};
    const auto visible = core::VisibleStars(stars, Radians(40.0), 0.0);
    ASSERT_EQ(visible.size(), 2U);
    EXPECT_EQ(visible[0].star.id, 1);
    EXPECT_EQ(visible[1].star.id, 4);
}

TEST(VisibleStars, EmptyInputGivesEmptyOutput) {
    EXPECT_TRUE(core::VisibleStars({}, Radians(40.0), 0.0).empty());
}

TEST(VisibleStars, EveryStarBelowTheHorizonGivesAnEmptyList) {
    const std::vector<core::Star> stars = {MakeStar(1, 0.0, -80.0), MakeStar(2, 20.0, -85.0)};
    EXPECT_TRUE(core::VisibleStars(stars, Radians(40.0), 0.0).empty());
}

TEST(VisibleStars, ThePositionMatchesCallingTheTransformDirectly) {
    // RA 0, dec 60, observer lat 40, LST 0: independently verified above the horizon at
    // altitude 70 degrees (see tests/celestial_test.cpp).
    const std::vector<core::Star> stars = {MakeStar(5, 0.0, 60.0)};
    const auto visible = core::VisibleStars(stars, Radians(40.0), 0.0);
    ASSERT_EQ(visible.size(), 1U);
    const auto expected =
        core::EquatorialToHorizontal(Radians(0.0), Radians(60.0), Radians(40.0), 0.0);
    EXPECT_DOUBLE_EQ(visible[0].position.altitudeRad, expected.altitudeRad);
    EXPECT_DOUBLE_EQ(visible[0].position.azimuthRad, expected.azimuthRad);
}

TEST(MagnitudeToPointStyle, TheBrightestStyledMagnitudeGivesTheLargestFullestStyle) {
    const auto style = core::MagnitudeToPointStyle(core::kBrightestStyledMagnitude);
    EXPECT_FLOAT_EQ(style.radiusPx, 3.0F);
    EXPECT_FLOAT_EQ(style.brightness, 1.0F);
}

TEST(MagnitudeToPointStyle, TheFaintestStyledMagnitudeGivesTheSmallestDimmestStyle) {
    const auto style = core::MagnitudeToPointStyle(core::kFaintestStyledMagnitude);
    EXPECT_FLOAT_EQ(style.radiusPx, 0.5F);
    EXPECT_FLOAT_EQ(style.brightness, 0.3F);
}

TEST(MagnitudeToPointStyle, TheMidpointGivesTheMidpointStyle) {
    const double midMag = (core::kBrightestStyledMagnitude + core::kFaintestStyledMagnitude) / 2.0;
    const auto style = core::MagnitudeToPointStyle(midMag);
    EXPECT_NEAR(style.radiusPx, 1.75, 1e-6);
    EXPECT_NEAR(style.brightness, 0.65, 1e-6);
}

TEST(MagnitudeToPointStyle, BrighterThanTheBrightestEndpointClampsToIt) {
    // Sol, magnitude -26.7 in this app's own catalog: far brighter than any real night-sky star,
    // and must not produce an oversized or out-of-range point.
    const auto style = core::MagnitudeToPointStyle(-26.7);
    EXPECT_FLOAT_EQ(style.radiusPx, 3.0F);
    EXPECT_FLOAT_EQ(style.brightness, 1.0F);
}

TEST(MagnitudeToPointStyle, DimmerThanTheFaintestEndpointClampsToIt) {
    const auto style = core::MagnitudeToPointStyle(9.0);
    EXPECT_FLOAT_EQ(style.radiusPx, 0.5F);
    EXPECT_FLOAT_EQ(style.brightness, 0.3F);
}

TEST(MagnitudeToPointStyle, RadiusAndBrightnessDecreaseMonotonicallyAsMagnitudeIncreases) {
    float previousRadius = 999.0F;
    float previousBrightness = 999.0F;
    for (double mag = core::kBrightestStyledMagnitude; mag <= core::kFaintestStyledMagnitude;
         mag += 0.25) {
        const auto style = core::MagnitudeToPointStyle(mag);
        EXPECT_LE(style.radiusPx, previousRadius) << mag;
        EXPECT_LE(style.brightness, previousBrightness) << mag;
        previousRadius = style.radiusPx;
        previousBrightness = style.brightness;
    }
}

TEST(MagnitudeToPointStyle, SiriusIsNearTheBrightEndAndTheCutoffIsAtTheDimEnd) {
    // Sirius, the brightest real star: -1.46. Should sit close to (but not necessarily exactly
    // at) the brightest style, since it is a touch dimmer than kBrightestStyledMagnitude (-1.5).
    const auto sirius = core::MagnitudeToPointStyle(-1.46);
    EXPECT_GT(sirius.radiusPx, 2.9F);
    EXPECT_GT(sirius.brightness, 0.99F);

    // A star right at the catalog's own faint cutoff.
    const auto atCutoff = core::MagnitudeToPointStyle(6.0);
    EXPECT_FLOAT_EQ(atCutoff.radiusPx, 0.5F);
}

void ExpectColor(const core::Rgb& actual, float r, float g, float b, float tolerance = 1e-6F) {
    EXPECT_NEAR(actual.r, r, tolerance);
    EXPECT_NEAR(actual.g, g, tolerance);
    EXPECT_NEAR(actual.b, b, tolerance);
}

TEST(ColorIndexToRgb, MatchesEachAnchorExactly) {
    ExpectColor(core::ColorIndexToRgb(-0.4), 0.61F, 0.70F, 1.00F);
    ExpectColor(core::ColorIndexToRgb(0.0), 0.80F, 0.85F, 1.00F);
    ExpectColor(core::ColorIndexToRgb(0.4), 1.00F, 0.98F, 0.95F);
    ExpectColor(core::ColorIndexToRgb(0.7), 1.00F, 0.92F, 0.80F);
    ExpectColor(core::ColorIndexToRgb(1.0), 1.00F, 0.80F, 0.60F);
    ExpectColor(core::ColorIndexToRgb(1.6), 1.00F, 0.65F, 0.45F);
    ExpectColor(core::ColorIndexToRgb(2.0), 1.00F, 0.50F, 0.40F);
}

TEST(ColorIndexToRgb, InterpolatesHalfwayBetweenTwoAnchors) {
    // Halfway between the 0.0 and 0.4 anchors (color index 0.2).
    ExpectColor(core::ColorIndexToRgb(0.2), (0.80F + 1.00F) / 2.0F, (0.85F + 0.98F) / 2.0F,
                (1.00F + 0.95F) / 2.0F, 1e-5F);
}

TEST(ColorIndexToRgb, BelowTheBluestAnchorClampsToIt) {
    ExpectColor(core::ColorIndexToRgb(-1.5), 0.61F, 0.70F, 1.00F);
}

TEST(ColorIndexToRgb, AboveTheReddestAnchorClampsToIt) {
    ExpectColor(core::ColorIndexToRgb(5.0), 1.00F, 0.50F, 0.40F);
}

TEST(ColorIndexToRgb, TheBlueComponentNeverIncreasesAsColorIndexRises) {
    // A meaningful, verifiable-by-construction property even without an external color
    // reference: warmer (higher color index) stars should never look bluer than cooler ones.
    float previousBlue = 999.0F;
    for (double ci = core::kBluestColorIndex; ci <= core::kReddestColorIndex; ci += 0.1) {
        const core::Rgb color = core::ColorIndexToRgb(ci);
        EXPECT_LE(color.b, previousBlue + 1e-6F) << ci;
        previousBlue = color.b;
    }
}

TEST(ColorIndexToRgb, RedDominanceNeverDecreasesAsColorIndexRises) {
    // "Red dominance": how much more red there is than blue. Should not fall as stars get
    // cooler/redder.
    float previousDominance = -999.0F;
    for (double ci = core::kBluestColorIndex; ci <= core::kReddestColorIndex; ci += 0.1) {
        const core::Rgb color = core::ColorIndexToRgb(ci);
        const float dominance = color.r - color.b;
        EXPECT_GE(dominance, previousDominance - 1e-6F) << ci;
        previousDominance = dominance;
    }
}

TEST(ColorIndexToRgb, EveryComponentStaysInZeroToOne) {
    for (double ci = -2.0; ci <= 3.0; ci += 0.1) {
        const core::Rgb color = core::ColorIndexToRgb(ci);
        EXPECT_GE(color.r, 0.0F) << ci;
        EXPECT_LE(color.r, 1.0F) << ci;
        EXPECT_GE(color.g, 0.0F) << ci;
        EXPECT_LE(color.g, 1.0F) << ci;
        EXPECT_GE(color.b, 0.0F) << ci;
        EXPECT_LE(color.b, 1.0F) << ci;
    }
}

TEST(ColorIndexToRgb, TheSunsOwnColorIndexLandsInTheYellowWhiteRange) {
    // Sol's color index in our own committed catalog is 0.656 (see LoadStarCatalog's real-data
    // test). Not an exact target -- there is no single "correct" RGB for this -- just a sanity
    // check that it falls where the yellow-white anchors put it: bright, and warmer than pure
    // white, cooler than orange.
    const core::Rgb sol = core::ColorIndexToRgb(0.656);
    EXPECT_GT(sol.r, 0.99F);
    EXPECT_LT(sol.g, 1.0F);
    EXPECT_GT(sol.g, 0.85F);
    EXPECT_LT(sol.b, sol.g);
}

core::Star MakeLabelTestStar(double magnitude, const std::string& properName) {
    core::Star star;
    star.magnitude = magnitude;
    star.properName = properName;
    return star;
}

TEST(StarLabel, ABrightNamedStarIsLabeledWithItsName) {
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(-1.44, "Sirius")), "Sirius");
}

TEST(StarLabel, ADimNamedStarHasNoLabel) {
    // A name alone is not enough: fainter than kLabelMagnitude means no label, named or not,
    // so the map does not turn into a wall of overlapping text.
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(6.0, "Some Obscure Name")), "");
}

TEST(StarLabel, ADimUnnamedStarHasNoLabel) {
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(6.0, "")), "");
}

TEST(StarLabel, ABrightUnnamedStarIsLabeledWithItsMagnitude) {
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(0.96, "")), "1.0");
}

TEST(StarLabel, ExactlyAtTheThresholdIsLabeled) {
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(core::kLabelMagnitude, "")), "2.0");
}

TEST(StarLabel, JustDimmerThanTheThresholdHasNoLabel) {
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(core::kLabelMagnitude + 0.01, "")), "");
}

TEST(StarLabel, ABrightNamedStarPrefersItsNameOverItsMagnitude) {
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(-1.44, "Sirius")), "Sirius");
}

TEST(StarLabel, AVeryNegativeMagnitudeStillFormatsCleanly) {
    // Sol, in this app's own catalog: -26.7. Even without a name, a very bright object should
    // format to a short, sane label, not something pathological.
    EXPECT_EQ(core::StarLabel(MakeLabelTestStar(-26.7, "")), "-26.7");
}

} // namespace
