#ifndef FREETYPE_HANDLER_H
#define FREETYPE_HANDLER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "tessellation_handler.h" // Para Point2D, ContourC
#include <stddef.h>               // Para size_t

// Datos para los callbacks de FreeType
typedef struct OutlineDataC { 
    ContourC* contours;
    size_t count;
    size_t capacity;
    Point2D currentPoint;
    int subdivisionSteps; 
} OutlineDataC;

// Variables globales
extern FT_Library ftLibrary;
extern FT_Face ftFace;        
extern FT_Face ftEmojiFace;   

// Prototipos de funciones principales
int initFreeType();
int loadFonts(const char* mainFontPath, const char* emojiFontPath);
void cleanupFreeType();

// Getters de propiedades de fuente
const char* getFontFamilyName();
const char* getFontStyleName();
long getFontNumGlyphs();

// Helpers para OutlineDataC y ContourC
int initOutlineData(OutlineDataC* data, size_t initialCapacity);
void freeOutlineData(OutlineDataC* data);

// DECLARACIONES AÑADIDAS para helpers de ContourC:
int initContour(ContourC* contour, size_t initialCapacity);
int addPointToContour(ContourC* contour, Point2D point);
void freeContour(ContourC* contour);
// Fin de declaraciones añadidas

// Funciones de callback (usadas por FT_Outline_Decompose)
int ftMoveToFunc(const FT_Vector* to, void* userData);
int ftLineToFunc(const FT_Vector* to, void* userData);
int ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData);
int ftCubicToFunc(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* userData);

#endif // FREETYPE_HANDLER_H