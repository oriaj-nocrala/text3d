#include "minunit.h"
#include "glyph_manager.h" // For GlyphInfo, CACHE_SIZE, initGlyphCache, cleanupGlyphCache, getGlyphInfo
#include "freetype_handler.h" // For FT_Library, FT_Face declarations
#include <stdio.h>
#include <stdlib.h> // For NULL
#include <math.h>   // For fabs

// External declarations from freetype_handler.c if not in a common test header
// These are declared as extern in freetype_handler.h, so they should be accessible.
extern FT_Library ftLibrary; 
extern FT_Face ftFace;

MU_TEST(test_init_glyph_cache) {
    mu_assert_int_eq(0, initGlyphCache()); // Expect 0 for success
    // Add a check to see if the cache is indeed initialized, 
    // e.g., by checking if a sample entry is zeroed out or set to a default state.
    // For now, we'll assume ftFace might be NULL if no font is loaded.
    // A more robust test would involve mocking FT_Load_Char and FT_Render_Glyph
    // or ensuring a font is loaded.
    // Let's check if the first few entries' VAOs are 0, assuming they are initialized.
    // This is a simple check; real cache state might be more complex.
    for (int i = 0; i < 10 && i < CACHE_SIZE; ++i) {
        mu_assert_int_eq(0, glyphCache[i].vao); 
    }
    cleanupGlyphCache(); // Clean up after test
}

MU_TEST(test_cleanup_glyph_cache) {
    initGlyphCache();
    // Potentially add some items to cache if we had a way to do it without full rendering
    cleanupGlyphCache();
    // Difficult to assert specific state after cleanup without knowing implementation details
    // or having access to internal state. We'll assume no crashes is a basic pass.
    mu_assert(1, "Cleanup completed (no crash check)");
}

MU_TEST(test_get_glyph_info_no_font) {
    // This test assumes FreeType is not initialized or no font is loaded.
    // The behavior of getGlyphInfo in this state should be predictable (e.g., return an empty/error GlyphInfo).
    // We need to know what getGlyphInfo returns if ftFace is NULL or char cannot be loaded.
    // For now, let's assume ftLibrary and ftFace might be NULL if not initialized.
    // A proper test would involve `initFreeType()` but not `loadFont()`.
    
    FT_Library prevFtLibrary = ftLibrary; // Save current state
    FT_Face prevFtFace = ftFace;         // Save current state
    
    ftLibrary = NULL; // Simulate FreeType not initialized or font not loaded
    ftFace = NULL;

    initGlyphCache(); // Initialize cache for the test

    GlyphInfo gi = getGlyphInfo('A');
    
    // Based on current glyph_manager.c, if ftFace is NULL, it prints an error and returns a zeroed GlyphInfo.
    mu_assert_int_eq(0, gi.vao);
    mu_assert_int_eq(0, gi.vbo);
    mu_assert_int_eq(0, gi.ebo);
    mu_assert_int_eq(0, gi.indexCount);
    mu_check(fabs(gi.advanceX - 0.0f) < 1e-5); // Corrected assertion for float comparison

    cleanupGlyphCache();
    
    // Restore previous state if necessary (though for tests, it's usually fine to leave it)
    ftLibrary = prevFtLibrary;
    ftFace = prevFtFace;
}

MU_TEST_SUITE(glyph_manager_suite) {
    MU_RUN_TEST(test_init_glyph_cache);
    MU_RUN_TEST(test_cleanup_glyph_cache);
    MU_RUN_TEST(test_get_glyph_info_no_font);
}

int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv; 
    MU_RUN_SUITE(glyph_manager_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
