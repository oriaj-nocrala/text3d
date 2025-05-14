#ifndef TESSELLATION_HANDLER_H
#define TESSELLATION_HANDLER_H

#include "vector_utils.h" // Para OutlineDataC
#include "tesselator.h"   // Para TESSreal, TESSindex
#include "freetype_handler.h"

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
TessellationResult generateGlyphTessellation(OutlineDataC* outlineData);

#endif // TESSELLATION_HANDLER_H