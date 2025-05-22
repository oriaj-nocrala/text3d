#include "freetype_handler.h"
#include <stdio.h>
#include <stdlib.h> // Para malloc, realloc, free
#include <string.h> // Para strlen

FT_Library ftLibrary = NULL;
FT_Face ftFace = NULL;
FT_Face ftEmojiFace = NULL;

int initFreeType() {
    FT_Error error = FT_Init_FreeType(&ftLibrary);
    if (error) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: No se pudo inicializar FreeType. Código de error: %d\n", error);
        return -1; 
    }
    return 0; 
}

int loadFonts(const char* mainFontPath, const char* emojiFontPath) {
    if (!ftLibrary) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftLibrary no inicializada antes de llamar a loadFonts.\n");
        return -3; 
    }
    if (!mainFontPath) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: La ruta de la fuente principal no puede ser NULL.\n");
        return -4;
    }

    FT_Error error = FT_New_Face(ftLibrary, mainFontPath, 0, &ftFace);
    if (error) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: No se pudo cargar la fuente principal desde '%s'. Código de error: %d\n", mainFontPath, error);
        perror("Detalle del error del sistema (fuente principal)");
        return -1;
    }
    // printf("Fuente principal '%s' cargada.\n", mainFontPath); // Comentado para reducir verbosidad

    if (emojiFontPath != NULL && strlen(emojiFontPath) > 0) {
        error = FT_New_Face(ftLibrary, emojiFontPath, 0, &ftEmojiFace);
        if (error) {
            fprintf(stderr, "ADVERTENCIA::FREETYPE_HANDLER: No se pudo cargar la fuente de emoji desde '%s' (código: %d). Se continuará sin fallback de emojis.\n", emojiFontPath, error);
            perror("Detalle del error del sistema (fuente de emoji)");
            ftEmojiFace = NULL;
        } else {
            // printf("Fuente de emoji '%s' cargada.\n", emojiFontPath); // Comentado
        }
    } else {
        ftEmojiFace = NULL; 
    }
    return 0; 
}

void cleanupFreeType() {
    if (ftEmojiFace) { FT_Done_Face(ftEmojiFace); ftEmojiFace = NULL; }
    if (ftFace) { FT_Done_Face(ftFace); ftFace = NULL; }
    if (ftLibrary) { FT_Done_FreeType(ftLibrary); ftLibrary = NULL; }
}

const char* getFontFamilyName() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftFace no está cargado. No se puede obtener el nombre de la familia.\n");
        return NULL;
    }
    return ftFace->family_name;
}

const char* getFontStyleName() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftFace no está cargado. No se puede obtener el nombre del estilo.\n");
        return NULL;
    }
    return ftFace->style_name;
}

long getFontNumGlyphs() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER: ftFace no está cargado. No se puede obtener el número de glifos.\n");
        return -1; 
    }
    return ftFace->num_glyphs;
}

// --- ContourC Helpers (AHORA NO ESTÁTICOS) ---
int initContour(ContourC* contour, size_t initialCapacity) { // Se quitó 'static'
    contour->points = (Point2D*)malloc(initialCapacity * sizeof(Point2D));
    if (!contour->points) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER::INIT_CONTOUR: Malloc failed for points.\n");
        return -1;
    }
    contour->count = 0;
    contour->capacity = initialCapacity;
    return 0;
}

int addPointToContour(ContourC* contour, Point2D point) { // Se quitó 'static'
    if (contour->count >= contour->capacity) {
        size_t newCapacity = (contour->capacity == 0) ? 8 : contour->capacity * 2;
        Point2D* newPoints = (Point2D*)realloc(contour->points, newCapacity * sizeof(Point2D));
        if (!newPoints) {
            fprintf(stderr, "ERROR::FREETYPE_HANDLER::ADD_POINT_TO_CONTOUR: Realloc failed for points.\n");
            return -1;
        }
        contour->points = newPoints;
        contour->capacity = newCapacity;
    }
    contour->points[contour->count++] = point;
    return 0;
}

void freeContour(ContourC* contour) { // Se quitó 'static'
    if (contour && contour->points) {
        free(contour->points);
        contour->points = NULL;
        contour->count = 0;
        contour->capacity = 0;
    }
}

// --- OutlineDataC Helpers ---
int initOutlineData(OutlineDataC* data, size_t initialCapacity) {
    if (!data) return -1;
    data->contours = (ContourC*)malloc(initialCapacity * sizeof(ContourC));
    if (!data->contours) {
        fprintf(stderr, "ERROR::FREETYPE_HANDLER::INIT_OUTLINE_DATA: Malloc failed for contours.\n");
        return -1;
    }
    data->count = 0;
    data->capacity = initialCapacity;
    data->currentPoint = (Point2D){0.0f, 0.0f};
    data->subdivisionSteps = 10; 
    return 0;
}

// Esta función addContourToOutline sigue siendo static porque es una utilidad interna para los callbacks de FreeType
// que se definen en este mismo archivo. No necesita ser llamada desde fuera.
static ContourC* addContourToOutline(OutlineDataC* data) {
    if (!data) return NULL;
    if (data->count >= data->capacity) {
        size_t newCapacity = (data->capacity == 0) ? 4 : data->capacity * 2;
        ContourC* newContours = (ContourC*)realloc(data->contours, newCapacity * sizeof(ContourC));
        if (!newContours) {
            fprintf(stderr, "ERROR::FREETYPE_HANDLER::ADD_CONTOUR_TO_OUTLINE: Realloc failed.\n");
            return NULL;
        }
        data->contours = newContours;
        data->capacity = newCapacity;
    }
    // Usa la función initContour (ahora no estática)
    if (initContour(&data->contours[data->count], 16) != 0) { 
         fprintf(stderr, "ERROR::FREETYPE_HANDLER::ADD_CONTOUR_TO_OUTLINE: initContour failed.\n");
        return NULL;
    }
    return &data->contours[data->count++];
}

void freeOutlineData(OutlineDataC* data) {
    if (data && data->contours) {
        for (size_t i = 0; i < data->count; ++i) {
            // Usa la función freeContour (ahora no estática)
            freeContour(&data->contours[i]);
        }
        free(data->contours);
        data->contours = NULL;
        data->count = 0;
        data->capacity = 0;
    }
}

// --- FreeType Callbacks ---
static Point2D ftVecToPoint2D(const FT_Vector* vec) {
    return (Point2D){ (float)vec->x / 64.0f, (float)vec->y / 64.0f };
}

static Point2D evaluateQuadraticBezier(float t, Point2D p0, Point2D p1, Point2D p2) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float ut2 = 2.0f * u * t;
    return (Point2D){
        uu * p0.x + ut2 * p1.x + tt * p2.x,
        uu * p0.y + ut2 * p1.y + tt * p2.y
    };
}

static Point2D evaluateCubicBezier(float t, Point2D p0, Point2D p1, Point2D p2, Point2D p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    Point2D p;
    p.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    p.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;
    return p;
}

int ftMoveToFunc(const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    ContourC* newContour = addContourToOutline(data); // Esta sigue siendo static y está bien
    if (!newContour) return 1; 
    data->currentPoint = ftVecToPoint2D(to);
    // Usa la función addPointToContour (ahora no estática)
    if (addPointToContour(newContour, data->currentPoint) != 0) return 1; 
    return 0; 
}

int ftLineToFunc(const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (data->count == 0) return 1; 
    data->currentPoint = ftVecToPoint2D(to);
    // Usa la función addPointToContour (ahora no estática)
    if (addPointToContour(&data->contours[data->count - 1], data->currentPoint) != 0) return 1; 
    return 0; 
}

int ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (data->count == 0) return 1; 

    Point2D ctrlPt = ftVecToPoint2D(control);
    Point2D endPt = ftVecToPoint2D(to);
    Point2D startPt = data->currentPoint;
    ContourC* currentContour = &data->contours[data->count - 1];

    for (int i = 1; i <= data->subdivisionSteps; ++i) {
        float t = (float)i / (float)data->subdivisionSteps;
        Point2D pointOnCurve = evaluateQuadraticBezier(t, startPt, ctrlPt, endPt);
        // Usa la función addPointToContour (ahora no estática)
        if (addPointToContour(currentContour, pointOnCurve) != 0) return 1; 
    }
    data->currentPoint = endPt;
    return 0; 
}

int ftCubicToFunc(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* userData) {
    OutlineDataC* data = (OutlineDataC*)userData;
    if (data->count == 0) return 1; 

    Point2D ctrlPt1 = ftVecToPoint2D(c1);
    Point2D ctrlPt2 = ftVecToPoint2D(c2);
    Point2D endPt = ftVecToPoint2D(to);
    Point2D startPt = data->currentPoint;
    ContourC* currentContour = &data->contours[data->count - 1];

    for (int i = 1; i <= data->subdivisionSteps; ++i) {
        float t = (float)i / (float)data->subdivisionSteps;
        Point2D pointOnCurve = evaluateCubicBezier(t, startPt, ctrlPt1, ctrlPt2, endPt);
        // Usa la función addPointToContour (ahora no estática)
        if (addPointToContour(currentContour, pointOnCurve) != 0) return 1; 
    }
    data->currentPoint = endPt;
    return 0; 
}