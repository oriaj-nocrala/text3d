#include "tessellation_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

TessellationResult generateGlyphTessellation(OutlineDataC* outlineData) {
    
    TESStesselator* tess = NULL;
    TessellationResult result = { NULL, NULL, 0, 0, 1 };
    tess = tessNewTess(NULL);
    
    if (!tess) { 
        fprintf(stderr, "Error: tessNewTess falló (posiblemente memoria insuficiente).\n");
        return result;
    }
    // ... (Añadir contornos con tessAddContour) ...
     for (size_t i = 0; i < outlineData->count; ++i) {
         if (outlineData->contours[i].count >= 3) {
             tessAddContour(tess, 2, outlineData->contours[i].points, sizeof(Point2D), (int)outlineData->contours[i].count);
         }
     }

    // NO pasamos &tessResult, la API no usa callbacks
    if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL)) {
        fprintf(stderr, "tessTesselate falló.\n");
        // if (tess->outOfMemory) {
        //     fprintf(stderr, " (Causa: Falta de memoria en libtess2)\n");
        // }
        tessDeleteTess(tess);
        return result;
    }

    // --- Recuperar Resultados ---
    const TESSreal* vertices = tessGetVertices(tess);
    const TESSindex* elements = tessGetElements(tess);
    const int numVertices = tessGetVertexCount(tess);
    const int numElements = tessGetElementCount(tess);

    // Si no hay salida, no es necesariamente un error, pero no hay nada que copiar.
    if (numVertices == 0 || numElements == 0) {
        tessDeleteTess(tess);
        result.allocationFailed = 0; // No falló la asignación, simplemente no hubo salida.
        return result;
    }

    const int numIndices = numElements * 3; // Asumiendo TESS_POLYGONS con polySize=3

    // --- Asignar memoria y copiar resultados ---
    // Tamaño para vértices: numVertices * componentes_por_vertice (2) * tamaño_de_componente
    size_t verticesSize = numVertices * 2 * sizeof(TESSreal);
    result.vertices = (TESSreal*)malloc(verticesSize);

    // Tamaño para elementos: numIndices * tamaño_de_indice
    size_t elementsSize = numIndices * sizeof(TESSindex);
    result.elements = (TESSindex*)malloc(elementsSize);

    if (!result.vertices || !result.elements) {
        fprintf(stderr, "Error: Falló malloc al copiar resultados de teselación.\n");
        free(result.vertices); // Libera lo que se haya podido asignar
        free(result.elements);
        result.vertices = NULL;
        result.elements = NULL;
        tessDeleteTess(tess);
        return result; // Devuelve fallo (allocationFailed ya es 1)
    }

    // Copiar los datos usando memcpy
    memcpy(result.vertices, vertices, verticesSize);
    memcpy(result.elements, elements, elementsSize);

    // Poblar el resto de la estructura de resultado
    result.vertexCount = numVertices;
    result.elementCount = numElements; // Mantenemos el número de *triángulos*
    result.allocationFailed = 0; // ¡Éxito!

    // Liberar el teselador (esto invalida internalVertices e internalElements)
    tessDeleteTess(tess);

    return result;
}