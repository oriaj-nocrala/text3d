#ifndef GLYPH_MANAGER_H
#define GLYPH_MANAGER_H

#include <GL/glew.h> // Para GLuint, GLsizei
#include <ft2build.h> // For FT_ULong
#include FT_FREETYPE_H

#define HASH_TABLE_SIZE 256 // Size of the hash table, can be adjusted

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;
    float advanceX;
    // Add other metrics if needed (bearingX, bearingY, glyphWidth, glyphHeight)
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