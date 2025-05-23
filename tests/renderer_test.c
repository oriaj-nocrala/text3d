#include <stdio.h>
#include <string.h>
#include <float.h> // For FLT_EPSILON
#include <math.h>  // For fabs

#include "minunit.h"
#include "text_layout.h" // The header for the function we are testing
#include "utils.h"       // For utf8_to_codepoint, if needed directly in tests (likely not)

// --- Test Configuration ---
const float TEST_START_X = -0.98f;
const float TEST_START_Y = 0.95f;
const float TEST_SCALE = 0.003f;
const float TEST_MAX_LINE_WIDTH = 1.96f; // Example: allows for several wide characters
const float TEST_LINE_HEIGHT = 0.18f;
const long MOCK_ADVANCE_X = 600; // A fixed advance width for all mock characters (e.g., 600 units for FreeType)
                                 // TEST_SCALE * MOCK_ADVANCE_X = 0.003 * 600 = 1.8 (too large for many chars on a line)
                                 // Let's use a more realistic scaled advance for testing line breaks.
                                 // If MOCK_ADVANCE_X_SCALED is, say, 0.3, then TEST_MAX_LINE_WIDTH / 0.3 = ~6 chars per line.
const float MOCK_ADVANCE_X_SCALED = 0.3f; // Target scaled advance width for mock characters

// Mock implementation for GetGlyphMetricsFunc
MinimalGlyphInfo mock_get_glyph_metrics(FT_ULong codepoint) {
    (void)codepoint; // Unused for this simple mock
    MinimalGlyphInfo info = {0};
    // We want the effective advance (advanceX * scale) to be MOCK_ADVANCE_X_SCALED
    // So, info.advanceX = MOCK_ADVANCE_X_SCALED / TEST_SCALE
    info.advanceX = (long)(MOCK_ADVANCE_X_SCALED / TEST_SCALE);
    // Other fields can be zero or dummy values as they are not used by calculateTextLayout's core logic
    // except potentially for glyph_info_under_cursor, but tests will mostly check positions.
    info.codepoint = codepoint; // Store it for potential checks
    return info;
}

// Helper to compare floats with a tolerance
int floats_are_close(float a, float b) {
    return fabs(a - b) < FLT_EPSILON * 100; // Using a slightly larger epsilon
}

// --- Test Cases ---

MU_TEST(test_empty_string) {
    printf("Running test_empty_string...\n");
    TextLayoutInfo layout = calculateTextLayout("", 0, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X));
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));
    mu_check(layout.cursor_is_over_char == 0);
    mu_check(layout.codepoint_under_cursor == 0);
}

MU_TEST(test_single_char_cursor_at_start) {
    printf("Running test_single_char_cursor_at_start...\n");
    TextLayoutInfo layout = calculateTextLayout("A", 0, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X)); // Cursor is AT char 'A'
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));
    mu_check(layout.cursor_is_over_char == 1);
    // mu_check(layout.codepoint_under_cursor == 'A'); // Assuming 'A' is passed as FT_ULong
}

MU_TEST(test_single_char_cursor_at_end) {
    printf("Running test_single_char_cursor_at_end...\n");
    TextLayoutInfo layout = calculateTextLayout("A", 1, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X + MOCK_ADVANCE_X_SCALED)); // Cursor is AFTER char 'A'
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));
    mu_check(layout.cursor_is_over_char == 0); // Cursor is at end, not over a char
}

MU_TEST(test_single_line_no_wrap) {
    printf("Running test_single_line_no_wrap...\n");
    const char* text = "Hello"; // 5 chars
    size_t text_len = strlen(text);
    TextLayoutInfo layout = calculateTextLayout(text, text_len, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    float expected_x = TEST_START_X + (text_len * MOCK_ADVANCE_X_SCALED);
    mu_check(floats_are_close(layout.cursor_pos.x, expected_x));
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));
    mu_check(layout.cursor_is_over_char == 0);
}

MU_TEST(test_text_exact_fit_no_wrap) {
    printf("Running test_text_exact_fit_no_wrap...\n");
    // Max line width = 1.96, MOCK_ADVANCE_X_SCALED = 0.3. 1.96 / 0.3 = 6.53. So 6 chars should fit.
    // 6 * 0.3 = 1.8. This is < 1.96.
    // We need to test the condition `simulatedCurrentX + (glyph_info.advanceX * scale) > startX + maxLineWidth`
    // Let's make maxLineWidth exactly 3 * MOCK_ADVANCE_X_SCALED from startX
    // So, maxLineWidth = 3 * 0.3 = 0.9
    // And startX + maxLineWidth = -0.98 + 0.9 = -0.08
    const float customMaxLineWidth = MOCK_ADVANCE_X_SCALED * 3; // Allows exactly 3 chars
    const char* text = "Abc"; // 3 chars
    size_t text_len = strlen(text);

    // Cursor at end of "Abc"
    TextLayoutInfo layout = calculateTextLayout(text, text_len, TEST_START_X, TEST_START_Y, TEST_SCALE, customMaxLineWidth, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    float expected_x_end = TEST_START_X + (3 * MOCK_ADVANCE_X_SCALED);
    mu_check(floats_are_close(layout.cursor_pos.x, expected_x_end));
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y)); // Still on the first line
    mu_check(layout.cursor_is_over_char == 0);

    // Cursor on 'c' (byte index 2)
    layout = calculateTextLayout(text, 2, TEST_START_X, TEST_START_Y, TEST_SCALE, customMaxLineWidth, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    float expected_x_on_c = TEST_START_X + (2 * MOCK_ADVANCE_X_SCALED);
    mu_check(floats_are_close(layout.cursor_pos.x, expected_x_on_c));
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));
    mu_check(layout.cursor_is_over_char == 1);
}

MU_TEST(test_simple_wrap) {
    printf("Running test_simple_wrap...\n");
    // TEST_MAX_LINE_WIDTH = 1.96f. MOCK_ADVANCE_X_SCALED = 0.3f.
    // Max chars per line = floor(1.96 / 0.3) = floor(6.53) = 6 chars.
    // 6 * 0.3 = 1.8.
    // The 7th char should wrap.
    const char* text = "1234567"; // 7 chars
    
    // Cursor on '7' (byte index 6, which is the 7th char)
    TextLayoutInfo layout = calculateTextLayout(text, 6, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X)); // '7' is at the start of the new line
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y - TEST_LINE_HEIGHT)); // '7' is on the second line
    mu_check(layout.cursor_is_over_char == 1);

    // Cursor after '7' (byte index 7, end of text)
    layout = calculateTextLayout(text, 7, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X + MOCK_ADVANCE_X_SCALED)); // After '7'
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y - TEST_LINE_HEIGHT)); // Still on the second line
    mu_check(layout.cursor_is_over_char == 0);

    // Cursor on '6' (byte index 5)
    layout = calculateTextLayout(text, 5, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    float expected_x_on_6 = TEST_START_X + (5 * MOCK_ADVANCE_X_SCALED);
    mu_check(floats_are_close(layout.cursor_pos.x, expected_x_on_6)); // '6' is on the first line
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));   // First line
    mu_check(layout.cursor_is_over_char == 1);
}


MU_TEST(test_cursor_at_wrap_point_after_char_that_causes_wrap) {
    printf("Running test_cursor_at_wrap_point_after_char_that_causes_wrap...\n");
    // Max 6 chars per line (123456). The 7th char (7) wraps.
    // Text: "1234567"
    // Cursor at byte position 6 (on character '7').
    // '7' itself should be on the new line.
    const char* text = "1234567";
    TextLayoutInfo layout = calculateTextLayout(text, 6, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X)); // '7' is at startX of new line
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y - TEST_LINE_HEIGHT)); // '7' is on second line
    mu_check(layout.cursor_is_over_char == 1);
    // mu_check(layout.codepoint_under_cursor == '7');
}

MU_TEST(test_cursor_at_end_of_wrapped_line) {
    printf("Running test_cursor_at_end_of_wrapped_line...\n");
    // Max 6 chars "123456". Cursor after '6'.
    // Text: "123456" (cursorBytePos = 6)
    const char* text = "123456";
    size_t text_len = strlen(text);
    TextLayoutInfo layout = calculateTextLayout(text, text_len, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    
    float expected_x = TEST_START_X + (6 * MOCK_ADVANCE_X_SCALED);
    mu_check(floats_are_close(layout.cursor_pos.x, expected_x));
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y));
    mu_check(layout.cursor_is_over_char == 0);
}


MU_TEST(test_multiple_wraps) {
    printf("Running test_multiple_wraps...\n");
    // Max 6 chars per line.
    // Line 1: "123456" (6 chars)
    // Line 2: "abcdef" (6 chars)
    // Line 3: "ghijkl" (6 chars)
    // Line 4: "m"      (1 char)
    // Total = 19 chars
    const char* text = "123456abcdefghijklm"; // 19 chars
    
    // Cursor on 'm' (byte index 18)
    TextLayoutInfo layout = calculateTextLayout(text, 18, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X)); // 'm' is at start of 4th line
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y - (3 * TEST_LINE_HEIGHT))); // 4th line
    mu_check(layout.cursor_is_over_char == 1);

    // Cursor after 'm' (byte index 19, end of text)
    layout = calculateTextLayout(text, 19, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    mu_check(floats_are_close(layout.cursor_pos.x, TEST_START_X + MOCK_ADVANCE_X_SCALED)); // after 'm'
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y - (3 * TEST_LINE_HEIGHT))); // 4th line
    mu_check(layout.cursor_is_over_char == 0);

    // Cursor on 'l' (byte index 17, last char of 3rd line)
    layout = calculateTextLayout(text, 17, TEST_START_X, TEST_START_Y, TEST_SCALE, TEST_MAX_LINE_WIDTH, TEST_LINE_HEIGHT, mock_get_glyph_metrics);
    float expected_x_on_l = TEST_START_X + (5 * MOCK_ADVANCE_X_SCALED); // 'l' is the 6th char on its line (index 5 within line)
    mu_check(floats_are_close(layout.cursor_pos.x, expected_x_on_l));
    mu_check(floats_are_close(layout.cursor_pos.y, TEST_START_Y - (2 * TEST_LINE_HEIGHT))); // 3rd line
    mu_check(layout.cursor_is_over_char == 1);
}


// --- Test Suite Setup ---
MU_TEST_SUITE(renderer_layout_test_suite) {
    MU_RUN_TEST(test_empty_string);
    MU_RUN_TEST(test_single_char_cursor_at_start);
    MU_RUN_TEST(test_single_char_cursor_at_end);
    MU_RUN_TEST(test_single_line_no_wrap);
    MU_RUN_TEST(test_text_exact_fit_no_wrap);
    MU_RUN_TEST(test_simple_wrap);
    MU_RUN_TEST(test_cursor_at_wrap_point_after_char_that_causes_wrap);
    MU_RUN_TEST(test_cursor_at_end_of_wrapped_line);
    MU_RUN_TEST(test_multiple_wraps);
}

// --- Main function to run tests ---
// Not strictly needed if using the Makefile's test runner,
// but good for direct execution.
int main(int argc, char *argv[]) {
    (void)argc; // Unused
    (void)argv; // Unused
    printf("Starting renderer layout tests...\n");
    MU_RUN_SUITE(renderer_layout_test_suite);
    MU_REPORT();
    printf("Renderer layout tests finished.\n");
    return MU_EXIT_CODE;
}
