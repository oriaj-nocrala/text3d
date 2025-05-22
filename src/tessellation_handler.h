#ifndef TESSELLATION_HANDLER_H
#define TESSELLATION_HANDLER_H

#include "tesselator.h"   // Para TESSreal, TESSindex
#include <stddef.h>       // Para size_t
// Forward declare OutlineDataC if freetype_handler.h isn't included,
// or include freetype_handler.h if it's safe from circular dependencies.
// For simplicity, assuming freetype_handler.h can be included or OutlineDataC is defined elsewhere.
// #include "freetype_handler.h" // Might be needed if OutlineDataC is complex
struct OutlineDataC; // Forward declaration


// Definiciones de estructuras movidas/añadidas aquí
typedef struct {
    float x, y;
} Point2D;

typedef struct {
    Point2D* points;
    size_t count;
    size_t capacity;
} ContourC;

// Estructura para devolver los resultados de la teselación
typedef struct {
    TESSreal* vertices;     // Puntero al array de vértices (x, y)
    TESSindex* elements;    // Puntero al array de índices de triángulos
    int vertexCount;
    int elementCount;       // Número de polígonos (triángulos en este caso)
    int allocationFailed; // Flag para indicar si falló la memoria interna
} TessellationResult;

// Función principal para teselar los contornos de un glifo
// Devuelve una estructura TessellationResult. El llamador es responsable
// de liberar vertices y elements si no son NULL.
// MODIFIED: Takes OutlineDataC* now
TessellationResult generateGlyphTessellation(struct OutlineDataC* outlineData);


#endif // TESSELLATION_HANDLER_H