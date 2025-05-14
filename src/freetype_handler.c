#include "freetype_handler.h"
#include "glyph_manager.h"
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

FT_Library ftLibrary;
FT_Face ftFace;

int initFreeType() {
    // Inicializar FreeType y cargar la fuente
    FT_Error error = FT_Init_FreeType( &ftLibrary );
    if ( error ) {
        fprintf(stderr, "No se pudo inicializar FreeType\n");
        return 0;
    }
    return 1;
}

int loadFont() {
    FT_Error error = FT_New_Face( ftLibrary,
        // Asegúrate que esta ruta es correcta en tu sistema
        "/usr/share/fonts/TTF/DejaVuSans.ttf", // Ruta alternativa
        0,
        &ftFace );
    if ( error == FT_Err_Unknown_File_Format ) {
        fprintf(stderr, "Formato de la fuente no soportado\n");
        FT_Done_FreeType(ftLibrary);
        return 0;
    } else if ( error ) {
        fprintf(stderr, "No se pudo leer el archivo de la fuente. Error: %d\n", error);
        perror("Detalle del error del sistema"); // Muestra error de bajo nivel si aplica
        FT_Done_FreeType(ftLibrary);
        return 0;
    }
    return 1;
}

// Inicializa OutlineDataC
int initOutlineData(OutlineDataC* data, size_t initialCapacity) {
    data->contours = (ContourC*)malloc(initialCapacity * sizeof(ContourC));
     if (!data->contours) return 0;
    data->count = 0;
    data->capacity = initialCapacity;
    data->currentPoint = (Point2D){0.0f, 0.0f};
    data->subdivisionSteps = 10; // Ajustable
    return 1;
}

// Añade un nuevo contorno (vacío inicialmente)
ContourC* addContourToOutline(OutlineDataC* data) {
     if (data->count >= data->capacity) {
        size_t newCapacity = (data->capacity == 0) ? 4 : data->capacity * 2;
        ContourC* newContours = (ContourC*)realloc(data->contours, newCapacity * sizeof(ContourC));
        if (!newContours) return NULL; // Error
        data->contours = newContours;
        data->capacity = newCapacity;
    }
    // Inicializa el nuevo contorno dentro del array
    if (!initContour(&data->contours[data->count], 16)) { // Capacidad inicial de 16 puntos
         return NULL; // Error al inicializar el contorno
    }
    return &data->contours[data->count++]; // Devuelve puntero al nuevo contorno y incrementa count
}


// Libera memoria de OutlineDataC
void freeOutlineData(OutlineDataC* data) {
    if (data) {
        for (size_t i = 0; i < data->count; ++i) {
            freeContour(&data->contours[i]);
        }
        free(data->contours);
        data->contours = NULL;
        data->count = 0;
        data->capacity = 0;
    }
}


void decomposeGlyphOutline() {
    printf("Generando caché de glifos (ASCII 32-126)...\n");
    // Inicializar todo a 0 por si acaso
    memset(glyphCache, 0, sizeof(glyphCache));
    for (int c = 32; c < 127; ++c) { // Rango de caracteres ASCII imprimibles
        glyphCache[c] = getGlyphInfo((char)c);
        // Podrías añadir comprobación de errores aquí si createGlyphGeometry devuelve NULL o vao=0
        if (glyphCache[c].vao == 0 && c != ' ') { // Permitir espacio sin geometría
             fprintf(stderr, "Advertencia: No se pudo generar geometría para el carácter '%c' (ASCII %d)\n", (char)c, c);
        }
    }
    printf("Caché de glifos generado.\n");
}

void cleanupFreeType() {
    // Liberar FreeType (igual que antes)
    if (ftFace) { FT_Done_Face(ftFace); ftFace = NULL; }
    if (ftLibrary) { FT_Done_FreeType(ftLibrary); ftLibrary = NULL; }

    printf("Limpieza completa.\n");
}

// --- Callbacks de FreeType (Usando las estructuras C) ---

Point2D ftVecToPoint2D(const FT_Vector* vec) {
    return (Point2D){
        (float)(vec->x) / 64.0f,
        (float)(vec->y) / 64.0f
    };
}

int ftMoveToFunc(const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    ContourC* newContour = addContourToOutline(data); // Añade e inicializa un nuevo contorno
    if (!newContour) return 1; // Error

    data->currentPoint = ftVecToPoint2D(to);
    if (!addPointToContour(newContour, data->currentPoint)) return 1; // Error
    return 0; // Éxito
}

int ftLineToFunc(const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (data->count == 0) return 1; // No hay contorno activo

    data->currentPoint = ftVecToPoint2D(to);
    // Añadir al último contorno añadido
    if (!addPointToContour(&data->contours[data->count - 1], data->currentPoint)) return 1; // Error
    return 0; // Éxito
}

Point2D evaluateQuadraticBezierC(float t, Point2D p0, Point2D p1, Point2D p2) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float ut2 = 2.0f * u * t;
    return (Point2D){
        uu * p0.x + ut2 * p1.x + tt * p2.x,
        uu * p0.y + ut2 * p1.y + tt * p2.y
    };
}

int ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (data->count == 0) return 1; // No hay contorno activo

    Point2D ctrlPt = ftVecToPoint2D(control);
    Point2D endPt = ftVecToPoint2D(to);
    Point2D startPt = data->currentPoint;
    ContourC* currentContour = &data->contours[data->count - 1];

    for (int i = 1; i <= data->subdivisionSteps; ++i) {
        float t = (float)i / (float)data->subdivisionSteps;
        Point2D pointOnCurve = evaluateQuadraticBezierC(t, startPt, ctrlPt, endPt);
        if (!addPointToContour(currentContour, pointOnCurve)) return 1; // Error
    }

    data->currentPoint = endPt;
    return 0; // Éxito
}

// ftCubicToFunc omitido por brevedad (análogo a ftConicToFunc con fórmula cúbica)
int ftCubicToFunc(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user) {
    // Implementación similar a conic, usando la fórmula cúbica.
    // Por simplicidad, solo avanzamos.
     OutlineDataC* data = (OutlineDataC*)user;
    if (data->count == 0) return 1;
     data->currentPoint = ftVecToPoint2D(to);
     if (!addPointToContour(&data->contours[data->count - 1], data->currentPoint)) return 1;
     fprintf(stderr, "Advertencia: ftCubicToFunc no implementado para subdivision completa.\n");
     return 0;
}