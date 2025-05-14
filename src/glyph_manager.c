#include "glyph_manager.h"
#include "freetype_handler.h"       // Para acceder a ftFace (declarada extern)
#include "tessellation_handler.h"
#include "vector_utils.h"           // Para Point2D, etc., usados en createGlyphGeometry
#include "tesselator.h"             // Para libtess2, usado en createGlyphGeometry
#include <stdio.h>                  // Para printf, fprintf
#include <stdlib.h>                 // Para malloc, etc. en createGlyphGeometry
#include <string.h>                 // Para memset
#include <GL/glew.h>                // Para tipos GLuint, etc. en createGlyphGeometry y GlyphInfo

GlyphInfo glyphCache[CACHE_SIZE]; // Declaración extern

// --- Función de Creación de Geometría (Refactorizada para libtess2 sin callbacks) ---
// Firma modificada: Devuelve GlyphInfo (o podrías pasar un puntero para rellenar)
GlyphInfo createGlyphGeometry(char character)
{
    GlyphInfo result = {0}; // Inicializar a 0 (vao=0 indica inválido/error)
    FT_Error ftError;
    int tessSuccess = 1;

    // 1. Cargar Glifo (NECESARIO cargar ANTES de descomponer para obtener métricas)
    ftError = FT_Set_Pixel_Sizes(ftFace, 0, 48); // Mismo tamaño para todos los cacheados
    if (ftError) { /* ... */ return result; }
    FT_UInt glyph_index = FT_Get_Char_Index(ftFace, character);
    if (glyph_index == 0) { /* ... */ return result; }
    // ¡IMPORTANTE! No usar FT_LOAD_NO_SCALE ahora si queremos métricas escaladas
    ftError = FT_Load_Glyph(ftFace, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP );
    if (ftError) { /* ... */ return result; }

    // --- Obtener Métrica de Avance ---
    // Las métricas están en unidades 26.6, dividir por 64.0
    result.advanceX = (float)(ftFace->glyph->advance.x) / 64.0f;
    // printf("AdvanceX para '%c': %.2f (raw: %ld)\n", character, result.advanceX, face->glyph->advance.x);


    // Comprobar si tiene contorno antes de intentar descomponer
    if (ftFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        fprintf(stderr, "Glifo '%c' no tiene formato outline.\n", character);
        // Aún es válido si tiene avance (ej: espacio), pero sin geometría
        result.vao = 0; // Asegurarse que no hay VAO
        return result;
    }

    // 2. Extraer contornos (igual que antes)
    OutlineDataC outlineData;
    if (!initOutlineData(&outlineData, 4)) { /* ... */ return result; }
    // ... (resto de la lógica de descomposición FT_Outline_Decompose) ...
    FT_Outline_Funcs ftFuncs;
    ftFuncs.move_to = ftMoveToFunc;
    ftFuncs.line_to = ftLineToFunc;
    ftFuncs.conic_to = ftConicToFunc;
    ftFuncs.cubic_to = ftCubicToFunc;
    ftFuncs.shift = 0;
    ftFuncs.delta = 0;
    ftError = FT_Outline_Decompose(&ftFace->glyph->outline, &ftFuncs, &outlineData);
    if (ftError) { freeOutlineData(&outlineData); /* ... */ return result; }
    int hasValidContours = 0; // ... (verificar contornos válidos) ...
    for(size_t i = 0; i < outlineData.count; ++i) { if (outlineData.contours[i].count >= 3) { hasValidContours = 1; break; } }
    if (!hasValidContours) { freeOutlineData(&outlineData); /* ... */ return result; }
    
    TessellationResult tessResult = generateGlyphTessellation(&outlineData);
    const int numIndices = tessResult.elementCount * 3;

    // 4. Crear Buffers OpenGL (VBO y EBO)
    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, tessResult.vertexCount * 2 * sizeof(TESSreal), tessResult.vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(TESSindex), tessResult.elements, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(TESSreal), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Guardar resultados en la estructura local
    result.vao = vao;
    result.vbo = vbo;
    result.ebo = ebo;
    result.indexCount = tessResult.elementCount * 3;
    // result.advanceX ya fue calculado

    return result; // Devolver la estructura con toda la información
}

// --- Nueva función para inicializar/llenar el caché ---
int initGlyphCache() {
    // Asegúrate de que FreeType (y ftFace) se inicializó antes de llamar a esto
    if (!ftFace) {
        fprintf(stderr, "Error: Se intentó inicializar el caché de glifos antes de cargar la fuente (ftFace).\n");
        return 0; // Indicar fallo
    }

    printf("Generando caché de glifos (ASCII 32-126)...\n");

    // Inicializar todo a 0 por si acaso
    memset(glyphCache, 0, sizeof(glyphCache));

    // Bucle para generar geometría para caracteres ASCII imprimibles
    for (int c = 32; c < 127; ++c) {
        // Llama a la función createGlyphGeometry (definida arriba)
        glyphCache[c] = createGlyphGeometry((char)c);

        // Comprobación de errores (igual que antes)
        if (glyphCache[c].vao == 0 && c != ' ') { // Permitir espacio sin geometría
             fprintf(stderr, "Advertencia: No se pudo generar geometría para el carácter '%c' (ASCII %d)\n", (char)c, c);
             // Podrías decidir si esto es un error fatal o solo una advertencia
        }
    }

    printf("Caché de glifos generado.\n");
    return 1; // Indicar éxito
}

// --- Función para obtener información del caché ---
GlyphInfo getGlyphInfo(char c) {
    // Comprobar límites básicos
    if (c < 0 || c >= CACHE_SIZE) {
        // Devolver un GlyphInfo vacío o un carácter por defecto (ej: '?')
        static GlyphInfo invalidGlyph = {0}; // Inicializa a 0 la primera vez
        fprintf(stderr, "Advertencia: Caracter '%c' fuera de rango del caché.\n", c);
        // Podrías intentar obtener el glifo para '?' si existe
        if ('?' >= 0 && '?' < CACHE_SIZE) return glyphCache['?'];
        return invalidGlyph;
    }
    return glyphCache[(int)c];
}

// --- Función para limpiar el caché ---
void cleanupGlyphCache() {
     printf("Limpiando caché de glifos...\n");
    for (int i = 0; i < CACHE_SIZE; ++i) {
        if (glyphCache[i].vao != 0) {
            glDeleteVertexArrays(1, &glyphCache[i].vao);
            glDeleteBuffers(1, &glyphCache[i].vbo);
            glDeleteBuffers(1, &glyphCache[i].ebo);
            glyphCache[i].vao = 0;
        }
    }
    printf("Caché de glifos limpiado.\n");
}