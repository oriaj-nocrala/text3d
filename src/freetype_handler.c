#include "freetype_handler.h"
// #include "glyph_manager.h" // Ya no es necesario que freetype_handler conozca glyph_manager directamente para el caché.
#include <stdio.h>
#include <stdlib.h> // Para malloc, realloc, free
#include <string.h> // Para memset si se usa (aunque initOutlineData lo maneja)

// Definición de las variables globales de FreeType
FT_Library ftLibrary = NULL;
FT_Face ftFace = NULL;
FT_Face ftEmojiFace = NULL; // Inicializar a NULL

int initFreeType() {
    FT_Error error = FT_Init_FreeType(&ftLibrary);
    if (error) {
        fprintf(stderr, "Error: No se pudo inicializar FreeType (código: %d)\n", error);
        return 0;
    }
    printf("FreeType inicializado correctamente.\n");
    return 1;
}

// Modificado para cargar fuente principal y opcionalmente fuente de emoji
int loadFonts(const char* mainFontPath, const char* emojiFontPath) {
    if (!ftLibrary) {
        fprintf(stderr, "Error: FreeType no inicializado antes de llamar a loadFonts.\n");
        return 0;
    }
    if (!mainFontPath) {
        fprintf(stderr, "Error: La ruta de la fuente principal no puede ser NULL.\n");
        return 0;
    }

    // Cargar la fuente principal
    FT_Error error = FT_New_Face(ftLibrary, mainFontPath, 0, &ftFace);
    if (error) {
        fprintf(stderr, "Error: No se pudo cargar la fuente principal desde '%s' (código: %d)\n", mainFontPath, error);
        perror("Detalle del error del sistema (fuente principal)");
        // No es necesario FT_Done_FreeType aquí, se hará en cleanupFreeType
        return 0;
    }
    printf("Fuente principal '%s' cargada correctamente.\n", mainFontPath);

    // Cargar la fuente de emoji si se proporciona una ruta
    if (emojiFontPath != NULL && strlen(emojiFontPath) > 0) {
        error = FT_New_Face(ftLibrary, emojiFontPath, 0, &ftEmojiFace);
        if (error) {
            fprintf(stderr, "Advertencia: No se pudo cargar la fuente de emoji desde '%s' (código: %d). Se continuará sin fallback de emojis.\n", emojiFontPath, error);
            perror("Detalle del error del sistema (fuente de emoji)");
            ftEmojiFace = NULL; // Asegurarse de que sea NULL si falla la carga
        } else {
            printf("Fuente de emoji '%s' cargada correctamente.\n", emojiFontPath);
        }
    } else {
        printf("No se proporcionó ruta para fuente de emoji, o la ruta estaba vacía. No se usará fallback de emojis.\n");
        ftEmojiFace = NULL;
    }

    return 1;
}

// Inicializa OutlineDataC
int initOutlineData(OutlineDataC* data, size_t initialCapacity) {
    if (!data) return 0;
    data->contours = (ContourC*)malloc(initialCapacity * sizeof(ContourC));
    if (!data->contours) {
        fprintf(stderr, "Error: Falló malloc para data->contours en initOutlineData.\n");
        return 0;
    }
    data->count = 0;
    data->capacity = initialCapacity;
    data->currentPoint = (Point2D){0.0f, 0.0f};
    data->subdivisionSteps = 10; // Calidad de las curvas, ajustable
    return 1;
}

// Añade un nuevo contorno (vacío inicialmente) a OutlineDataC
ContourC* addContourToOutline(OutlineDataC* data) {
    if (!data) return NULL;
    if (data->count >= data->capacity) {
        size_t newCapacity = (data->capacity == 0) ? 4 : data->capacity * 2; // Duplicar capacidad
        ContourC* newContours = (ContourC*)realloc(data->contours, newCapacity * sizeof(ContourC));
        if (!newContours) {
            fprintf(stderr, "Error: Falló realloc para data->contours en addContourToOutline.\n");
            return NULL; // Error de memoria
        }
        data->contours = newContours;
        data->capacity = newCapacity;
    }
    
    // Inicializa el nuevo contorno dentro del array
    // Capacidad inicial de 16 puntos por contorno, ajustable
    if (!initContour(&data->contours[data->count], 16)) { 
        fprintf(stderr, "Error: Falló initContour para nuevo contorno en addContourToOutline.\n");
        return NULL; // Error al inicializar el contorno
    }
    // Devuelve puntero al nuevo contorno y luego incrementa count
    return &data->contours[data->count++]; 
}

// Libera memoria de OutlineDataC
void freeOutlineData(OutlineDataC* data) {
    if (data && data->contours) {
        for (size_t i = 0; i < data->count; ++i) {
            freeContour(&data->contours[i]); // freeContour debe estar definida en vector_utils.c
        }
        free(data->contours);
        data->contours = NULL;
        data->count = 0;
        data->capacity = 0;
    }
}

/*
// La función decomposeGlyphOutline ya no es necesaria con el caché dinámico
// y causaba errores de enlace porque usaba el antiguo `glyphCache`.
void decomposeGlyphOutline() {
    // Esta función estaba diseñada para pre-llenar un caché estático (glyphCache).
    // Con el nuevo sistema de tabla hash dinámica en glyph_manager,
    // los glifos se generan y cachean bajo demanda.
    // Por lo tanto, esta función ya no es necesaria y se comenta/elimina.

    // printf("Generando caché de glifos (ASCII 32-126)...\n");
    // // Inicializar todo a 0 por si acaso
    // memset(glyphCache, 0, sizeof(glyphCache)); // glyphCache ya no existe
    // for (int c = 32; c < 127; ++c) { // Rango de caracteres ASCII imprimibles
    //     glyphCache[c] = getGlyphInfo((char)c); // getGlyphInfo ahora toma FT_ULong y usa la tabla hash
    //     if (glyphCache[c].vao == 0 && c != ' ') { 
    //          fprintf(stderr, "Advertencia: No se pudo generar geometría para el carácter '%c' (ASCII %d)\n", (char)c, c);
    //     }
    // }
    // printf("Caché de glifos generado.\n");
    printf("Nota: decomposeGlyphOutline() ya no está en uso debido al caché dinámico.\n");
}
*/

void cleanupFreeType() {
    if (ftEmojiFace) {
        FT_Done_Face(ftEmojiFace);
        ftEmojiFace = NULL;
        printf("Fuente de emoji liberada.\n");
    }
    if (ftFace) {
        FT_Done_Face(ftFace);
        ftFace = NULL;
        printf("Fuente principal liberada.\n");
    }
    if (ftLibrary) {
        FT_Done_FreeType(ftLibrary);
        ftLibrary = NULL;
        printf("Librería FreeType liberada.\n");
    }
    printf("Limpieza de FreeType completa.\n");
}

// --- Callbacks de FreeType (Usando las estructuras C) ---

Point2D ftVecToPoint2D(const FT_Vector* vec) {
    return (Point2D){
        (float)(vec->x) / 64.0f, // Las unidades de FreeType son 1/64 de píxel
        (float)(vec->y) / 64.0f
    };
}

int ftMoveToFunc(const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (!data) return 1; // Error, no hay datos de usuario

    // Iniciar un nuevo contorno
    ContourC* newContour = addContourToOutline(data);
    if (!newContour) return 1; // Error al añadir/inicializar contorno

    data->currentPoint = ftVecToPoint2D(to);
    if (!addPointToContour(newContour, data->currentPoint)) return 1; // Error al añadir punto
    return 0; // Éxito
}

int ftLineToFunc(const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (!data || data->count == 0) return 1; // No hay datos o no hay contorno activo

    data->currentPoint = ftVecToPoint2D(to);
    // Añadir al último contorno añadido (el actual)
    if (!addPointToContour(&data->contours[data->count - 1], data->currentPoint)) return 1; // Error
    return 0; // Éxito
}

// Evalúa un punto en una curva de Bézier cuadrática
Point2D evaluateQuadraticBezierC(float t, Point2D p0, Point2D p1, Point2D p2) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float ut2 = 2.0f * u * t; // 2*u*t
    return (Point2D){
        uu * p0.x + ut2 * p1.x + tt * p2.x, // (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
        uu * p0.y + ut2 * p1.y + tt * p2.y
    };
}

int ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (!data || data->count == 0) return 1; // No hay contorno activo

    Point2D ctrlPt = ftVecToPoint2D(control);
    Point2D endPt = ftVecToPoint2D(to);
    Point2D startPt = data->currentPoint; // El punto actual antes de esta curva
    ContourC* currentContour = &data->contours[data->count - 1];

    // Subdividir la curva cuadrática en segmentos de línea
    for (int i = 1; i <= data->subdivisionSteps; ++i) {
        float t = (float)i / (float)data->subdivisionSteps;
        Point2D pointOnCurve = evaluateQuadraticBezierC(t, startPt, ctrlPt, endPt);
        if (!addPointToContour(currentContour, pointOnCurve)) return 1; // Error
    }

    data->currentPoint = endPt; // Actualizar el punto actual
    return 0; // Éxito
}

// Evalúa un punto en una curva de Bézier cúbica
Point2D evaluateCubicBezierC(float t, Point2D p0, Point2D p1, Point2D p2, Point2D p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Point2D p = {0,0};
    p.x = uuu * p0.x; // (1-t)^3 * P0
    p.y = uuu * p0.y;

    p.x += 3 * uu * t * p1.x; // 3 * (1-t)^2 * t * P1
    p.y += 3 * uu * t * p1.y;

    p.x += 3 * u * tt * p2.x; // 3 * (1-t) * t^2 * P2
    p.y += 3 * u * tt * p2.y;

    p.x += ttt * p3.x; // t^3 * P3
    p.y += ttt * p3.y;

    return p;
}

int ftCubicToFunc(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (!data || data->count == 0) return 1; // No hay contorno activo

    Point2D ctrlPt1 = ftVecToPoint2D(c1);
    Point2D ctrlPt2 = ftVecToPoint2D(c2);
    Point2D endPt = ftVecToPoint2D(to);
    Point2D startPt = data->currentPoint; // El punto actual antes de esta curva
    ContourC* currentContour = &data->contours[data->count - 1];

    // Subdividir la curva cúbica en segmentos de línea
    for (int i = 1; i <= data->subdivisionSteps; ++i) {
        float t = (float)i / (float)data->subdivisionSteps;
        Point2D pointOnCurve = evaluateCubicBezierC(t, startPt, ctrlPt1, ctrlPt2, endPt);
        if (!addPointToContour(currentContour, pointOnCurve)) return 1; // Error
    }
    data->currentPoint = endPt; // Actualizar el punto actual
    return 0; // Éxito
}
