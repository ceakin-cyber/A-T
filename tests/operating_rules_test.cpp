#include "app/operating_rules.h"

#include <gtest/gtest.h>

namespace {

TEST(ParseOperatingRules, EmptyTextGivesAnEmptyList) {
    EXPECT_TRUE(app::ParseOperatingRules("").empty());
}

TEST(ParseOperatingRules, ReadsOneRulePerLine) {
    const auto rules = app::ParseOperatingRules("FIRST RULE\nSECOND RULE\nTHIRD RULE\n");
    ASSERT_EQ(rules.size(), 3U);
    EXPECT_EQ(rules[0], "FIRST RULE");
    EXPECT_EQ(rules[1], "SECOND RULE");
    EXPECT_EQ(rules[2], "THIRD RULE");
}

TEST(ParseOperatingRules, PreservesFileOrder) {
    const auto rules = app::ParseOperatingRules("C RULE\nA RULE\nB RULE\n");
    ASSERT_EQ(rules.size(), 3U);
    EXPECT_EQ(rules[0], "C RULE");
    EXPECT_EQ(rules[1], "A RULE");
    EXPECT_EQ(rules[2], "B RULE");
}

TEST(ParseOperatingRules, SkipsBlankLines) {
    const auto rules = app::ParseOperatingRules("FIRST RULE\n\n\nSECOND RULE\n");
    ASSERT_EQ(rules.size(), 2U);
    EXPECT_EQ(rules[0], "FIRST RULE");
    EXPECT_EQ(rules[1], "SECOND RULE");
}

TEST(ParseOperatingRules, SkipsCommentLines) {
    const auto rules = app::ParseOperatingRules("# a header comment\nFIRST RULE\n"
                                                 "# another comment\nSECOND RULE\n");
    ASSERT_EQ(rules.size(), 2U);
    EXPECT_EQ(rules[0], "FIRST RULE");
    EXPECT_EQ(rules[1], "SECOND RULE");
}

TEST(ParseOperatingRules, TrimsSurroundingWhitespace) {
    const auto rules = app::ParseOperatingRules("   INDENTED RULE   \n\tTAB-INDENTED RULE\t\n");
    ASSERT_EQ(rules.size(), 2U);
    EXPECT_EQ(rules[0], "INDENTED RULE");
    EXPECT_EQ(rules[1], "TAB-INDENTED RULE");
}

TEST(ParseOperatingRules, ACommentAfterLeadingWhitespaceIsStillSkipped) {
    const auto rules = app::ParseOperatingRules("   # indented comment\nFIRST RULE\n");
    ASSERT_EQ(rules.size(), 1U);
    EXPECT_EQ(rules[0], "FIRST RULE");
}

TEST(ParseOperatingRules, ALineOfOnlyWhitespaceIsTreatedAsBlank) {
    const auto rules = app::ParseOperatingRules("FIRST RULE\n   \t  \nSECOND RULE\n");
    ASSERT_EQ(rules.size(), 2U);
    EXPECT_EQ(rules[0], "FIRST RULE");
    EXPECT_EQ(rules[1], "SECOND RULE");
}

TEST(ParseOperatingRules, HandlesCrlfLineEndings) {
    const auto rules = app::ParseOperatingRules("FIRST RULE\r\nSECOND RULE\r\n");
    ASSERT_EQ(rules.size(), 2U);
    EXPECT_EQ(rules[0], "FIRST RULE");
    EXPECT_EQ(rules[1], "SECOND RULE");
}

TEST(ParseOperatingRules, ALineWithNoTrailingNewlineIsStillRead) {
    const auto rules = app::ParseOperatingRules("FIRST RULE\nSECOND RULE");
    ASSERT_EQ(rules.size(), 2U);
    EXPECT_EQ(rules[1], "SECOND RULE");
}

TEST(LoadOperatingRules, ReturnsAnEmptyListWhenTheFileDoesNotExist) {
    EXPECT_TRUE(app::LoadOperatingRules("/nonexistent/path/operating_rules.txt").empty());
}

// End-to-end against the real, committed rules file.
TEST(LoadOperatingRules, ReadsTheRealCommittedFile) {
    const auto rules = app::LoadOperatingRules("assets/operating_rules.txt");
    EXPECT_GT(rules.size(), 0U);
    for (const std::string& rule : rules) {
        EXPECT_FALSE(rule.empty());
        EXPECT_NE(rule.front(), '#');
    }
}

} // namespace
