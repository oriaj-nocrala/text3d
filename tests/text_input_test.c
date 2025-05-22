#include "minunit.h"
#include <string.h>
#include <stdio.h>
#include <GL/gl.h> // For GLuint definition

// External declarations from src/main.c
// globalTextInputBuffer is defined in main.c
extern char globalTextInputBuffer[256]; 
// keyboardCallback and specialKeyboardCallback are defined in main.c
extern void keyboardCallback(unsigned char key, int x, int y);
extern void specialKeyboardCallback(int key, int x, int y);

// Dummy glutPostRedisplay for testing purposes
// The real one is in FreeGLUT and not needed for these unit tests' logic.
// It's called by the callbacks in main.c, so a dummy is needed to link.
void glutPostRedisplay(void) { 
    // For these tests, we primarily care about the buffer content.
    // A real test might set a flag here to check if it was called.
}

// Dummy implementations for functions referenced by main_module.o (compiled from src/main.c)
// but not defined in text_input_test.o or main_module.o, and not relevant to text input logic.
void renderText(GLuint program, const char* text) { (void)program; (void)text; /* Dummy */ }
void cleanupGlyphCache() { /* Dummy */ }
void cleanupOpenGL(GLuint program) { (void)program; /* Dummy */ }
void cleanupFreeType() { /* Dummy */ }
// initOpenGL, initFreeType, loadFonts, initGlyphCache are called from main() in src/main.c,
// which is excluded by #ifndef UNIT_TESTING. So dummies for these are not needed
// unless other functions in main_module.o (not excluded) call them.
// display() and cleanup() are in main_module.o and call the above, hence the dummies.


// The specialKeyboardCallback in main.c uses GLUT_KEY_BACKSPACE.
// This constant is defined in <GL/freeglut_std.h> or <GL/glut.h>.
// When main.c is compiled for tests (main_module.o), it should now find this
// definition thanks to the updated Makefile including GLUT cflags.
// So, we don't strictly need to define it here in the test file itself,
// but the value 8 is what GLUT_KEY_BACKSPACE typically is.
// We rely on the actual value from GLUT headers being used by specialKeyboardCallback.

// Helper function to reset buffer for each test
static void setup_buffer(const char* initial_content) {
    if (initial_content == NULL) {
        globalTextInputBuffer[0] = '\0';
    } else {
        strncpy(globalTextInputBuffer, initial_content, sizeof(globalTextInputBuffer) - 1);
        globalTextInputBuffer[sizeof(globalTextInputBuffer) - 1] = '\0';
    }
}

// --- Test Cases for keyboardCallback ---

MU_TEST(test_keyboard_append_to_empty) {
    setup_buffer("");
    keyboardCallback('a', 0, 0);
    mu_assert_string_eq("a", globalTextInputBuffer);
}

MU_TEST(test_keyboard_append_multiple) {
    setup_buffer("abc");
    keyboardCallback('d', 0, 0);
    mu_assert_string_eq("abcd", globalTextInputBuffer);
    keyboardCallback('e', 0, 0);
    mu_assert_string_eq("abcde", globalTextInputBuffer);
}

MU_TEST(test_keyboard_append_to_almost_full) {
    char test_str_initial[256];
    memset(test_str_initial, 'x', 254);
    test_str_initial[254] = '\0';
    setup_buffer(test_str_initial);
    
    keyboardCallback('y', 0, 0);
    
    char expected_str[256];
    memset(expected_str, 'x', 254);
    expected_str[254] = 'y';
    expected_str[255] = '\0';
    mu_assert_string_eq(expected_str, globalTextInputBuffer);
}

MU_TEST(test_keyboard_append_to_full) {
    char test_str_full[256];
    memset(test_str_full, 'x', 255);
    test_str_full[255] = '\0';
    setup_buffer(test_str_full);

    keyboardCallback('y', 0, 0); // Should not append
    mu_assert_string_eq(test_str_full, globalTextInputBuffer);
    // Also check length just to be sure
    mu_assert_int_eq(255, strlen(globalTextInputBuffer));
}

// --- Test Cases for specialKeyboardCallback ---

MU_TEST(test_special_backspace_multiple_chars) {
    setup_buffer("abcde");
    // We need to use the actual GLUT_KEY_BACKSPACE value that specialKeyboardCallback expects.
    // Assuming this is 8 as per typical GLUT setups / freeglut_std.h
    // The callback in main.c uses the GLUT_KEY_BACKSPACE constant directly.
    specialKeyboardCallback(8, 0, 0); // 8 is typical for GLUT_KEY_BACKSPACE
    mu_assert_string_eq("abcd", globalTextInputBuffer);
}

MU_TEST(test_special_backspace_single_char) {
    setup_buffer("a");
    specialKeyboardCallback(8, 0, 0); // 8 for GLUT_KEY_BACKSPACE
    mu_assert_string_eq("", globalTextInputBuffer);
}

MU_TEST(test_special_backspace_empty_buffer) {
    setup_buffer("");
    specialKeyboardCallback(8, 0, 0); // 8 for GLUT_KEY_BACKSPACE
    mu_assert_string_eq("", globalTextInputBuffer);
}

MU_TEST(test_special_other_key) {
    setup_buffer("abc");
    // Assuming GLUT_KEY_LEFT is some other special key, e.g., 100
    // The actual value doesn't matter as long as it's not GLUT_KEY_BACKSPACE (8)
    int OTHER_SPECIAL_KEY = 100; 
    specialKeyboardCallback(OTHER_SPECIAL_KEY, 0, 0);
    mu_assert_string_eq("abc", globalTextInputBuffer);
}


// --- Test Suite Definition ---
MU_TEST_SUITE(text_input_suite) {
    MU_RUN_TEST(test_keyboard_append_to_empty);
    MU_RUN_TEST(test_keyboard_append_multiple);
    MU_RUN_TEST(test_keyboard_append_to_almost_full);
    MU_RUN_TEST(test_keyboard_append_to_full);
    
    MU_RUN_TEST(test_special_backspace_multiple_chars);
    MU_RUN_TEST(test_special_backspace_single_char);
    MU_RUN_TEST(test_special_backspace_empty_buffer);
    MU_RUN_TEST(test_special_other_key);
}

// --- Main Test Runner ---
int main(int argc, char *argv[]) {
    (void)argc; // Unused
    (void)argv; // Unused

    printf("--- Running Text Input Unit Tests ---\n");
    MU_RUN_SUITE(text_input_suite);
    MU_REPORT();
    printf("--- Text Input Unit Tests Finished ---\n");
    return MU_EXIT_CODE;
}
