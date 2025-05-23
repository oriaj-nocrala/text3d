#ifndef GLYPH_MANAGER_H
#define GLYPH_MANAGER_H

#include <GL/glew.h> // Para GLuint, GLsizei
#include <ft2build.h> // For FT_ULong
#include FT_FREETYPE_H

#define HASH_TABLE_SIZE 256 // Size of the hash table, can be adjusted

typedef struct {
    GLuint vao;         // No se usa para SDF puro si globalQuadVAO se usa para todos
    GLuint vbo;         // No se usa para SDF puro
    GLuint ebo;         // No se usa para SDF puro
    GLsizei indexCount; // No se usa para SDF puro

    float advanceX;         // Avance horizontal en píxeles (unidades FT / 64.0f)
    int bitmap_left;        // Desplazamiento X desde el origen del pen al borde izq. del bitmap (píxeles)
    int bitmap_top;         // Desplazamiento Y desde la línea base al borde sup. del bitmap (píxeles)

    // SDF Texture data
    GLuint sdfTextureID;
    int sdfTextureWidth;    // Ancho de la textura SDF (con padding, en píxeles)
    int sdfTextureHeight;   // Alto de la textura SDF (con padding, en píxeles)
} GlyphInfo;

// Node for the hash table (linked list for collision resolution)
typedef struct GlyphCacheNode {
    FT_ULong char_code;          // Unicode codepoint
    GlyphInfo glyph_info;        // Rendered glyph data
    struct GlyphCacheNode* next; // Pointer to the next node in case of collision
} GlyphCacheNode;

// The hash table itself (array of pointers to GlyphCacheNode)
// Defined in glyph_manager.c
extern GlyphCacheNode* glyphHashTable[HASH_TABLE_SIZE];

int initGlyphCache(); // Returns 0 for success, non-zero for failure
GlyphInfo getGlyphInfo(FT_ULong char_code); // Takes Unicode codepoint
void cleanupGlyphCache();

#endif // GLYPH_MANAGER_H