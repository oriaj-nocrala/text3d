#include "glyph_manager.h"      // Incluye nuestra propia cabecera primero
#include "freetype_handler.h"   // Para acceder a ftFace, ftEmojiFace y funciones de FreeType
#include "tessellation_handler.h" // Para generateGlyphTessellation
#include "vector_utils.h"       // Para OutlineDataC, initOutlineData, etc.
#include "tesselator.h"         // Para TESSreal, TESSindex, etc. (si generateGlyphTessellation los usa)

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para memset (aunque ya no lo usamos para glyphCache)

// Definición de la tabla hash
GlyphCacheNode* glyphHashTable[HASH_TABLE_SIZE];

// Declaraciones extern para las caras de FreeType (deben estar definidas en freetype_handler.c)
// y freetype_handler.h debe ser incluido por freetype_handler.c
extern FT_Face ftFace;
extern FT_Face ftEmojiFace; // Asegúrate de que esto esté definido en freetype_handler.c si lo usas

// Declaración forward para la función auxiliar interna
static GlyphInfo generate_glyph_data_for_codepoint(FT_ULong char_code);

// --- Inicialización del Caché de Glifos ---
int initGlyphCache() {
    // Asegurarse de que FreeType (y ftFace) se inicializó antes de llamar a esto
    if (!ftFace) {
        fprintf(stderr, "Error Critico: ftFace no está inicializada antes de initGlyphCache().\n");
        fprintf(stderr, "Asegúrate de que initFreeType() y loadFont() se llamen primero.\n");
        return 0; // Indicar fallo
    }

    printf("Inicializando estructura del caché de glifos (tabla hash)...\n");

    // Inicializar todas las entradas de la tabla hash a NULL
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        glyphHashTable[i] = NULL;
    }

    printf("Caché de glifos listo para usarse (se llenará bajo demanda).\n");
    return 1; // Indicar éxito
}

// --- Obtener Información del Glifo del Caché (o generarla) ---
GlyphInfo getGlyphInfo(FT_ULong char_code) {
    static GlyphInfo invalidGlyph = {0}; // Para devolver en caso de error grave
    unsigned int hash_index = char_code % HASH_TABLE_SIZE;

    // 1. Buscar en la tabla hash
    GlyphCacheNode* node = glyphHashTable[hash_index];
    while (node != NULL) {
        if (node->char_code == char_code) {
            return node->glyph_info; // Encontrado
        }
        node = node->next;
    }

    // 2. No encontrado (Cache Miss): Generar, crear nodo y añadir a la tabla hash
    GlyphInfo new_glyph_data = generate_glyph_data_for_codepoint(char_code);

    // Si la generación falló (vao == 0) y no es un espacio en blanco u otro carácter
    // que intencionalmente no tiene geometría, podríamos optar por no cachearlo,
    // o cachear el "fallo" para no reintentar. Por ahora, lo cachearemos.
    if (new_glyph_data.vao == 0 && char_code != ' ' && char_code != '\t' && char_code != '\n') {
         fprintf(stderr, "Advertencia: No se generó VAO para U+%04lX. Se cacheará como glifo vacío.\n", char_code);
    }

    GlyphCacheNode* newNode = (GlyphCacheNode*)malloc(sizeof(GlyphCacheNode));
    if (newNode == NULL) {
        fprintf(stderr, "Error Critico: Malloc falló para GlyphCacheNode U+%04lX\n", char_code);
        // En un caso real, esto es un error grave. Podrías intentar limpiar y salir.
        return invalidGlyph; // Devolver un glifo inválido
    }
    newNode->char_code = char_code;
    newNode->glyph_info = new_glyph_data; // La información recién generada
    
    // Añadir el nuevo nodo a la cabeza de la lista para esta entrada del hash (más simple)
    newNode->next = glyphHashTable[hash_index];
    glyphHashTable[hash_index] = newNode;
    
    return newNode->glyph_info;
}

// --- Generar Datos del Glifo para un Punto de Código Dado ---
static GlyphInfo generate_glyph_data_for_codepoint(FT_ULong char_code) {
    GlyphInfo result = {0}; // Inicializar a 0 (vao=0 indica inválido/error)
    FT_Error ftError;
    FT_Face face_to_use = ftFace; // Empezar con la fuente principal

    if (!ftFace) {
        fprintf(stderr, "Error Critico en generate_glyph_data_for_codepoint: ftFace no está inicializada.\n");
        return result; // Devuelve GlyphInfo vacía
    }

    // --- Lógica de Fallback de Fuentes ---
    FT_UInt glyph_index = FT_Get_Char_Index(face_to_use, char_code);

    // Intentar con la fuente de emoji si el glifo no está en la principal y ftEmojiFace está disponible
    if (glyph_index == 0 && ftEmojiFace != NULL) {
        face_to_use = ftEmojiFace;
        glyph_index = FT_Get_Char_Index(face_to_use, char_code);
    }

    // Si aún no se encuentra, es un glifo no definido en las fuentes disponibles
    if (glyph_index == 0) {
        // fprintf(stderr, "Advertencia: Glifo para U+%04lX no encontrado en ninguna fuente.\n", char_code);
        // Se devolverá un GlyphInfo vacío (result ya está inicializado a ceros),
        // pero con advanceX = 0. Podrías querer un avance por defecto para glifos faltantes.
        // O cargar un glifo de reemplazo específico (como '.notdef' o '?')
        // FT_ULong replacement_char = (FT_ULong)'?'; // o 0xFFFD
        // glyph_index = FT_Get_Char_Index(ftFace, replacement_char); // Usar la fuente principal para '?'
        // if(glyph_index == 0) return result; // Ni siquiera '?' existe
        // face_to_use = ftFace; // Asegurarse de usar la fuente principal para '?'
        return result;
    }

    // --- Cargar el Glifo ---
    // Establecer el tamaño de píxel deseado para el glifo
    ftError = FT_Set_Pixel_Sizes(face_to_use, 0, 48); // Tamaño de fuente configurable, 48 es un ejemplo
    if (ftError) {
        fprintf(stderr, "Error FT_Set_Pixel_Sizes para U+%04lX: %d\n", char_code, ftError);
        return result;
    }

    // Cargar el glifo. FT_LOAD_NO_BITMAP para obtener contornos para teselar.
    // Para emojis a color, podrías usar FT_LOAD_COLOR y manejar formatos de bitmap.
    ftError = FT_Load_Glyph(face_to_use, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
    if (ftError) {
        fprintf(stderr, "Error FT_Load_Glyph para U+%04lX (índice %u): %d\n", char_code, glyph_index, ftError);
        return result;
    }

    // --- Obtener Métrica de Avance ---
    result.advanceX = (float)(face_to_use->glyph->advance.x) / 64.0f;

    // --- Comprobar si tiene Contorno (necesario para teselar) ---
    if (face_to_use->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        // Esto es normal para espacios, tabulaciones, o si cargaste un bitmap (emoji a color).
        // Si es un espacio, advanceX es lo que importa, y result.vao será 0.
        // fprintf(stderr, "Nota: Glifo U+%04lX (formato %d) no tiene formato outline. No se teselará.\n",
        // char_code, face_to_use->glyph->format);
        return result; // Devuelve con vao=0, pero advanceX puede ser válido.
    }

    // --- Extraer Contornos ---
    OutlineDataC outlineData;
    // Capacidad inicial de 4 contornos, se redimensionará si es necesario.
    if (!initOutlineData(&outlineData, 4)) {
        fprintf(stderr, "Error initOutlineData para U+%04lX\n", char_code);
        return result;
    }

    FT_Outline_Funcs ftFuncs;
    ftFuncs.move_to = ftMoveToFunc;     // Definido en freetype_handler.c
    ftFuncs.line_to = ftLineToFunc;     // Definido en freetype_handler.c
    ftFuncs.conic_to = ftConicToFunc;   // Definido en freetype_handler.c
    ftFuncs.cubic_to = ftCubicToFunc;   // Definido en freetype_handler.c
    ftFuncs.shift = 0;
    ftFuncs.delta = 0;

    ftError = FT_Outline_Decompose(&face_to_use->glyph->outline, &ftFuncs, &outlineData);
    if (ftError) {
        fprintf(stderr, "Error FT_Outline_Decompose para U+%04lX: %d\n", char_code, ftError);
        freeOutlineData(&outlineData); // Limpiar lo que se haya asignado
        return result;
    }

    // --- Teselación ---
    int hasValidContours = 0;
    for (size_t i = 0; i < outlineData.count; ++i) {
        if (outlineData.contours[i].count >= 3) { // Un polígono necesita al menos 3 puntos
            hasValidContours = 1;
            break;
        }
    }

    if (!hasValidContours) {
        // fprintf(stderr, "Advertencia: No hay contornos válidos para teselar para U+%04lX.\n", char_code);
        freeOutlineData(&outlineData);
        return result; // Normal para un espacio, devuelve con vao=0 pero advanceX correcto.
    }

    TessellationResult tessResult = generateGlyphTessellation(&outlineData);
    freeOutlineData(&outlineData); // Los datos del contorno ya no son necesarios

    if (tessResult.allocationFailed || tessResult.vertexCount == 0 || tessResult.elementCount == 0) {
        // fprintf(stderr, "Error de Teselación o sin resultados para U+%04lX.\n", char_code);
        free(tessResult.vertices); // free(NULL) es seguro
        free(tessResult.elements);
        return result; // No se pudo teselar o no hay geometría
    }

    // --- Crear Buffers OpenGL (VBO, EBO, VAO) ---
    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);

    glBindVertexArray(result.vao);

    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, tessResult.vertexCount * 2 * sizeof(TESSreal), tessResult.vertices, GL_STATIC_DRAW);

    result.indexCount = tessResult.elementCount * 3; // Asumiendo triángulos de libtess2
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, result.indexCount * sizeof(TESSindex), tessResult.elements, GL_STATIC_DRAW);

    // Atributo de vértice para la posición (2 floats)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(TESSreal), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Desvincular VBO del target actual
    glBindVertexArray(0);             // Desvincular VAO

    // Liberar la memoria de los resultados de la teselación ahora que están en la GPU
    free(tessResult.vertices);
    free(tessResult.elements);
    
    return result; // ¡Glifo generado y listo!
}


// --- Limpieza del Caché de Glifos ---
void cleanupGlyphCache() {
    printf("Limpiando caché de glifos (tabla hash)...\n");
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        GlyphCacheNode* node = glyphHashTable[i];
        while (node != NULL) {
            GlyphCacheNode* temp = node;
            node = node->next; // Avanzar ANTES de liberar temp

            // Limpiar los buffers de OpenGL si existen
            if (temp->glyph_info.vao != 0) {
                glDeleteVertexArrays(1, &temp->glyph_info.vao);
                // Es buena práctica también borrar los buffers asociados si el VAO los gestionaba exclusivamente.
                // Si los VBO/EBO son compartidos, la lógica de borrado sería diferente.
                // Asumiendo que son exclusivos por glifo:
                glDeleteBuffers(1, &temp->glyph_info.vbo);
                glDeleteBuffers(1, &temp->glyph_info.ebo);
            }
            free(temp); // Liberar el nodo de la lista enlazada
        }
        glyphHashTable[i] = NULL; // Marcar el bucket como vacío
    }
    printf("Caché de glifos (tabla hash) limpiado.\n");
}

// **Próximos Pasos y Verificaciones:**

// 1.  **`freetype_handler.h` y `freetype_handler.c`**:
//     * Asegúrate de que `freetype_handler.h` declare `extern FT_Face ftFace;` y `extern FT_Face ftEmojiFace;` (si usas fallback).
//     * Asegúrate de que `freetype_handler.c` defina `FT_Face ftFace = NULL;` y `FT_Face ftEmojiFace = NULL;` (o como los inicialices).
//     * Asegúrate de que `loadFont()` en `freetype_handler.c` realmente cargue las fuentes en estas variables.
//     * Las funciones de callback de FreeType (`ftMoveToFunc`, etc.) deben estar en `freetype_handler.c` y sus prototipos en `freetype_handler.h` si `generate_glyph_data_for_codepoint` las necesita (lo cual hace).

// 2.  **Compila de Nuevo:** Intenta compilar con estos cambios. Los errores de "unknown type" y "conflicting types" deberían reducirse drásticamente o desaparecer si las inclusiones y definiciones son correctas.

// 3.  **Errores Restantes:** Si quedan errores, obsérvalos cuidadosamente. A menudo, un error inicial puede causar una cascada de otros.

// 4.  **Advertencia `-Wsign-compare`**:
//     * `src/glyph_manager.c: In function ‘getGlyphInfo’: src/glyph_manager.c:123:29: warning: comparison of integer expressions of different signedness: ‘int’ and ‘FT_ULong’ {aka ‘long unsigned int’} [-Wsign-compare] if (node->char_code == char_code)`
//     * Esto se debe a que `node->char_code` es `FT_ULong` (que es `long unsigned int`) y `char_code` (el parámetro de la función) también es `FT_ULong`. Esto parece un falso positivo o un malentendido del compilador, ya que ambos deberían ser del mismo tipo. Si `node->char_code` fuera `int` y `char_code` `FT_ULong`, sí sería un problema. Verifica la definición de `GlyphCacheNode` en `glyph_manager.h` para asegurar que `char_code` sea `FT_ULong`. Si ambos son `FT_ULong`, esta advertencia es extraña pero generalmente inofensiva.

// 5.  **Advertencia `control reaches end of non-void function` en `getGlyphInfo`**:
//     * Asegúrate de que todas las rutas de código en `getGlyphInfo` realmente devuelvan un valor `GlyphInfo`. Parece que mi versión corregida de `getGlyphInfo` siempre devuelve algo (`node->glyph_info`, `invalidGlyph`, o `newNode->glyph_info`). Si el error persiste, revisa cuidadosamente los `if/else` y los retornos.

// Estos cambios son sustanciales pero deberían alinear tu código con el objetivo de un caché de glifos basado en tabla hash para Unicode. ¡Paciencia y a depur
