//Definiciones de Point2D, ContourC. Prototipos para initContour, 
// addPointToContour, freeContour, etc.
#ifndef VECTOR_UTILS_H
#define VECTOR_UTILS_H

#include <stddef.h>

typedef struct {
    float x, y;
} Point2D;

// Un contorno dinámico
typedef struct {
    Point2D* points;
    size_t count;    // Número actual de puntos
    size_t capacity; // Capacidad actual del array
} ContourC;

int initContour(ContourC* contour, size_t initialCapacity);
int addPointToContour(ContourC* contour, Point2D point);
void freeContour();

#endif