#ifndef TEXT_LAYOUT_H
#define TEXT_LAYOUT_H

#include <stddef.h> // For size_t
#include <ft2build.h>
#include FT_FREETYPE_H // Include the main FreeType header for FT_ULong and other types
// #include "glyph_manager.h" // Avoid direct dependency on full glyph_manager for easier testing

// Forward declaration if GlyphInfo is complex and comes from glyph_manager.h
// For testing, we might only care about advanceX.
typedef struct {
    // Minimal info needed for layout testing.
    // Matches fields available in GlyphInfo from glyph_manager.h or that can be mocked.
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    // textureID, width, height, bearingX, bearingY, advanceY are not in GlyphInfo
    // and might not be strictly needed for layout logic itself.
    // If renderer needs them, it should fetch the full GlyphInfo.
    long advanceX; // Key for layout
    int indexCount;
    FT_ULong codepoint; // For debugging, or if the GetGlyphMetricsFunc provides it
} MinimalGlyphInfo;


// Function pointer type for getting glyph metrics.
// For testing, this can point to a mock. For rendering, it points to getGlyphInfo.
typedef MinimalGlyphInfo (*GetGlyphMetricsFunc)(FT_ULong codepoint);

typedef struct {
    float x;
    float y;
} Position;

// Structure to hold calculated layout information.
// We might not store all char positions for simplicity in this phase,
// focusing on cursor position and key points.
typedef struct {
    Position cursor_pos;          // Calculated X, Y for the cursor
    FT_ULong codepoint_under_cursor; // Codepoint under the cursor
    MinimalGlyphInfo glyph_info_under_cursor; // Glyph info for char under cursor
    int cursor_is_over_char;    // Flag: 1 if cursor is over a char, 0 if at EOL

    // For debugging or more detailed tests, one could add:
    // Position char_positions[MAX_TEXT_LENGTH]; // Or dynamic
    // int num_chars_on_line[MAX_LINES];
    // float line_widths[MAX_LINES];
    // int total_lines;
} TextLayoutInfo;

TextLayoutInfo calculateTextLayout(
    const char* text,
    size_t cursorBytePos,
    float startX,
    float startY,
    float scale,
    float maxLineWidth,
    float lineHeight,
    GetGlyphMetricsFunc get_glyph_metrics // Function to get glyph advance width
);

#endif // TEXT_LAYOUT_H
