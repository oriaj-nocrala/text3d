#ifndef FREETYPE_HANDLER_H
#define FREETYPE_HANDLER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "vector_utils.h" // Para ContourC, Point2D
#include <stddef.h>     // Para size_t

// Datos para los callbacks de FreeType
typedef struct {
    ContourC* contours;
    size_t count;
    size_t capacity;
    Point2D currentPoint;
    int subdivisionSteps; // Para la calidad de las curvas
} OutlineDataC;

// Variables globales para la librería y las caras de FreeType
// Definidas en freetype_handler.c
extern FT_Library ftLibrary;
extern FT_Face ftFace;        // Cara principal para texto
extern FT_Face ftEmojiFace;   // Cara para fallback de emojis (opcional)

// Prototipos de funciones
int initFreeType();
// Modificado para aceptar opcionalmente una ruta de fuente de emoji
int loadFonts(const char* mainFontPath, const char* emojiFontPath); 
int initOutlineData(OutlineDataC* data, size_t initialCapacity);
void freeOutlineData(OutlineDataC* data);
// decomposeGlyphOutline ya no es necesaria con el caché dinámico
// void decomposeGlyphOutline(); 
void cleanupFreeType();

// Funciones de callback (usadas por FT_Outline_Decompose)
Point2D ftVecToPoint2D(const FT_Vector* vec);
int ftMoveToFunc(const FT_Vector* to, void* userData);
int ftLineToFunc(const FT_Vector* to, void* userData);
Point2D evaluateQuadraticBezierC(float t, Point2D p0, Point2D p1, Point2D p2);
int ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData);
int ftCubicToFunc(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user);

#endif // FREETYPE_HANDLER_H