#include "vector_utils.h"
#include <stddef.h>
#include <stdlib.h>

// Inicializa un contorno
int initContour(ContourC* contour, size_t initialCapacity) {
    contour->points = (Point2D*)malloc(initialCapacity * sizeof(Point2D));
    if (!contour->points) return 0; // Error
    contour->count = 0;
    contour->capacity = initialCapacity;
    return 1; // Éxito
}

// Añade un punto a un contorno, redimensionando si es necesario
int addPointToContour(ContourC* contour, Point2D point) {
    if (contour->count >= contour->capacity) {
        size_t newCapacity = (contour->capacity == 0) ? 8 : contour->capacity * 2;
        Point2D* newPoints = (Point2D*)realloc(contour->points, newCapacity * sizeof(Point2D));
        if (!newPoints) return 0; // Error
        contour->points = newPoints;
        contour->capacity = newCapacity;
    }
    contour->points[contour->count++] = point;
    return 1; // Éxito
}

// Libera memoria de un contorno
void freeContour(ContourC* contour) {
    if (contour && contour->points) {
        free(contour->points);
        contour->points = NULL;
        contour->count = 0;
        contour->capacity = 0;
    }
}