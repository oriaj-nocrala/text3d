#include "minunit.h"
#include "keybindings.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h> // Para size_t
#include <GL/gl.h>  // Para GLuint (aunque no se usa directamente en los tests de lógica de input)
#include <GL/freeglut.h> // <<--- AÑADE ESTA LÍNEA

#define KEY_ACUTE_DEAD 180 // ´
#define KEY_DIAERESIS_DEAD 168 // ¨ (Asegúrate que este es el código que envía tu tecla ¨)
#define KEY_SPACE 32


// Variables globales definidas en main.c y usadas por input_handler.c
extern char globalTextInputBuffer[256];
extern size_t globalCursorBytePos; // <--- AÑADIDO

// Callbacks definidos en input_handler.c
extern void app_keyboard_callback(unsigned char key, int x, int y);
extern void app_special_keyboard_callback(int key, int x, int y);

// --- Dummies (simulacros) ---
void glutPostRedisplay(void) {
    // No hace nada en los tests unitarios de lógica de buffer
}

// Los siguientes dummies son necesarios si text_input_test.c se enlaza con
// un main_module.o (compilado desde main.c con -DUNIT_TESTING) que aún
// defina display() o cleanup() que los llamen.
// Si tu Makefile para tests solo enlaza text_input_test.o con input_handler.o y main_globals.o (solo para los globals),
// podrías no necesitar todos estos si los callbacks de input no los llaman.
// Por seguridad, los mantenemos si la estructura de enlazado lo requiere.
void renderText(GLuint program, const char* text, size_t cursorBytePos) { // Modificado para coincidir con la nueva firma
    (void)program; (void)text; (void)cursorBytePos; /* Dummy */
}
void cleanupGlyphCache() { /* Dummy */ }
void cleanupOpenGL(GLuint program) { (void)program; /* Dummy */ }
void cleanupFreeType() { /* Dummy */ }

// --- Función de Ayuda para Configurar el Buffer y Cursor ---
static void setup_test_state(const char* initial_content, size_t cursor_pos_override) {
    if (initial_content == NULL) {
        globalTextInputBuffer[0] = '\0';
    } else {
        strncpy(globalTextInputBuffer, initial_content, sizeof(globalTextInputBuffer) - 1);
        globalTextInputBuffer[sizeof(globalTextInputBuffer) - 1] = '\0';
    }
    // Si cursor_pos_override es un valor sentinel (ej. SIZE_MAX), poner cursor al final.
    // De lo contrario, usar el valor proporcionado.
    if (cursor_pos_override == (size_t)-1) { // Usar -1 como sentinel para "final del string"
        globalCursorBytePos = strlen(globalTextInputBuffer);
    } else {
        globalCursorBytePos = cursor_pos_override;
    }
    // Nota: pending_dead_key es static en input_handler.c y no se puede resetear directamente
    // desde aquí sin una función de API. Para estos tests, asumimos que no interfiere
    // o que las primeras teclas (como backspace) la resetean.
}


// --- Test Cases para app_keyboard_callback ---

MU_TEST(test_kb_append_to_empty) {
    setup_test_state("", (size_t)-1); // Cursor al final (posición 0)
    app_keyboard_callback('a', 0, 0);
    mu_assert_string_eq("a", globalTextInputBuffer);
    mu_assert_int_eq(1, globalCursorBytePos); // Cursor después de 'a'
}

MU_TEST(test_kb_append_multiple) {
    setup_test_state("abc", (size_t)-1); // Cursor después de 'c' (posición 3)
    app_keyboard_callback('d', 0, 0);
    mu_assert_string_eq("abcd", globalTextInputBuffer);
    mu_assert_int_eq(4, globalCursorBytePos);
    app_keyboard_callback('e', 0, 0);
    mu_assert_string_eq("abcde", globalTextInputBuffer);
    mu_assert_int_eq(5, globalCursorBytePos);
}

MU_TEST(test_kb_insert_at_beginning) {
    setup_test_state("bc", 0); // Cursor al inicio (posición 0)
    app_keyboard_callback('a', 0, 0);
    mu_assert_string_eq("abc", globalTextInputBuffer);
    mu_assert_int_eq(1, globalCursorBytePos); // Cursor después de 'a'
}

MU_TEST(test_kb_insert_in_middle) {
    setup_test_state("ac", 1); // Cursor después de 'a' (posición 1)
    app_keyboard_callback('b', 0, 0);
    mu_assert_string_eq("abc", globalTextInputBuffer);
    mu_assert_int_eq(2, globalCursorBytePos); // Cursor después de 'b'
}


MU_TEST(test_kb_append_to_almost_full) {
    char test_str_initial[256];
    memset(test_str_initial, 'x', 254);
    test_str_initial[254] = '\0';
    setup_test_state(test_str_initial, (size_t)-1); // Cursor al final (posición 254)
    
    app_keyboard_callback('y', 0, 0);
    
    char expected_str[256];
    memset(expected_str, 'x', 254);
    expected_str[254] = 'y';
    expected_str[255] = '\0';
    mu_assert_string_eq(expected_str, globalTextInputBuffer);
    mu_assert_int_eq(255, globalCursorBytePos);
}

MU_TEST(test_kb_append_to_full) {
    char test_str_full[256];
    memset(test_str_full, 'x', 255); // Buffer completamente lleno con 'x'
    test_str_full[255] = '\0';
    setup_test_state(test_str_full, (size_t)-1); // Cursor en pos 255

    app_keyboard_callback('y', 0, 0); // No debería añadir
    mu_assert_string_eq(test_str_full, globalTextInputBuffer);
    mu_assert_int_eq(255, globalCursorBytePos); // Cursor no debería moverse
    mu_assert_int_eq(255, strlen(globalTextInputBuffer));
}

// --- Test Cases para Backspace y Delete (manejados en app_keyboard_callback) ---

MU_TEST(test_kb_backspace_multiple_chars) {
    setup_test_state("abcde", 5); // Cursor al final: abcde|
    app_keyboard_callback(APP_KEY_BACKSPACE, 0, 0);   // ASCII 8 para Backspace
    mu_assert_string_eq("abcd", globalTextInputBuffer);
    mu_assert_int_eq(4, globalCursorBytePos); // Cursor se mueve a abcd|
}

MU_TEST(test_kb_backspace_in_middle) {
    setup_test_state("abcde", 3); // Cursor en: abc|de
    app_keyboard_callback(APP_KEY_BACKSPACE, 0, 0);   // Borra 'c'
    mu_assert_string_eq("abde", globalTextInputBuffer);
    mu_assert_int_eq(2, globalCursorBytePos); // Cursor en: ab|de
}

MU_TEST(test_kb_backspace_single_char) {
    setup_test_state("a", 1); // Cursor en: a|
    app_keyboard_callback(APP_KEY_BACKSPACE, 0, 0);
    mu_assert_string_eq("", globalTextInputBuffer);
    mu_assert_int_eq(0, globalCursorBytePos);
}

MU_TEST(test_kb_backspace_empty_buffer) {
    setup_test_state("", 0); // Cursor en: |
    app_keyboard_callback(APP_KEY_BACKSPACE, 0, 0);
    mu_assert_string_eq("", globalTextInputBuffer);
    mu_assert_int_eq(0, globalCursorBytePos);
}

MU_TEST(test_kb_delete_in_middle) {
    setup_test_state("abcde", 2); // Cursor en: ab|cde
    app_keyboard_callback(APP_KEY_DEL, 0, 0); // ASCII 127 para Delete (Suprimir)
    mu_assert_string_eq("abde", globalTextInputBuffer); // Borra 'c'
    mu_assert_int_eq(2, globalCursorBytePos); // Cursor se queda en: ab|de
}

MU_TEST(test_kb_delete_at_end) {
    setup_test_state("abc", 3); // Cursor en: abc|
    app_keyboard_callback(APP_KEY_DEL, 0, 0);
    mu_assert_string_eq("abc", globalTextInputBuffer); // No borra nada
    mu_assert_int_eq(3, globalCursorBytePos);      // Cursor se queda al final
}


// --- Test Cases para app_special_keyboard_callback (Flechas, etc.) ---

MU_TEST(test_special_arrow_left) {
    setup_test_state("abc", 3); // Cursor en: abc|
    app_special_keyboard_callback(APP_KEY_LEFT, 0, 0); // APP_KEY_LEFT es 100
    mu_assert_int_eq(2, globalCursorBytePos); // Cursor en: ab|c
    mu_assert_string_eq("abc", globalTextInputBuffer); // El buffer no cambia
}

MU_TEST(test_special_arrow_right_from_middle) {
    setup_test_state("abc", 1); // Cursor en: a|bc
    app_special_keyboard_callback(APP_KEY_RIGHT, 0, 0); // APP_KEY_RIGHT es 102
    mu_assert_int_eq(2, globalCursorBytePos);  // Cursor en: ab|c
    mu_assert_string_eq("abc", globalTextInputBuffer);
}

MU_TEST(test_special_arrow_right_at_end) {
    setup_test_state("abc", 3); // Cursor en: abc|
    app_special_keyboard_callback(APP_KEY_RIGHT, 0, 0); // APP_KEY_RIGHT
    mu_assert_int_eq(3, globalCursorBytePos);  // Cursor se queda al final
    mu_assert_string_eq("abc", globalTextInputBuffer);
}

MU_TEST(test_special_home_key) {
    setup_test_state("abc", 2); // Cursor en: ab|c
    app_special_keyboard_callback(APP_KEY_HOME, 0, 0); // APP_KEY_HOME
    mu_assert_int_eq(0, globalCursorBytePos);  // Cursor al inicio: |abc
    mu_assert_string_eq("abc", globalTextInputBuffer);
}

MU_TEST(test_special_end_key) {
    setup_test_state("abc", 1); // Cursor en: a|bc
    app_special_keyboard_callback(APP_KEY_END, 0, 0); // APP_KEY_END
    mu_assert_int_eq(3, globalCursorBytePos);  // Cursor al final: abc|
    mu_assert_string_eq("abc", globalTextInputBuffer);
}

MU_TEST(test_special_unhandled_key) {
    setup_test_state("abc", 1);
    int UNHANDLED_SPECIAL_KEY = 150; // Un código que no manejamos
    app_special_keyboard_callback(UNHANDLED_SPECIAL_KEY, 0, 0);
    mu_assert_int_eq(1, globalCursorBytePos); // Cursor no debería moverse
    mu_assert_string_eq("abc", globalTextInputBuffer); // Buffer no debería cambiar
}

// --- Test Cases para Teclas Muertas (manejadas en app_keyboard_callback) ---

MU_TEST(test_dk_acute_plus_a) {
    setup_test_state("hola", (size_t)-1); // Cursor al final: hola|
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´
    mu_assert_string_eq("hola", globalTextInputBuffer); // No cambia aún
    mu_assert_int_eq(4, globalCursorBytePos);      // Cursor no se mueve aún
    // pending_dead_key ahora es KEY_ACUTE_DEAD internamente

    app_keyboard_callback('a', 0, 0);              // a
    mu_assert_string_eq("holaá", globalTextInputBuffer); // UTF-8 para á es C3 A1
    mu_assert_int_eq(4 + 2, globalCursorBytePos);    // Cursor después de á (2 bytes)
}

MU_TEST(test_dk_acute_plus_E_uppercase) {
    setup_test_state("", 0);
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´
    app_keyboard_callback('E', 0, 0);              // E
    mu_assert_string_eq("É", globalTextInputBuffer); // UTF-8 para É es C3 89
    mu_assert_int_eq(2, globalCursorBytePos);
}

MU_TEST(test_dk_diaeresis_plus_u) {
    setup_test_state("ping", (size_t)-1); // ping|
    app_keyboard_callback(KEY_DIAERESIS_DEAD, 0, 0); // ¨
    app_keyboard_callback('u', 0, 0);                 // u
    mu_assert_string_eq("pingü", globalTextInputBuffer); // UTF-8 para ü es C3 BC
    mu_assert_int_eq(4 + 2, globalCursorBytePos);
}

MU_TEST(test_dk_diaeresis_plus_U_uppercase) {
    setup_test_state("", 0);
    app_keyboard_callback(KEY_DIAERESIS_DEAD, 0, 0); // ¨
    app_keyboard_callback('U', 0, 0);                 // U
    mu_assert_string_eq("Ü", globalTextInputBuffer); // UTF-8 para Ü es C3 9C
    mu_assert_int_eq(2, globalCursorBytePos);
}

MU_TEST(test_dk_acute_plus_acute) { // ´ + ´ -> ´
    setup_test_state("", 0);
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0);
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0);
    mu_assert_string_eq("´", globalTextInputBuffer); // UTF-8 para ´ es C2 B4
    mu_assert_int_eq(2, globalCursorBytePos);
}

MU_TEST(test_dk_diaeresis_plus_diaeresis) { // ¨ + ¨ -> ¨
    setup_test_state("", 0);
    app_keyboard_callback(KEY_DIAERESIS_DEAD, 0, 0);
    app_keyboard_callback(KEY_DIAERESIS_DEAD, 0, 0);
    mu_assert_string_eq("¨", globalTextInputBuffer); // UTF-8 para ¨ es C2 A8
    mu_assert_int_eq(2, globalCursorBytePos);
}

MU_TEST(test_dk_acute_plus_space) { // ´ + espacio -> ' (apóstrofo)
    setup_test_state("", 0);
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0);
    app_keyboard_callback(KEY_SPACE, 0, 0);
    mu_assert_string_eq("'", globalTextInputBuffer); // Apóstrofo ASCII
    mu_assert_int_eq(1, globalCursorBytePos);
}

MU_TEST(test_dk_diaeresis_plus_space) { // ¨ + espacio -> ¨
    setup_test_state("", 0);
    app_keyboard_callback(KEY_DIAERESIS_DEAD, 0, 0);
    app_keyboard_callback(KEY_SPACE, 0, 0);
    mu_assert_string_eq("¨", globalTextInputBuffer); // UTF-8 para ¨
    mu_assert_int_eq(2, globalCursorBytePos);
}

MU_TEST(test_dk_acute_plus_consonant) { // ´ + p -> ´p
    setup_test_state("", 0);
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´
    app_keyboard_callback('p', 0, 0);             // p
    // El resultado esperado es la tilde UTF-8 seguida de 'p'
    char expected[4];
    expected[0] = (char)0xC2; // ´ UTF-8 byte 1
    expected[1] = (char)0xB4; // ´ UTF-8 byte 2
    expected[2] = 'p';
    expected[3] = '\0';
    mu_assert_string_eq(expected, globalTextInputBuffer);
    mu_assert_int_eq(3, globalCursorBytePos); // 2 bytes para ´ + 1 byte para p
}

MU_TEST(test_dk_diaeresis_plus_consonant) { // ¨ + t -> ¨t
    setup_test_state("a", 1); // a|
    app_keyboard_callback(KEY_DIAERESIS_DEAD, 0, 0); // ¨
    app_keyboard_callback('t', 0, 0);                // t
    char expected[5]; // "a" + "¨" + "t" + "\0"
    expected[0] = 'a';
    expected[1] = (char)0xC2; // ¨ UTF-8 byte 1
    expected[2] = (char)0xA8; // ¨ UTF-8 byte 2
    expected[3] = 't';
    expected[4] = '\0';
    mu_assert_string_eq(expected, globalTextInputBuffer);
    mu_assert_int_eq(1 + 2 + 1, globalCursorBytePos); // Cursor después de a¨t
}

MU_TEST(test_dk_acute_then_backspace) {
    setup_test_state("abc", 3); // abc|
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´ (pending)
    // pending_dead_key es ahora 180. El buffer es "abc", cursor en 3.
    app_keyboard_callback(8, 0, 0); // Backspace
    // Backspace debería limpiar pending_dead_key y luego borrar 'c'.
    mu_assert_string_eq("ab", globalTextInputBuffer);
    mu_assert_int_eq(2, globalCursorBytePos);
    // Para verificar que pending_dead_key se limpió, intentamos escribir una 'e'.
    // Si pending_dead_key no se limpió, intentaría formar 'é'.
    app_keyboard_callback('e', 0, 0);
    mu_assert_string_eq("abe", globalTextInputBuffer);
    mu_assert_int_eq(3, globalCursorBytePos);
}

MU_TEST(test_dk_acute_then_delete) {
    setup_test_state("abc", 1); // a|bc
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´ (pending)
    // pending_dead_key es 180. Buffer "abc", cursor en 1.
    app_keyboard_callback(127, 0, 0); // Delete (debería borrar 'b')
    // Delete debería limpiar pending_dead_key y luego borrar 'b'.
    mu_assert_string_eq("ac", globalTextInputBuffer);
    mu_assert_int_eq(1, globalCursorBytePos); // Cursor se queda en a|c
    // Verificar que pending_dead_key se limpió
    app_keyboard_callback('x', 0, 0);
    mu_assert_string_eq("axc", globalTextInputBuffer); // Inserta 'x', no 'x́'
    mu_assert_int_eq(2, globalCursorBytePos);
}

// Test para "dead key + special key (arrow)"
// Esto prueba la interacción entre app_keyboard_callback y app_special_keyboard_callback
// a través del estado compartido pending_dead_key.
MU_TEST(test_dk_acute_then_arrow_left) {
    setup_test_state("b", 1); // b|
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´ (pending_dead_key = 180)
    // Buffer: "b", Cursor: 1, Pending: 180

    app_special_keyboard_callback(APP_KEY_LEFT, 0, 0); // Usar tu macro de keybindings.h
    // app_special_keyboard_callback debería detectar pending_dead_key,
    // insertar ´ (UTF-8 0xC2 0xB4) en la posición actual del cursor (1),
    // mover el cursor a después de ´ (posición 1+2=3),
    // y LUEGO procesar Arrow Left, moviendo el cursor a la izquierda de ´ (posición 1).
    // Buffer esperado: "b´"
    // Cursor esperado: después de "b", antes de "´" (índice 1)

    char expected_buffer[4];
    expected_buffer[0] = 'b';
    expected_buffer[1] = (char)0xC2; // ´
    expected_buffer[2] = (char)0xB4; // ´
    expected_buffer[3] = '\0';
    mu_assert_string_eq(expected_buffer, globalTextInputBuffer);
    mu_assert_int_eq(1, globalCursorBytePos); // Cursor en b|´
}

MU_TEST(test_dk_acute_then_shift_o_compose) {
    setup_test_state("", 0);
    app_keyboard_callback(KEY_ACUTE_DEAD, 0, 0); // ´ pendiente
    // Simular Shift+o enviando 'O'
    app_keyboard_callback('O', 0, 0);
    mu_assert_string_eq("Ó", globalTextInputBuffer); // UTF-8 para Ó es C3 93
    mu_assert_int_eq(2, globalCursorBytePos);
}

// Otros tests que podrías añadir:
// - Secuencias de múltiples teclas muertas (ej. ´¨u -> ¿´ü o ¨ú o ´¨u?)
//   La lógica actual con reprocesamiento de una tecla podría necesitar más robustez para esto.
//   Por ahora, ´¨u resultaría en:
//     1. ´ (pending=180)
//     2. ¨ (key=168). `default` de `pending_dead_key=180` se activa.
//        Inserta ´ (buffer="´", cursor=2). `key_to_reprocess = 168`.
//        Llamada recursiva con key=168.
//     3. En llamada recursiva: key=168. `pending_dead_key=0`. `bytes_to_add=0`. `key_to_reprocess=0`.
//        Bloque `if (bytes_to_add == 0 && key_to_reprocess_after_dead_key_insertion == 0)`:
//        `current_processing_key = 168`. Se almacena `pending_dead_key = 168`. Nada se inserta.
//     Resultado final: Buffer="´", pending_dead_key=168.
//     Si luego se presiona 'u': `pending_dead_key=168`, `key='u'`. Se compone "ü". Buffer="´ü".
//     Esto parece un comportamiento razonable.

// - Teclas muertas al borde del buffer.
// - Más combinaciones si tu teclado las soporta y quieres implementarlas (ej. `^` + vocal).

// --- Test Suite Definition ---
MU_TEST_SUITE(text_input_suite) {
    MU_RUN_TEST(test_kb_append_to_empty);
    MU_RUN_TEST(test_kb_append_multiple);
    MU_RUN_TEST(test_kb_insert_at_beginning);
    MU_RUN_TEST(test_kb_insert_in_middle);
    MU_RUN_TEST(test_kb_append_to_almost_full);
    MU_RUN_TEST(test_kb_append_to_full);
    
    MU_RUN_TEST(test_kb_backspace_multiple_chars);
    MU_RUN_TEST(test_kb_backspace_in_middle);
    MU_RUN_TEST(test_kb_backspace_single_char);
    MU_RUN_TEST(test_kb_backspace_empty_buffer);

    MU_RUN_TEST(test_kb_delete_in_middle);
    MU_RUN_TEST(test_kb_delete_at_end);

    MU_RUN_TEST(test_special_arrow_left);
    MU_RUN_TEST(test_special_arrow_right_from_middle);
    MU_RUN_TEST(test_special_arrow_right_at_end);
    MU_RUN_TEST(test_special_home_key);
    MU_RUN_TEST(test_special_end_key);
    MU_RUN_TEST(test_special_unhandled_key);

    // Nuevos tests para teclas muertas
    MU_RUN_TEST(test_dk_acute_plus_a);
    MU_RUN_TEST(test_dk_acute_plus_E_uppercase);
    MU_RUN_TEST(test_dk_diaeresis_plus_u);
    MU_RUN_TEST(test_dk_diaeresis_plus_U_uppercase);
    MU_RUN_TEST(test_dk_acute_plus_acute);
    MU_RUN_TEST(test_dk_diaeresis_plus_diaeresis);
    MU_RUN_TEST(test_dk_acute_plus_space);
    MU_RUN_TEST(test_dk_diaeresis_plus_space);
    MU_RUN_TEST(test_dk_acute_plus_consonant);
    MU_RUN_TEST(test_dk_diaeresis_plus_consonant);
    MU_RUN_TEST(test_dk_acute_then_backspace);
    MU_RUN_TEST(test_dk_acute_then_delete);
    MU_RUN_TEST(test_dk_acute_then_arrow_left);
    MU_RUN_TEST(test_dk_acute_then_shift_o_compose);
}

// --- Main Test Runner ---
int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv; 

    printf("--- Running Text Input Unit Tests (Refactored) ---\n");
    MU_RUN_SUITE(text_input_suite);
    MU_REPORT();
    printf("--- Text Input Unit Tests Finished ---\n");
    return MU_EXIT_CODE;
}