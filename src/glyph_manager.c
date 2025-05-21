#include "glyph_manager.h"
#include "freetype_handler.h"       // Para acceder a ftFace (declarada extern)
#include "tessellation_handler.h"   // Para generateGlyphTessellation
// #include "vector_utils.h"        // Eliminado, definiciones movidas aquí
#include "tesselator.h"             // Para libtess2 tipos (TESSreal, TESSindex)
#include <ft2build.h>               // Necesario para FT_Error, FT_GlyphSlot, etc.
#include FT_FREETYPE_H             // Necesario para FT_Face, FT_GlyphSlot, etc.
#include FT_OUTLINE_H              // Necesario para FT_Outline_Decompose, FT_Outline_Funcs
#include <stdio.h>                  // Para printf, fprintf
#include <stdlib.h>                 // Para malloc, free, realloc
#include <string.h>                 // Para memset
#include <GL/glew.h>                // Para tipos GLuint, etc. en createGlyphGeometry y GlyphInfo

// Point2D and ContourC are now defined in tessellation_handler.h, which is included above.
// No need for local static definitions of these types here.

// --- Static helper functions for ContourC (now using types from tessellation_handler.h) ---
static int initContour(ContourC* contour, size_t initialCapacity) {
    contour->points = (Point2D*)malloc(initialCapacity * sizeof(Point2D));
    if (!contour->points) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::INIT_CONTOUR: Fallo al alocar memoria para puntos de contorno.\n");
        return -1; // Error
    }
    contour->count = 0;
    contour->capacity = initialCapacity;
    return 0; // Éxito
}

static int addPointToContour(ContourC* contour, Point2D point) {
    if (contour->count >= contour->capacity) {
        size_t newCapacity = (contour->capacity == 0) ? 8 : contour->capacity * 2;
        Point2D* newPoints = (Point2D*)realloc(contour->points, newCapacity * sizeof(Point2D));
        if (!newPoints) {
            fprintf(stderr, "ERROR::GLYPH_MANAGER::ADD_POINT_TO_CONTOUR: Fallo al realocar memoria para puntos de contorno.\n");
            return -1; // Error
        }
        contour->points = newPoints;
        contour->capacity = newCapacity;
    }
    contour->points[contour->count++] = point;
    return 0; // Éxito
}

static void freeContour(ContourC* contour) {
    if (contour && contour->points) {
        free(contour->points);
        contour->points = NULL;
        contour->count = 0;
        contour->capacity = 0;
    }
}

// --- Definición de GlyphOutlineData y helpers ---
typedef struct {
    ContourC* contours;
    size_t count;
    size_t capacity;
    Point2D currentPoint;
    int subdivisionSteps;
} GlyphOutlineData;

static int initGlyphOutlineData(GlyphOutlineData* data, size_t initialCapacity) {
    data->contours = (ContourC*)malloc(initialCapacity * sizeof(ContourC));
    if (!data->contours) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::INIT_GLYPH_OUTLINE_DATA: Fallo al alocar memoria para contornos.\n");
        return -1; // Error
    }
    data->count = 0;
    data->capacity = initialCapacity;
    data->currentPoint = (Point2D){0.0f, 0.0f};
    data->subdivisionSteps = 10; // Valor por defecto
    return 0; // Éxito
}

static ContourC* addContourToGlyphOutline(GlyphOutlineData* data) {
    if (data->count >= data->capacity) {
        size_t newCapacity = (data->capacity == 0) ? 4 : data->capacity * 2; // Capacidad inicial de 4 contornos
        ContourC* newContours = (ContourC*)realloc(data->contours, newCapacity * sizeof(ContourC));
        if (!newContours) {
            fprintf(stderr, "ERROR::GLYPH_MANAGER::ADD_CONTOUR_TO_GLYPH_OUTLINE: Fallo al realocar memoria para contornos.\n");
            return NULL; // Error
        }
        data->contours = newContours;
        data->capacity = newCapacity;
    }
    // Inicializa el nuevo contorno
    if (initContour(&data->contours[data->count], 16) != 0) { // Capacidad inicial de 16 puntos por contorno
        fprintf(stderr, "ERROR::GLYPH_MANAGER::ADD_CONTOUR_TO_GLYPH_OUTLINE: Fallo al inicializar un nuevo contorno.\n");
        // No es necesario limpiar aquí ya que initContour no debería haber modificado data->contours[data->count] si falló.
        return NULL; // Error
    }
    // Devuelve el puntero al nuevo contorno y luego incrementa 'count'
    return &data->contours[data->count++];
}

static void freeGlyphOutlineData(GlyphOutlineData* data) {
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

// --- Callbacks estáticos de FreeType-style y helpers ---

// Helper para convertir FT_Vector a Point2D (similar al anterior ftVecToPoint2D)
static Point2D ft_vector_to_point2d(const FT_Vector* vec) {
    return (Point2D){ (float)vec->x / 64.0f, (float)vec->y / 64.0f };
}

// Helper para evaluar curvas Bézier cuadráticas (similar al anterior)
static Point2D evaluate_quadratic_bezier(float t, Point2D p0, Point2D p1, Point2D p2) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float ut2 = 2.0f * u * t;
    return (Point2D){
        uu * p0.x + ut2 * p1.x + tt * p2.x,
        uu * p0.y + ut2 * p1.y + tt * p2.y
    };
}

static int glyph_manager_ftMoveToFunc(const FT_Vector* to, void* userData) {
    GlyphOutlineData* data = (GlyphOutlineData*)userData;
    ContourC* newContour = addContourToGlyphOutline(data);
    if (!newContour) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_MOVE_TO_FUNC: Error al añadir nuevo contorno.\n");
        return 1; // FreeType espera un entero no-cero para error
    }
    data->currentPoint = ft_vector_to_point2d(to);
    if (addPointToContour(newContour, data->currentPoint) != 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_MOVE_TO_FUNC: Error al añadir punto al nuevo contorno.\n");
        // El contorno parcialmente añadido se limpiará por freeGlyphOutlineData
        return 1; // Error
    }
    return 0; // Éxito
}

static int glyph_manager_ftLineToFunc(const FT_Vector* to, void* userData) {
    GlyphOutlineData* data = (GlyphOutlineData*)userData;
    if (data->count == 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_LINE_TO_FUNC: No hay contorno activo.\n");
        return 1; // Error
    }
    data->currentPoint = ft_vector_to_point2d(to);
    if (addPointToContour(&data->contours[data->count - 1], data->currentPoint) != 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_LINE_TO_FUNC: Error al añadir punto al contorno existente.\n");
        return 1; // Error
    }
    return 0; // Éxito
}

static int glyph_manager_ftConicToFunc(const FT_Vector* control, const FT_Vector* to, void* userData) {
    GlyphOutlineData* data = (GlyphOutlineData*)userData;
    if (data->count == 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_CONIC_TO_FUNC: No hay contorno activo.\n");
        return 1; // Error
    }

    Point2D ctrlPt = ft_vector_to_point2d(control);
    Point2D endPt = ft_vector_to_point2d(to);
    Point2D startPt = data->currentPoint; // El punto actual es el inicio de la curva
    ContourC* currentContour = &data->contours[data->count - 1];

    for (int i = 1; i <= data->subdivisionSteps; ++i) {
        float t = (float)i / (float)data->subdivisionSteps;
        Point2D pointOnCurve = evaluate_quadratic_bezier(t, startPt, ctrlPt, endPt);
        if (addPointToContour(currentContour, pointOnCurve) != 0) {
            fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_CONIC_TO_FUNC: Error al añadir punto subdividido al contorno.\n");
            return 1; // Error
        }
    }
    data->currentPoint = endPt; // Actualizar el punto actual al final de la curva
    return 0; // Éxito
}

static int glyph_manager_ftCubicToFunc(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* userData) {
    GlyphOutlineData* data = (GlyphOutlineData*)userData;
    if (data->count == 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_CUBIC_TO_FUNC: No hay contorno activo.\n");
        return 1; // Error
    }
    // Para simplificar, solo nos movemos al punto final 'to', como en la versión original.
    // Una implementación completa subdividiría la cúbica.
    // fprintf(stderr, "Advertencia: glyph_manager_ftCubicToFunc no implementado para subdivision completa. Solo se usa el punto final.\n");
    data->currentPoint = ft_vector_to_point2d(to);
    if (addPointToContour(&data->contours[data->count - 1], data->currentPoint) != 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::FT_CUBIC_TO_FUNC: Error al añadir punto final al contorno.\n");
        return 1; // Error
    }
    return 0; // Éxito
}

// --- Función para Descomponer Contornos de FreeType ---
static int decompose_ft_outline(FT_GlyphSlot glyph, GlyphOutlineData* outlineData) {
    if (!glyph) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::DECOMPOSE_FT_OUTLINE: FT_GlyphSlot es NULL.\n");
        return -1; // Error
    }
    if (glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        // Esto no es necesariamente un error para glifos como el espacio.
        // createGlyphGeometry manejará esto devolviendo un GlyphInfo con vao=0.
        // No se imprime error aquí para no saturar la salida con espacios.
        return -2; // No es un error fatal, pero no se puede descomponer.
    }

    if (initGlyphOutlineData(outlineData, 4) != 0) { // Capacidad inicial de 4 contornos
        fprintf(stderr, "ERROR::GLYPH_MANAGER::DECOMPOSE_FT_OUTLINE: Fallo al inicializar GlyphOutlineData.\n");
        return -3; // Error
    }

    FT_Outline_Funcs ftFuncs;
    ftFuncs.move_to = glyph_manager_ftMoveToFunc;
    ftFuncs.line_to = glyph_manager_ftLineToFunc;
    ftFuncs.conic_to = glyph_manager_ftConicToFunc;
    ftFuncs.cubic_to = glyph_manager_ftCubicToFunc;
    ftFuncs.shift = 0;
    ftFuncs.delta = 0;

    FT_Error ftError = FT_Outline_Decompose(&glyph->outline, &ftFuncs, outlineData);
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::DECOMPOSE_FT_OUTLINE: FT_Outline_Decompose falló con error %d.\n", ftError);
        // outlineData se limpiará por el llamador (createGlyphGeometry)
        return -4; // Error
    }

    // Verificar si se generaron contornos válidos (al menos 3 puntos por contorno)
    int hasValidContours = 0;
    if (outlineData->count > 0) { // Solo verificar si hay algún contorno
        for (size_t i = 0; i < outlineData->count; ++i) {
            if (outlineData->contours[i].count >= 3) {
                hasValidContours = 1;
                break;
            }
        }
        if (!hasValidContours) {
            // No es un error fatal, pero no habrá nada que teselar.
            // createGlyphGeometry manejará esto.
            // fprintf(stderr, "ADVERTENCIA::GLYPH_MANAGER::DECOMPOSE_FT_OUTLINE: No se generaron contornos con suficientes puntos (mínimo 3).\n");
            return 0; // Éxito, pero sin datos útiles para teselar.
        }
    } else {
        // No hay contornos, podría ser un glifo vacío (como un espacio modificado) o un error en callbacks.
        // Si los callbacks fallaron, ya habrán impreso un error.
        // createGlyphGeometry manejará esto.
        return 0; // Éxito, pero sin datos.
    }

    return 0; // Éxito
}


GlyphInfo glyphCache[CACHE_SIZE]; // Definición (ya no extern si este es el único .c que lo usa)

// --- Función de Creación de Geometría (Refactorizada) ---
GlyphInfo createGlyphGeometry(char character) {
    GlyphInfo result = {0}; // Inicializar a 0 (vao=0 indica inválido/error)
    FT_Error ftError;
    GlyphOutlineData glyphOutline = {0}; // Inicializar a cero para seguridad
    int outlineDecomposed = 0;

    // 1. Cargar Glifo y Métricas
    ftError = FT_Set_Pixel_Sizes(ftFace, 0, 48); // Tamaño de píxel deseado
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::CREATE_GEOMETRY: FT_Set_Pixel_Sizes falló para '%c'. Error: %d\n", character, ftError);
        return result; // vao=0
    }

    FT_UInt glyph_index = FT_Get_Char_Index(ftFace, character);
    // No consideramos glyph_index == 0 un error fatal aquí, FT_Load_Glyph lo manejará (puede cargar .notdef)
    // if (glyph_index == 0 && character != 0) {
    //     fprintf(stderr, "ADVERTENCIA::GLYPH_MANAGER::CREATE_GEOMETRY: Glifo no encontrado para el carácter '%c'.\n", character);
    // }

    ftError = FT_Load_Glyph(ftFace, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::CREATE_GEOMETRY: FT_Load_Glyph falló para '%c' (índice %u). Error: %d\n", character, glyph_index, ftError);
        return result; // vao=0
    }

    result.advanceX = (float)(ftFace->glyph->advance.x) / 64.0f;

    if (ftFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        // Para glifos sin contornos (como el espacio), esto es normal.
        // No se imprime error, simplemente se devuelve un GlyphInfo con vao=0.
        // fprintf(stderr, "INFO::GLYPH_MANAGER::CREATE_GEOMETRY: Glifo '%c' no tiene formato outline.\n", character);
        return result; // vao=0, pero advanceX puede ser válido.
    }

    // 2. Descomponer Contornos
    int decomp_err = decompose_ft_outline(ftFace->glyph, &glyphOutline);
    outlineDecomposed = 1; // Marcar que glyphOutline fue al menos intentada inicializar y necesita limpieza.

    if (decomp_err != 0) {
        // decompose_ft_outline ya debería haber impreso un error específico.
        fprintf(stderr, "ERROR::GLYPH_MANAGER::CREATE_GEOMETRY: Fallo al descomponer el outline para '%c' (código %d).\n", character, decomp_err);
        freeGlyphOutlineData(&glyphOutline);
        return result; // vao=0
    }
    
    // Verificar si hay contornos válidos para teselar
    // (decompose_ft_outline ya hace una verificación básica, aquí podemos ser más estrictos si es necesario)
    if (glyphOutline.count == 0) {
        // Esto puede ocurrir si el glifo está vacío o los contornos no son válidos (e.g. < 3 puntos)
        // No es necesariamente un error fatal si el glifo es intencionalmente vacío.
        // fprintf(stderr, "INFO::GLYPH_MANAGER::CREATE_GEOMETRY: No hay contornos para teselar para '%c' después de la descomposición.\n", character);
        freeGlyphOutlineData(&glyphOutline);
        return result; // vao=0, pero advanceX puede ser válido
    }
    
    // 3. Teselar Contornos
    // Pasamos un puntero a GlyphOutlineData, que contiene ContourC* etc.
    // generateGlyphTessellation necesita ser adaptado si su firma esperaba OutlineDataC.
    // Asumiendo que generateGlyphTessellation puede manejar GlyphOutlineData o una estructura similar:
    TessellationResult tessResult = generateGlyphTessellation(glyphOutline.contours, glyphOutline.count);

    if (tessResult.allocationFailed || tessResult.vertices == NULL || tessResult.elements == NULL || tessResult.vertexCount == 0 || tessResult.elementCount == 0) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::CREATE_GEOMETRY: Fallo en la teselación para '%c'.\n", character);
        if(tessResult.allocationFailed) {
             fprintf(stderr, " (Causa: Fallo de alocación en teselador para '%c')\n", character);
        }
        // generateGlyphTessellation es responsable de liberar sus propios datos parciales si falla.
        // pero si devuelve datos y luego consideramos que no son válidos, debemos liberarlos.
        free(tessResult.vertices); // Seguro llamar con NULL
        free(tessResult.elements); // Seguro llamar con NULL
        freeGlyphOutlineData(&glyphOutline);
        return result; // vao=0
    }
    
    const int numIndices = tessResult.elementCount * 3; // Asumiendo triángulos

    // 4. Crear Buffers OpenGL
    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, tessResult.vertexCount * 2 * sizeof(TESSreal), tessResult.vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(TESSindex), tessResult.elements, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(TESSreal), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Desvincular VBO del target actual
    glBindVertexArray(0);             // Desvincular VAO

    // Liberar datos de teselación CPU-side ya que ahora están en GPU
    free(tessResult.vertices);
    free(tessResult.elements);

    // Guardar resultados en la estructura GlyphInfo
    result.vao = vao;
    result.vbo = vbo;
    result.ebo = ebo;
    result.indexCount = numIndices; // Ya es el número de índices, no de elementos.

    // 5. Cleanup
    if (outlineDecomposed) { // Solo limpiar si fue inicializada
        freeGlyphOutlineData(&glyphOutline);
    }

    return result;
}

// --- Nueva función para inicializar/llenar el caché ---
int initGlyphCache() {
    // Asegúrate de que FreeType (y ftFace) se inicializó antes de llamar a esto
    if (!ftFace) { // ftFace es una variable global extern de freetype_handler.h
        fprintf(stderr, "ERROR::GLYPH_MANAGER::INIT_GLYPH_CACHE: ftFace no inicializada (la fuente no se cargó correctamente).\n");
        return -1; // Indicar fallo
    }

    printf("Generando caché de glifos (ASCII 32-126)...\n");
    memset(glyphCache, 0, sizeof(glyphCache)); // Inicializar todo a 0

    for (int c = 32; c < 127; ++c) {
        glyphCache[c] = createGlyphGeometry((char)c);
        // Considerar un error si un carácter imprimible importante no se pudo generar.
        // Espacio (' ') y Tab ('\t') a menudo no tienen geometría visible, así que su vao == 0 es normal.
        if (glyphCache[c].vao == 0 && c != ' ' && c != '\t') {
             fprintf(stderr, "ADVERTENCIA::GLYPH_MANAGER::INIT_GLYPH_CACHE: No se pudo generar geometría para el carácter '%c' (ASCII %d).\n", (char)c, c);
             // Decidir si esto es un error fatal para el caché. Por ahora, es una advertencia.
             // Para hacerlo un error, descomentar la siguiente línea:
             // return -2; 
        }
    }

    printf("Caché de glifos generado.\n");
    return 0; // Éxito
}

// --- Función para obtener información del caché ---
GlyphInfo getGlyphInfo(char c) {
    unsigned char uc = (unsigned char)c; // Usar unsigned char para indexar array
    if (uc >= CACHE_SIZE) {
        static GlyphInfo invalidGlyph = {0};
        fprintf(stderr, "Advertencia: Caracter '%c' (ASCII %u) fuera de rango del caché (tamaño %d).\n", c, uc, CACHE_SIZE);
        // Intentar devolver '?' si está en caché
        if ('?' < CACHE_SIZE) return glyphCache['?'];
        return invalidGlyph;
    }
    return glyphCache[uc];
}

// --- Función para limpiar el caché ---
void cleanupGlyphCache() {
     printf("Limpiando caché de glifos...\n");
    for (int i = 0; i < CACHE_SIZE; ++i) {
        if (glyphCache[i].vao != 0) {
            glDeleteVertexArrays(1, &glyphCache[i].vao);
            glyphCache[i].vao = 0;
        }
        if (glyphCache[i].vbo != 0) {
            glDeleteBuffers(1, &glyphCache[i].vbo);
            glyphCache[i].vbo = 0;
        }
        if (glyphCache[i].ebo != 0) {
            glDeleteBuffers(1, &glyphCache[i].ebo);
            glyphCache[i].ebo = 0;
        }
        // Otras métricas como indexCount o advanceX no necesitan "limpieza" especial.
    }
    printf("Caché de glifos limpiado.\n");
}