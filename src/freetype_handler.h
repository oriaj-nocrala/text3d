#ifndef FREETYPE_HANDLER_H
#define FREETYPE_HANDLER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

int initFreeType();
int loadFont(const char* fontPath);
void cleanupFreeType();

// Font property getters
const char* getFontFamilyName();
const char* getFontStyleName();
long getFontNumGlyphs();

extern FT_Library ftLibrary; // Renombrado de 'library' para evitar posible conflicto
extern FT_Face ftFace;       // Renombrado de 'face'

#endif