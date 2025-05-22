#include "minunit.h"
#include "freetype_handler.h" 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 

// Variables externas de freetype_handler.c
// freetype_handler.h ya las declara como extern
extern FT_Library ftLibrary; 
extern FT_Face ftFace;
extern FT_Face ftEmojiFace;

// Define rutas a fuentes válidas para tus pruebas.
// Necesitarás tener un `test_font.ttf` (y opcionalmente `test_emoji_font.ttf`)
// en la carpeta `tests/fonts/` para que estas pruebas pasen.
const char* validFontPath = "tests/fonts/test_font.ttf"; 
const char* validEmojiFontPath = "tests/fonts/test_emoji_font.ttf"; 
const char* invalidFontPath = "tests/fonts/non_existent_font.ttf";


MU_TEST(test_initFreeType_success) {
    mu_assert_int_eq(0, initFreeType());
    mu_check(ftLibrary != NULL); // Verifica que la librería se inicializó
    cleanupFreeType(); 
}

MU_TEST(test_loadFonts_main_only_success) {
    mu_assert_int_eq(0, initFreeType());
    // Prueba cargando solo la fuente principal, emojiPath es NULL
    mu_assert_int_eq(0, loadFonts(validFontPath, NULL)); 
    mu_check(ftFace != NULL); // La cara principal debería cargarse
    mu_check(ftEmojiFace == NULL); // La cara de emoji no debería cargarse
    cleanupFreeType();
}

MU_TEST(test_loadFonts_main_and_emoji_success) {
    FILE* tempFontFile = fopen(validFontPath, "rb");
    FILE* tempEmojiFile = fopen(validEmojiFontPath, "rb");

    if (!tempFontFile) {
        // Reemplaza mu_warn con printf a stderr
        fprintf(stderr, "\nADVERTENCIA (test_loadFonts_main_and_emoji_success): tests/fonts/test_font.ttf no encontrado. Omitiendo prueba.\n");
        if (tempEmojiFile) fclose(tempEmojiFile);
        // Para MinUnit, una prueba omitida usualmente no se cuenta como fallo, 
        // pero tampoco como éxito total si no se ejecuta la aserción principal.
        // Podemos simplemente retornar para omitirla.
        return; 
    }
    fclose(tempFontFile);
    
    mu_assert_int_eq(0, initFreeType());
    mu_assert_int_eq(0, loadFonts(validFontPath, tempEmojiFile ? validEmojiFontPath : NULL));
    mu_check(ftFace != NULL); 

    if (tempEmojiFile) {
        mu_check(ftEmojiFace != NULL); 
        fclose(tempEmojiFile);
    } else {
        mu_check(ftEmojiFace == NULL);
        // Reemplaza mu_warn con printf a stderr
        fprintf(stderr, "\nADVERTENCIA (test_loadFonts_main_and_emoji_success): tests/fonts/test_emoji_font.ttf no encontrado o no cargado.\n");
    }
    cleanupFreeType();
}


MU_TEST(test_loadFonts_main_fail_invalid_path) {
    mu_assert_int_eq(0, initFreeType());
    mu_check(loadFonts(invalidFontPath, NULL) != 0); 
    mu_check(ftFace == NULL); 
    cleanupFreeType();
}

MU_TEST(test_loadFonts_emoji_fail_does_not_affect_main) {
    mu_assert_int_eq(0, initFreeType());
    mu_assert_int_eq(0, loadFonts(validFontPath, invalidFontPath)); 
    mu_check(ftFace != NULL); 
    mu_check(ftEmojiFace == NULL); 
    cleanupFreeType();
}


MU_TEST(test_cleanupFreeType_multiple_times) {
    mu_assert_int_eq(0, initFreeType());
    loadFonts(validFontPath, NULL); 
    cleanupFreeType();
    mu_check(ftFace == NULL && ftLibrary == NULL && ftEmojiFace == NULL);
    
    cleanupFreeType(); 
    mu_check(ftFace == NULL && ftLibrary == NULL && ftEmojiFace == NULL); 
}

MU_TEST(test_font_properties_after_loadFonts) {
    mu_assert_int_eq(0, initFreeType());
    mu_assert_int_eq(0, loadFonts(validFontPath, NULL));

    const char* family_name = getFontFamilyName();
    mu_check(family_name != NULL);
    if (family_name) { 
        mu_check(strlen(family_name) > 0);
    }

    const char* style_name = getFontStyleName();
    mu_check(style_name != NULL);
    if (style_name) { 
        mu_check(strlen(style_name) > 0);
    }

    long num_glyphs = getFontNumGlyphs();
    mu_check(num_glyphs > 0); 
    
    cleanupFreeType();
}

MU_TEST(test_loadFonts_without_init) {
    mu_check(loadFonts(validFontPath, NULL) == -3);
}


MU_TEST_SUITE(freetype_handler_suite) {
    MU_RUN_TEST(test_initFreeType_success);
    MU_RUN_TEST(test_loadFonts_main_only_success);
    MU_RUN_TEST(test_loadFonts_main_and_emoji_success); // Ahora se llama a esta prueba
    MU_RUN_TEST(test_loadFonts_main_fail_invalid_path);
    MU_RUN_TEST(test_loadFonts_emoji_fail_does_not_affect_main);
    MU_RUN_TEST(test_cleanupFreeType_multiple_times);
    MU_RUN_TEST(test_font_properties_after_loadFonts);
    MU_RUN_TEST(test_loadFonts_without_init);
}

int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv; 

    // Comprobación de la existencia de test_font.ttf
    FILE* f_main_check = fopen(validFontPath, "rb"); 
    if (!f_main_check) {
        fprintf(stderr, "ADVERTENCIA CRÍTICA PARA PRUEBAS: El archivo de fuente principal de prueba '%s' no se encontró.\n", validFontPath);
        fprintf(stderr, "Muchas pruebas de freetype_handler fallarán o se omitirán.\n");
        fprintf(stderr, "Asegúrate de que 'tests/fonts/test_font.ttf' existe y es una fuente válida.\n");
    } else {
        fclose(f_main_check);
    }
    // Comprobación opcional para la fuente de emoji
    FILE* f_emoji_check = fopen(validEmojiFontPath, "rb");
    if (!f_emoji_check) {
         fprintf(stderr, "INFO PARA PRUEBAS: El archivo de fuente de emoji de prueba '%s' no se encontró. Las pruebas de fallback de emoji pueden no ser completas.\n", validEmojiFontPath);
    } else {
        fclose(f_emoji_check);
    }

    MU_RUN_SUITE(freetype_handler_suite);
    MU_REPORT();
    return MU_EXIT_CODE; 
}