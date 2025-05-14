#ifndef FREETYPE_HANDLER_H
#define FREETYPE_HANDLER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "vector_utils.h"
#include <stddef.h>

// Datos para los callbacks de FreeType
typedef struct {
    ContourC* contours;
    size_t count;
    size_t capacity;
    Point2D currentPoint;
    int subdivisionSteps;
} OutlineDataC;

int initFreeType();
int loadFont();
int initOutlineData(OutlineDataC* data, size_t initialCapacity);
void freeOutlineData(OutlineDataC* data);
void decomposeGlyphOutline();
void cleanupFreeType();

extern FT_Library ftLibrary; // Renombrado de 'library' para evitar posible conflicto
extern FT_Face ftFace;       // Renombrado de 'face'

Point2D ftVecToPoint2D(const FT_Vector* vec);
int ftMoveToFunc(const FT_Vector* to, void* userData);
int ftLineToFunc(const FT_Vector* to, void* userData);
Point2D evaluateQuadraticBezierC(float t, Point2D p0, Point2D p1, Point2D p2);
int ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData);
int ftCubicToFunc(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user);

#endif