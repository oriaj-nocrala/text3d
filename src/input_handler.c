#include "input_handler.h"
#include "keybindings.h"

#include <stdio.h>
#include <string.h>      // Para strlen, memmove
#include <GL/freeglut.h> // Para glutPostRedisplay y constantes GLUT_KEY_*

// --- Variables globales de main.c a las que este módulo necesita acceder ---
// Estas están definidas en main.c
extern char globalTextInputBuffer[256];
extern size_t globalCursorBytePos;

// --- Variable estática (ámbito de archivo) para este módulo ---
static unsigned char pending_dead_key = 0; // 0: ninguna, 180: tilde aguda, 168: diéresis

// --- Definiciones para manejo de teclas ---

#define ACUTE_ACCENT_DEAD_KEY_LATIN1 180 // ´ (0xB4) - Tecla muerta tilde aguda
#define DIAERESIS_DEAD_KEY_LATIN1    168 // ¨ (0xA8) - Tecla muerta diéresis

#define SPACE_KEY_ASCII    32  // ' '
#define APOSTROPHE_ASCII   39  // '

// UTF-8 para el carácter de tilde aguda ´ (U+00B4)
#define ACUTE_ACCENT_CHAR_UTF8_B1 0xC2
#define ACUTE_ACCENT_CHAR_UTF8_B2 0xB4

// UTF-8 para el carácter de diéresis ¨ (U+00A8)
#define DIAERESIS_CHAR_UTF8_B1 0xC2
#define DIAERESIS_CHAR_UTF8_B2 0xA8

// Fallbacks para constantes de teclas especiales GLUT (idealmente de freeglut.h)
#ifndef GLUT_KEY_SHIFT_L
#define GLUT_KEY_SHIFT_L 112
#endif
#ifndef GLUT_KEY_SHIFT_R
#define GLUT_KEY_SHIFT_R 113
#endif
#ifndef GLUT_KEY_CTRL_L
#define GLUT_KEY_CTRL_L 114
#endif
#ifndef GLUT_KEY_CTRL_R
#define GLUT_KEY_CTRL_R 115
#endif
#ifndef GLUT_KEY_ALT_L
#define GLUT_KEY_ALT_L 116
#endif
#ifndef GLUT_KEY_ALT_R
#define GLUT_KEY_ALT_R 117
#endif

// --- Funciones Auxiliares UTF-8 ---
static size_t get_utf8_char_len_from_first_byte(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; 
}

static size_t utf8_get_next_char_start_offset(const char* str, size_t str_len, size_t current_byte_offset) {
    if (current_byte_offset >= str_len) {
        return str_len;
    }
    size_t char_len = get_utf8_char_len_from_first_byte((unsigned char)str[current_byte_offset]);
    size_t next_offset = current_byte_offset + char_len;
    return (next_offset > str_len) ? str_len : next_offset;
}

static size_t utf8_get_prev_char_start_offset(const char* str, size_t current_byte_offset) {
    if (current_byte_offset == 0) {
        return 0;
    }
    size_t offset = current_byte_offset;
    do {
        offset--;
    } while (offset > 0 && ((unsigned char)str[offset] & 0xC0) == 0x80);
    return offset;
}

// --- Implementaciones de los Callbacks ---
void app_keyboard_callback(unsigned char key, int x, int y) {
    // COPIA AQUÍ LA IMPLEMENTACIÓN COMPLETA Y FUNCIONAL DE 
    // 'keyboardCallback' DE TU main.c (la última versión que te proporcioné)
    // Ejemplo de la estructura que debería tener:
    printf("app_keyboard_callback: START -> Raw byte (decimal): %d, cursor_pos: %zu, pending_dead_key: %d, buffer: \"%s\"\n",
           (int)key, globalCursorBytePos, pending_dead_key, globalTextInputBuffer);
    fflush(stdout);

    int needs_redisplay_from_kb = 0;

    if (key == 8) { // Backspace (ASCII 8)
        pending_dead_key = 0;
        printf("app_keyboard_callback: Handling Backspace (ASCII 8).\n");
        fflush(stdout);
        if (globalCursorBytePos > 0) {
            size_t original_cursor_pos = globalCursorBytePos;
            size_t char_start_to_remove_offset = utf8_get_prev_char_start_offset(globalTextInputBuffer, original_cursor_pos);
            size_t text_len = strlen(globalTextInputBuffer);
            memmove(&globalTextInputBuffer[char_start_to_remove_offset],
                    &globalTextInputBuffer[original_cursor_pos],
                    text_len - original_cursor_pos + 1);
            globalCursorBytePos = char_start_to_remove_offset;
            needs_redisplay_from_kb = 1;
            printf("app_keyboard_callback: After Backspace, New CursorPos: %zu, Buffer: \"%s\"\n",
                   globalCursorBytePos, globalTextInputBuffer);
        } else {
            printf("app_keyboard_callback: Cursor at start/Buffer empty, Backspace does nothing.\n");
        }
        fflush(stdout);
        if (needs_redisplay_from_kb) glutPostRedisplay();
        return;
    } else if (key == APP_KEY_DEL) { // Tecla Suprimir (DEL ASCII 127)
        pending_dead_key = 0;
        printf("app_keyboard_callback: Handling Delete (ASCII 127).\n");
        fflush(stdout);
        size_t text_len = strlen(globalTextInputBuffer);
        if (globalCursorBytePos < text_len) { 
            size_t char_len_to_delete = get_utf8_char_len_from_first_byte((unsigned char)globalTextInputBuffer[globalCursorBytePos]);
            memmove(&globalTextInputBuffer[globalCursorBytePos],
                    &globalTextInputBuffer[globalCursorBytePos + char_len_to_delete],
                    text_len - (globalCursorBytePos + char_len_to_delete) + 1);
            needs_redisplay_from_kb = 1;
            printf("app_keyboard_callback: After Delete, CursorPos: %zu, Buffer: \"%s\"\n", globalCursorBytePos, globalTextInputBuffer);
        } else {
            printf("app_keyboard_callback: Cursor at end, Delete does nothing.\n");
        }
        fflush(stdout);
        if (needs_redisplay_from_kb) glutPostRedisplay();
        return;
    }

    unsigned char char_to_add_b1 = 0, char_to_add_b2 = 0;
    int bytes_to_add = 0;
    unsigned char key_to_reprocess_after_dead_key_insertion = 0;

    if (pending_dead_key != 0) {
        unsigned char composed_char_latin1 = 0;
        if (pending_dead_key == ACUTE_ACCENT_DEAD_KEY_LATIN1) {
            switch (key) {
                case 'a': composed_char_latin1 = 225; break; case 'e': composed_char_latin1 = 233; break;
                case 'i': composed_char_latin1 = 237; break; case 'o': composed_char_latin1 = 243; break;
                case 'u': composed_char_latin1 = 250; break; case 'A': composed_char_latin1 = 193; break;
                case 'E': composed_char_latin1 = 201; break; case 'I': composed_char_latin1 = 205; break;
                case 'O': composed_char_latin1 = 211; break; case 'U': composed_char_latin1 = 218; break;
                case ACUTE_ACCENT_DEAD_KEY_LATIN1: 
                    char_to_add_b1 = ACUTE_ACCENT_CHAR_UTF8_B1; char_to_add_b2 = ACUTE_ACCENT_CHAR_UTF8_B2; bytes_to_add = 2; break;
                case SPACE_KEY_ASCII: 
                    char_to_add_b1 = APOSTROPHE_ASCII; bytes_to_add = 1; break;
                default: 
                    char_to_add_b1 = ACUTE_ACCENT_CHAR_UTF8_B1; char_to_add_b2 = ACUTE_ACCENT_CHAR_UTF8_B2; bytes_to_add = 2;
                    key_to_reprocess_after_dead_key_insertion = key; 
                    break;
            }
            if (composed_char_latin1 != 0) { 
                char_to_add_b1 = 0xC0 | (composed_char_latin1 >> 6);
                char_to_add_b2 = 0x80 | (composed_char_latin1 & 0x3F);
                bytes_to_add = 2;
                key_to_reprocess_after_dead_key_insertion = 0; 
            }
        } else if (pending_dead_key == DIAERESIS_DEAD_KEY_LATIN1) {
            switch (key) {
                case 'u': composed_char_latin1 = 252; break; 
                case 'U': composed_char_latin1 = 220; break; 
                case DIAERESIS_DEAD_KEY_LATIN1: 
                    char_to_add_b1 = DIAERESIS_CHAR_UTF8_B1; char_to_add_b2 = DIAERESIS_CHAR_UTF8_B2; bytes_to_add = 2; break;
                case SPACE_KEY_ASCII: 
                    char_to_add_b1 = DIAERESIS_CHAR_UTF8_B1; char_to_add_b2 = DIAERESIS_CHAR_UTF8_B2; bytes_to_add = 2; break;
                default: 
                    char_to_add_b1 = DIAERESIS_CHAR_UTF8_B1; char_to_add_b2 = DIAERESIS_CHAR_UTF8_B2; bytes_to_add = 2;
                    key_to_reprocess_after_dead_key_insertion = key;
                    break;
            }
            if (composed_char_latin1 != 0) {
                char_to_add_b1 = 0xC0 | (composed_char_latin1 >> 6);
                char_to_add_b2 = 0x80 | (composed_char_latin1 & 0x3F);
                bytes_to_add = 2;
                key_to_reprocess_after_dead_key_insertion = 0;
            }
        }
        pending_dead_key = 0; 
    }

    if (bytes_to_add == 0 && key_to_reprocess_after_dead_key_insertion == 0) {
        if (key == ACUTE_ACCENT_DEAD_KEY_LATIN1 || key == DIAERESIS_DEAD_KEY_LATIN1) {
            pending_dead_key = key;
            printf("app_keyboard_callback: Storing dead key: %d\n", pending_dead_key);
        } else if (key >= 128) { 
            char_to_add_b1 = 0xC0 | (key >> 6);
            char_to_add_b2 = 0x80 | (key & 0x3F);
            bytes_to_add = 2;
        } else { 
            char_to_add_b1 = key;
            bytes_to_add = 1;
        }
    }

    if (bytes_to_add > 0) {
        size_t text_len = strlen(globalTextInputBuffer);
        if (text_len + bytes_to_add < sizeof(globalTextInputBuffer)) {
            if (globalCursorBytePos < text_len) { 
                memmove(&globalTextInputBuffer[globalCursorBytePos + bytes_to_add],
                        &globalTextInputBuffer[globalCursorBytePos],
                        text_len - globalCursorBytePos + 1);
            } else { 
                globalTextInputBuffer[text_len + bytes_to_add] = '\0'; 
            }
            globalTextInputBuffer[globalCursorBytePos] = char_to_add_b1;
            if (bytes_to_add >= 2) globalTextInputBuffer[globalCursorBytePos + 1] = char_to_add_b2;
            
            globalCursorBytePos += bytes_to_add;
            needs_redisplay_from_kb = 1;
            printf("app_keyboard_callback: Inserted %d bytes. Cursor at %zu. Buffer: \"%s\"\n",
                   bytes_to_add, globalCursorBytePos, globalTextInputBuffer);
        } else {
            printf("app_keyboard_callback: Buffer full. Cannot insert.\n"); printf("\a");
        }
        fflush(stdout);
    }
    
    if (key_to_reprocess_after_dead_key_insertion != 0) {
        unsigned char reprocess_key = key_to_reprocess_after_dead_key_insertion;
        printf("app_keyboard_callback: Reprocessing original key %d after dead key char insertion.\n", reprocess_key);
        fflush(stdout);
        app_keyboard_callback(reprocess_key, x, y); // Llamada recursiva
    } else if (needs_redisplay_from_kb) { 
        glutPostRedisplay();
    }

    printf("app_keyboard_callback: END -> Raw byte: %d, CursorPos: %zu, pending_dead_key: %d, Buffer: \"%s\"\n",
           (int)key, globalCursorBytePos, pending_dead_key, globalTextInputBuffer);
    fflush(stdout);
}


void app_special_keyboard_callback(int key, int x, int y) {
    // COPIA AQUÍ LA IMPLEMENTACIÓN COMPLETA Y FUNCIONAL DE
    // 'specialKeyboardCallback' DE TU main.c (la última versión que te proporcioné)
    // Ejemplo de la estructura que debería tener:
    printf("--- app_special_keyboard_callback --- key: %d, current_cursor_pos: %zu, text_len: %zu, pending_dead_key: %d\n",
           key, globalCursorBytePos, strlen(globalTextInputBuffer), pending_dead_key);
    fflush(stdout);

    // --- NUEVO PRINTF DE DIAGNÓSTICO ---
    printf("DEBUG MACROS in app_special_keyboard_callback: APP_KEY_HOME=%d, APP_KEY_END=%d\n",
        APP_KEY_HOME, APP_KEY_END);
    fflush(stdout);

    int needs_redisplay_from_special = 0;
    int is_modifier_key = 0;

    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R ||
        key == GLUT_KEY_CTRL_L  || key == GLUT_KEY_CTRL_R  ||
        key == GLUT_KEY_ALT_L   || key == GLUT_KEY_ALT_R) {
        is_modifier_key = 1;
        printf("Modifier key %d detected. Will not flush pending dead key.\n", key);
    }

    if (pending_dead_key != 0 && !is_modifier_key) {
        printf("Action special key (%d) with pending dead key (%d). Inserting dead key char.\n", key, pending_dead_key);
        unsigned char dk_b1 = 0, dk_b2 = 0;
        int dk_bytes = 0;
        if (pending_dead_key == ACUTE_ACCENT_DEAD_KEY_LATIN1) {
            dk_b1 = ACUTE_ACCENT_CHAR_UTF8_B1; dk_b2 = ACUTE_ACCENT_CHAR_UTF8_B2; dk_bytes = 2;
        } else if (pending_dead_key == DIAERESIS_DEAD_KEY_LATIN1) {
            dk_b1 = DIAERESIS_CHAR_UTF8_B1; dk_b2 = DIAERESIS_CHAR_UTF8_B2; dk_bytes = 2;
        }

        if (dk_bytes > 0) {
            size_t text_len = strlen(globalTextInputBuffer);
            if (text_len + dk_bytes < sizeof(globalTextInputBuffer)) {
                if (globalCursorBytePos < text_len) {
                    memmove(&globalTextInputBuffer[globalCursorBytePos + dk_bytes],
                            &globalTextInputBuffer[globalCursorBytePos],
                            text_len - globalCursorBytePos + 1);
                } else { globalTextInputBuffer[text_len + dk_bytes] = '\0'; }
                globalTextInputBuffer[globalCursorBytePos] = dk_b1;
                if (dk_bytes >= 2) globalTextInputBuffer[globalCursorBytePos + 1] = dk_b2;
                globalCursorBytePos += dk_bytes;
                needs_redisplay_from_special = 1;
                printf("Inserted pending dead key. New cursor: %zu, Buffer: \"%s\"\n", globalCursorBytePos, globalTextInputBuffer);
            } else { printf("\a"); fflush(stdout); }
        }
        pending_dead_key = 0;
    }

    if (is_modifier_key) {
        printf("app_special_keyboard_callback: END (modifier key %d)\n", key);
        fflush(stdout);
        if (needs_redisplay_from_special) glutPostRedisplay(); // Si la tecla muerta se insertó
        return;
    }

    size_t current_str_len = strlen(globalTextInputBuffer);
    size_t original_cursor_for_debug = globalCursorBytePos;

    switch (key) {
        case APP_KEY_LEFT:
            if (globalCursorBytePos > 0) {
                globalCursorBytePos = utf8_get_prev_char_start_offset(globalTextInputBuffer, globalCursorBytePos);
                if (original_cursor_for_debug != globalCursorBytePos) needs_redisplay_from_special = 1;
            }
            printf("LEFT: old_pos=%zu, new_pos=%zu\n", original_cursor_for_debug, globalCursorBytePos);
            break;
        case APP_KEY_RIGHT:
            if (globalCursorBytePos < current_str_len) {
                globalCursorBytePos = utf8_get_next_char_start_offset(globalTextInputBuffer, current_str_len, globalCursorBytePos);
                if (original_cursor_for_debug != globalCursorBytePos) needs_redisplay_from_special = 1;
            }
            printf("RIGHT: old_pos=%zu, new_pos=%zu, str_len=%zu\n", original_cursor_for_debug, globalCursorBytePos, current_str_len);
            break;
        case APP_KEY_HOME: // This should be case 104:
            if (globalCursorBytePos != 0) needs_redisplay_from_special = 1; // Set flag *before* changing
            globalCursorBytePos = 0;
            // needs_redisplay_from_special = 1; // Redundant if set before
            printf("HOME: new_pos=%zu\n", globalCursorBytePos);
            break;
        case APP_KEY_END: // This should be case 105:
             if (globalCursorBytePos != current_str_len) needs_redisplay_from_special = 1; // Set flag *before* changing
            globalCursorBytePos = current_str_len;
            // needs_redisplay_from_special = 1; // Redundant
            printf("END: new_pos=%zu, str_len=%zu\n", globalCursorBytePos, current_str_len);
            break;
        default:
            printf("Other (non-modifier) special key: %d\n", key);
            break;
    }

    if (needs_redisplay_from_special) {
        printf("app_special_keyboard_callback: Cursor/text changed. Redisplaying. Cursor at: %zu\n", globalCursorBytePos);
        glutPostRedisplay();
    }
    printf("app_special_keyboard_callback: END (processed action key %d)\n", key);
    fflush(stdout);
}