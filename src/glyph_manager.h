#ifndef GLYPH_MANAGER_H
#define GLYPH_MANAGER_H

#include <GL/glew.h> // Para GLuint, GLsizei

#define CACHE_SIZE 128

// ¡NUEVO! Estructura para almacenar la geometría y métricas de un glifo cacheado
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;
    float advanceX;   // Avance horizontal en unidades (después de dividir / 64.0)
    // Podrías añadir más métricas si las necesitas (ej: tamaño, bearing)
} GlyphInfo;

extern GlyphInfo glyphCache[CACHE_SIZE]; // Declaración extern

// GlyphInfo createGlyphGeometry(FT_Face face, char character);
int initGlyphCache();
GlyphInfo getGlyphInfo(char c);
void cleanupGlyphCache();

#endif