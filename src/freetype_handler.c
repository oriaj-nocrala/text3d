#include "freetype_handler.h"
#include "glyph_manager.h"
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

FT_Library ftLibrary;
FT_Face ftFace;

int initFreeType() {
    FT_Error error = FT_Init_FreeType(&ftLibrary);
    if (error) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: No se pudo inicializar FreeType. Código de error: %d\n", error);
        return -1; // Fallo
    }
    return 0; // Éxito
}

int loadFont(const char* fontPath) { // Permitir pasar la ruta como argumento
    if (!ftLibrary) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftLibrary no inicializada antes de llamar a loadFont.\n");
        return -3; // Error de precondición
    }
    FT_Error error = FT_New_Face(ftLibrary,
                                 fontPath,
                                 0,
                                 &ftFace);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: Formato de la fuente '%s' no soportado.\n", fontPath);
        // No es necesario llamar a FT_Done_FreeType aquí, se hará en cleanup general si es necesario.
        return -2; // Fallo específico de formato
    } else if (error) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: No se pudo leer el archivo de la fuente '%s'. Código de error: %d\n", fontPath, error);
        perror("Detalle del error del sistema para FT_New_Face");
        // No es necesario llamar a FT_Done_FreeType aquí.
        return -1; // Fallo general
    }
    return 0; // Éxito
}

void cleanupFreeType() {
    // Liberar FreeType (igual que antes)
    if (ftFace) { FT_Done_Face(ftFace); ftFace = NULL; }
    if (ftLibrary) { FT_Done_FreeType(ftLibrary); ftLibrary = NULL; }

    printf("Limpieza completa.\n");
}

// --- Font Property Getters ---

const char* getFontFamilyName() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftFace no está cargado. No se puede obtener el nombre de la familia.\n");
        return NULL;
    }
    return ftFace->family_name;
}

const char* getFontStyleName() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftFace no está cargado. No se puede obtener el nombre del estilo.\n");
        return NULL;
    }
    return ftFace->style_name;
}

long getFontNumGlyphs() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftFace no está cargado. No se puede obtener el número de glifos.\n");
        return -1; // Retornar -1 para indicar error o fuente no cargada
    }
    return ftFace->num_glyphs;
}

// --- Callbacks de FreeType (Usando las estructuras C) ---