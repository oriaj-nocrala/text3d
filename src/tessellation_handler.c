#include "tessellation_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

TessellationResult generateGlyphTessellation(ContourC* contours, size_t contourCount) {
    
    TESStesselator* tess = NULL;
    TessellationResult result = { NULL, NULL, 0, 0, 1 }; // allocationFailed = 1 (true) initially
    
    tess = tessNewTess(NULL); // Initialize tessellator
    if (!tess) { 
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: tessNewTess failed (possibly out of memory).\n");
        // result.allocationFailed is already 1, so just return
        return result;
    }

    // Add contours to the tessellator
    for (size_t i = 0; i < contourCount; ++i) {
        if (contours[i].points && contours[i].count >= 3) { // Check for NULL points
            tessAddContour(tess, 2, contours[i].points, sizeof(Point2D), (int)contours[i].count);
        }
    }

    // Tesselate the contours
    if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL)) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: tessTesselate failed.\n");
        // Check if out of memory flag is set by libtess2, if available and meaningful
        // if (tessGetStatus(tess) == TESS_ERROR_OUT_OF_MEMORY) { // Hypothetical function
        //     fprintf(stderr, " (Cause: libtess2 out of memory)\n");
        // }
        tessDeleteTess(tess);
        // result.allocationFailed is already 1
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
        result.allocationFailed = 0; // No allocation *attempt* failed for result.vertices/elements yet.
                                     // But no geometry was produced.
        return result;
    }

    const int numIndices = numElements * 3; // Asumiendo TESS_POLYGONS con polySize=3 (triangles)

    // --- Asignar memoria y copiar resultados ---
    size_t verticesSize = numVertices * 2 * sizeof(TESSreal); // 2 components (x, y) per vertex
    result.vertices = (TESSreal*)malloc(verticesSize);
    if (!result.vertices) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: malloc failed for vertices.\n");
        tessDeleteTess(tess);
        // result.allocationFailed is already 1
        return result;
    }

    size_t elementsSize = numIndices * sizeof(TESSindex);
    result.elements = (TESSindex*)malloc(elementsSize);
    if (!result.elements) {
        fprintf(stderr, "ERROR::TESSELLATION_HANDLER: malloc failed for elements.\n");
        free(result.vertices); // Free already allocated vertices
        result.vertices = NULL;
        tessDeleteTess(tess);
        // result.allocationFailed is already 1
        return result;
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