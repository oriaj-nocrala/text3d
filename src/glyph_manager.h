#ifndef GLYPH_MANAGER_H
#define GLYPH_MANAGER_H

// Incluir FreeType aquí para que FT_ULong y FT_Face sean conocidos
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H // Necesario para FT_Outline_Funcs si se usa en este header, o en el .c

// Incluir GLEW para tipos de OpenGL
#include <GL/glew.h>

#define HASH_TABLE_SIZE 256 // Puedes ajustar este tamaño según sea necesario

// Definición de GlyphInfo: almacena manejadores de OpenGL y métricas para un glifo
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;
    float advanceX;
    // Puedes añadir más métricas aquí si las necesitas:
    // float advanceY;
    // float width;
    // float height;
    // float bearingX;
    // float bearingY;
} GlyphInfo;

// Nodo para la tabla hash (lista enlazada para resolver colisiones)
typedef struct GlyphCacheNode {
    FT_ULong char_code;          // El punto de código Unicode
    GlyphInfo glyph_info;        // Los datos del glifo renderizado
    struct GlyphCacheNode* next; // Puntero al siguiente nodo en caso de colisión
} GlyphCacheNode;

// La tabla hash en sí (un array de punteros a GlyphCacheNode)
// Se definirá en glyph_manager.c y se accederá a través de funciones.
extern GlyphCacheNode* glyphHashTable[HASH_TABLE_SIZE];

// Declaraciones externas para las caras de FreeType (definidas en freetype_handler.c)
// freetype_handler.h debería ser incluido por freetype_handler.c
// y glyph_manager.c incluirá freetype_handler.h si necesita estas definiciones directamente,
// o confiará en que estén vinculadas. Por ahora, asumimos que están disponibles.
// extern FT_Face ftFace;
// extern FT_Face ftEmojiFace; // Opcional: para fallback de emojis

// Prototipos de Funciones Públicas
int initGlyphCache(); // Devuelve 1 para éxito, 0 para fallo
GlyphInfo getGlyphInfo(FT_ULong char_code);
void cleanupGlyphCache();

// La función para generar los datos del glifo es interna a glyph_manager.c
// No es necesario declararla aquí a menos que otros módulos la necesiten.

#endif // GLYPH_MANAGER_H
