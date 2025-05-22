#include "tessellation_handler.h"
#include "freetype_handler.h" // Para la definición de OutlineDataC
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

TessellationResult generateGlyphTessellation(OutlineDataC* outlineData) {
    TESStesselator* tess = NULL;
    // Inicializa allocationFailed a 0. Se pondrá a 1 si hay errores de alocación para result.vertices/elements.
    TessellationResult result = { NULL, NULL, 0, 0, 0 }; 

    if (!outlineData) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: outlineData es NULL.\n");
        result.allocationFailed = 1; // Error de entrada, se considera fallo de "alocación" en sentido amplio.
        return result;
    }

    tess = tessNewTess(NULL);
    if (!tess) { 
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: tessNewTess falló (posiblemente memoria insuficiente).\n");
        result.allocationFailed = 1; // Fallo al alocar el teselador.
        return result;
    }

    int contoursAdded = 0;
    for (size_t i = 0; i < outlineData->count; ++i) {
         if (outlineData->contours[i].points && outlineData->contours[i].count >= 3) {
             tessAddContour(tess, 2, outlineData->contours[i].points, sizeof(Point2D), (int)outlineData->contours[i].count);
             contoursAdded++;
         }
     }

    // Si no se añadieron contornos válidos, no hay nada que teselar.
    if (contoursAdded == 0) {
        tessDeleteTess(tess);
        // Devuelve un resultado vacío, allocationFailed permanece 0.
        // No se imprimirá "tessTesselate failed" porque no se llamará.
        return result; 
    }

    if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL)) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: tessTesselate falló (con %d contornos añadidos).\n", contoursAdded);
        tessDeleteTess(tess);
        // Si la teselación falla después de añadir contornos, no es un fallo de alocación para 'result',
        // pero no se puede continuar. Devolvemos un resultado vacío. allocationFailed permanece 0.
        return result;
    }

    // --- Recuperar Resultados ---
    const TESSreal* vertices = tessGetVertices(tess);
    const TESSindex* elements = tessGetElements(tess);
    const int numVertices = tessGetVertexCount(tess);
    const int numElements = tessGetElementCount(tess);

    if (numVertices == 0 || numElements == 0) {
        // La teselación tuvo éxito pero no produjo salida (ej. contornos degenerados que se eliminaron).
        tessDeleteTess(tess);
        // allocationFailed permanece 0.
        return result;
    }

    const int numIndices = numElements * 3;

    // --- Asignar memoria y copiar resultados ---
    size_t verticesSize = numVertices * 2 * sizeof(TESSreal);
    result.vertices = (TESSreal*)malloc(verticesSize);
    if (!result.vertices) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: malloc falló para vértices.\n");
        result.allocationFailed = 1; // Fallo de alocación para result.vertices
        // No es necesario liberar result.elements aquí porque aún no se ha alocado.
        tessDeleteTess(tess);
        return result;
    }

    size_t elementsSize = numIndices * sizeof(TESSindex);
    result.elements = (TESSindex*)malloc(elementsSize);
    if (!result.elements) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: malloc falló para elementos.\n");
        result.allocationFailed = 1; // Fallo de alocación para result.elements
        free(result.vertices); 
        result.vertices = NULL;
        tessDeleteTess(tess);
        return result;
    }

    memcpy(result.vertices, vertices, verticesSize);
    memcpy(result.elements, elements, elementsSize);

    result.vertexCount = numVertices;
    result.elementCount = numElements; 
    // result.allocationFailed ya es 0 si llegamos aquí con éxito.

    tessDeleteTess(tess);
    return result;
}