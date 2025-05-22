#include "minunit.h"
#include "freetype_handler.h" // Assuming this declares ftLibrary and ftFace as extern if needed for direct checks, or provides accessors.
#include <stdio.h>
#include <stdlib.h> // For EXIT_SUCCESS

// External declarations from freetype_handler.c if not in .h
// It's better if freetype_handler.h declares these if they are meant to be accessed by tests.
// For now, we assume they are accessible or tests are structured to not need direct access.
// extern FT_Library ftLibrary; 
// extern FT_Face ftFace;

MU_TEST(test_initFreeType_success) {
    mu_check(initFreeType() == 0);
    cleanupFreeType(); // Clean up after this test
}

MU_TEST(test_loadFont_success) {
    mu_check(initFreeType() == 0);
    // This test assumes tests/fonts/test_font.ttf exists.
    // If the font is not available, this specific assertion might fail,
    // but the test structure is what's being set up.
    mu_check(loadFont("tests/fonts/test_font.ttf") == 0);
    // mu_check(ftFace != NULL); // This check requires ftFace to be accessible
    cleanupFreeType();
}

MU_TEST(test_loadFont_fail_invalid_path) {
    mu_check(initFreeType() == 0);
    mu_check(loadFont("tests/fonts/non_existent_font.ttf") != 0);
    cleanupFreeType();
}

MU_TEST(test_cleanupFreeType) {
    mu_check(initFreeType() == 0);
    // Again, assuming tests/fonts/test_font.ttf exists for a successful load first.
    loadFont("tests/fonts/test_font.ttf"); 
    cleanupFreeType();
    // Verifying ftLibrary is NULL is tricky as it's an internal pointer.
    // A good test would be if a subsequent initFreeType() works without issues.
    // For now, we rely on no crashes and proper behavior in other tests.
    // mu_check(ftLibrary == NULL); // This check requires ftLibrary to be accessible and for FT_Done_FreeType to nullify it.
    // As a proxy, we can try to init again. If cleanup was proper, this should work.
    mu_check(initFreeType() == 0); // Re-initialize to see if cleanup was okay
    cleanupFreeType(); // Clean up the second init
}

MU_TEST_SUITE(freetype_handler_suite) {
    MU_RUN_TEST(test_initFreeType_success);
    MU_RUN_TEST(test_loadFont_success);
    MU_RUN_TEST(test_loadFont_fail_invalid_path);
    MU_RUN_TEST(test_cleanupFreeType);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(freetype_handler_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
