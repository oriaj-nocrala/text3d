#include "minunit.h"
#include "glyph_manager.h" 
#include "freetype_handler.h" 
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>   

extern FT_Library ftLibrary; 
extern FT_Face ftFace;       

const char* testFontPathForGlyphManager = "tests/fonts/test_font.ttf"; 

static int setup_freetype_for_glyph_tests() { 
    if (initFreeType() != 0) {
        fprintf(stderr, "ERROR_SETUP_GLYPH_TESTS: Fallo al inicializar FreeType.\n");
        return -1; 
    }
    if (loadFonts(testFontPathForGlyphManager, NULL) != 0) { 
        fprintf(stderr, "ERROR_SETUP_GLYPH_TESTS: Fallo al cargar la fuente de prueba '%s'.\n", testFontPathForGlyphManager);
        cleanupFreeType(); 
        return -1; 
    }
    return 0; 
}

static void teardown_freetype_for_glyph_tests() {
    cleanupFreeType();
}

MU_TEST(test_init_and_cleanup_glyph_cache) {
    if (setup_freetype_for_glyph_tests() != 0) {
        mu_fail("Fallo en la configuración de FreeType para test_init_and_cleanup_glyph_cache.");
        return;
    }

    mu_assert_int_eq(0, initGlyphCache()); 
    
    int all_null = 1;
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        if (glyphHashTable[i] != NULL) {
            all_null = 0;
            break;
        }
    }
    mu_check(all_null);

    cleanupGlyphCache(); // Esta función ahora tiene condicionales para UNIT_TESTING
    
    all_null = 1;
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        if (glyphHashTable[i] != NULL) {
            all_null = 0;
            break; 
        }
    }
    mu_check(all_null); 

    teardown_freetype_for_glyph_tests();
}


MU_TEST(test_get_glyph_info_basic_ascii) {
    if (setup_freetype_for_glyph_tests() != 0) {
        mu_fail("Fallo en la configuración de FreeType para test_get_glyph_info_basic_ascii.");
        return;
    }
    initGlyphCache();

    FT_ULong char_A = 'A'; 
    GlyphInfo gi_A = getGlyphInfo(char_A);

#ifdef UNIT_TESTING
    mu_assert_int_eq(0, gi_A.vao); // En testing, VAO será 0
    mu_assert_int_eq(0, gi_A.vbo);
    mu_assert_int_eq(0, gi_A.ebo);
#else
    mu_check(gi_A.vao != 0); // En ejecución normal, VAO debería existir
    mu_check(gi_A.vbo != 0);
    mu_check(gi_A.ebo != 0);
#endif
    // Estas comprobaciones deberían ser válidas en ambos casos si la fuente y el glifo son válidos
    mu_check(gi_A.indexCount > 0); 
    mu_check(gi_A.advanceX > 0.0f); 

    GlyphInfo gi_A_cached = getGlyphInfo(char_A);
#ifdef UNIT_TESTING
    mu_assert_int_eq(0, gi_A_cached.vao);
#else
    mu_assert_int_eq(gi_A.vao, gi_A_cached.vao);
#endif
    mu_assert_int_eq(gi_A.indexCount, gi_A_cached.indexCount);
    mu_check(fabs(gi_A.advanceX - gi_A_cached.advanceX) < 1e-5);

    cleanupGlyphCache();
    teardown_freetype_for_glyph_tests();
}

MU_TEST(test_get_glyph_info_space) {
     if (setup_freetype_for_glyph_tests() != 0) {
        mu_fail("Fallo en la configuración de FreeType para test_get_glyph_info_space.");
        return;
    }
    initGlyphCache();

    FT_ULong char_space = ' '; 
    GlyphInfo gi_space = getGlyphInfo(char_space);

    mu_assert_int_eq(0, gi_space.vao); // El espacio no tiene VAO
    mu_assert_int_eq(0, gi_space.indexCount); // El espacio no tiene índices
    mu_check(gi_space.advanceX > 0.0f); 

    cleanupGlyphCache();
    teardown_freetype_for_glyph_tests();
}

MU_TEST(test_get_glyph_info_unicode_and_fallback) {
    if (setup_freetype_for_glyph_tests() != 0) {
        mu_fail("Fallo en la configuración de FreeType para test_get_glyph_info_unicode_and_fallback.");
        return;
    }
    initGlyphCache();

    FT_ULong char_euro = 0x20AC; 
    GlyphInfo gi_euro = getGlyphInfo(char_euro);

#ifdef UNIT_TESTING
    mu_assert_int_eq(0, gi_euro.vao);
#else
    mu_check(gi_euro.vao != 0);
#endif
    mu_check(gi_euro.indexCount > 0);
    mu_check(gi_euro.advanceX > 0.0f);
    
    cleanupGlyphCache();
    teardown_freetype_for_glyph_tests();
}


MU_TEST_SUITE(glyph_manager_suite) {
    MU_RUN_TEST(test_init_and_cleanup_glyph_cache);
    MU_RUN_TEST(test_get_glyph_info_basic_ascii);
    MU_RUN_TEST(test_get_glyph_info_space);
    MU_RUN_TEST(test_get_glyph_info_unicode_and_fallback);
}

int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv; 

    FILE* f_check = fopen(testFontPathForGlyphManager, "rb"); 
    if (!f_check) {
        fprintf(stderr, "ADVERTENCIA CRÍTICA PARA PRUEBAS DE GLYPH_MANAGER: El archivo de fuente de prueba '%s' no se encontró.\n", testFontPathForGlyphManager);
        fprintf(stderr, "Todas las pruebas de glyph_manager probablemente fallarán debido a que no se puede cargar la fuente.\n");
        fprintf(stderr, "Asegúrate de que 'tests/fonts/test_font.ttf' existe y es una fuente válida.\n");
    } else {
        fclose(f_check);
    }

    MU_RUN_SUITE(glyph_manager_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}