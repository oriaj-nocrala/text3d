#include "glyph_manager.h"
#include "freetype_handler.h"     // Para ftFace, ftEmojiFace
#include "sdf_generator.h"        // Para generate_sdf_from_bitmap y free_sdf_bitmap
// #include "tessellation_handler.h" // No es necesaria si solo haces SDF

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para memset
#include <GL/glew.h> 

// Definición de la tabla hash (si no está ya en glyph_manager.h como extern)
GlyphCacheNode* glyphHashTable[HASH_TABLE_SIZE];
extern FT_Face ftFace;        // Declarada en freetype_handler.h
extern FT_Face ftEmojiFace;   // Declarada en freetype_handler.h


// Helper function to initialize GlyphInfo
static void init_glyph_info(GlyphInfo* info) {
    memset(info, 0, sizeof(GlyphInfo));
    // Inicializaciones específicas si 0 no es el valor por defecto deseado
    // info->vao = 0; // etc. ya cubierto por memset
}

static GlyphInfo generate_glyph_data_for_codepoint(FT_ULong char_code) {
    GlyphInfo result; 
    init_glyph_info(&result); 
    FT_Error ftError;
    FT_Face current_ft_face = ftFace; 

    if (!ftFace) { // ftFace debe estar inicializada por initFreeType() y loadFonts()
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: ftFace no está inicializada.\n");
        return result; // result está vacía/cero
    }

    FT_UInt glyph_index = FT_Get_Char_Index(current_ft_face, char_code);

    if (glyph_index == 0 && ftEmojiFace != NULL) {
        // printf("[GM] U+%04lX not in main font, trying emoji font.\n", char_code);
        current_ft_face = ftEmojiFace;
        glyph_index = FT_Get_Char_Index(current_ft_face, char_code);
    }
    
    if (glyph_index == 0) {
        // fprintf(stderr, "WARN::GLYPH_MANAGER::GENERATE_GLYPH: Glyph not found for U+%04lX. Returning empty glyph.\n", char_code);
        return result; // result.advanceX será 0.0, etc.
    }

    ftError = FT_Set_Pixel_Sizes(current_ft_face, 0, 192); // 48 
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Set_Pixel_Sizes falló para U+%04lX. Error: %d\n", char_code, ftError);
        return result; 
    }

    // Cargar el glifo. FT_LOAD_NO_BITMAP es para si solo quieres métricas de contorno.
    // Para SDF, necesitamos el bitmap, así que no usamos FT_LOAD_NO_BITMAP aquí.
    // O lo usamos y luego llamamos FT_Render_Glyph explícitamente.
    // Vamos a cargar con FT_LOAD_DEFAULT y luego renderizar si es outline.
    ftError = FT_Load_Glyph(current_ft_face, glyph_index, FT_LOAD_DEFAULT);
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Load_Glyph falló para U+%04lX (índice %u). Error: %d\n", char_code, glyph_index, ftError);
        return result;
    }

    // Diagnóstico de FreeType advance.x (formato 26.6)
    printf("  [GlyphManager DEBUG] Char U+%04lX (idx %u): current_ft_face->glyph->advance.x = %ld (raw 26.6 units)\n",
           char_code, glyph_index, current_ft_face->glyph->advance.x);

    result.advanceX = (float)(current_ft_face->glyph->advance.x) / 64.0f; // Convertir a píxeles

    // Renderizar el glifo a un bitmap para SDF y para obtener métricas de bitmap correctas
    // Es importante renderizar ANTES de acceder a glyph->bitmap_left/top y glyph->bitmap.
    if (current_ft_face->glyph->format == FT_GLYPH_FORMAT_BITMAP) {
        // El glifo ya es un bitmap (ej. emojis de color, o fuentes bitmap)
        // No se puede (o no tiene sentido) generar SDF a partir de esto de la misma manera que de un outline.
        // Podríamos intentar usarlo directamente o generar un pseudo-SDF.
        // Por ahora, lo trataremos como si no pudiéramos generar SDF de él.
         // FT_Render_Glyph(current_ft_face->glyph, FT_RENDER_MODE_NORMAL); // ¿O ya está renderizado?
    } else if (current_ft_face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
        ftError = FT_Render_Glyph(current_ft_face->glyph, FT_RENDER_MODE_NORMAL); // Render to 8-bit grayscale bitmap
        if (ftError) {
            fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Render_Glyph failed for U+%04lX. Error: %d\n", char_code, ftError);
            // advanceX ya está seteado. bitmap_left/top podrían no ser válidos.
            // Devolver result como está (sin datos de textura/bitmap).
            return result; 
        }
    } else {
         fprintf(stderr, "WARN::GLYPH_MANAGER::GENERATE_GLYPH: Glyph U+%04lX has unhandled format %d.\n", char_code, current_ft_face->glyph->format);
         return result; // No se puede procesar para SDF
    }

    // Ahora que glyph->bitmap está poblado (si FT_Render_Glyph tuvo éxito o era un bitmap):
    FT_Bitmap* ft_bitmap = &current_ft_face->glyph->bitmap;
    result.bitmap_left = current_ft_face->glyph->bitmap_left;
    result.bitmap_top  = current_ft_face->glyph->bitmap_top;

    printf("  [GlyphManager DEBUG FT_Bitmap] Char U+%04lX: width=%d, rows=%d, pitch=%d, num_grays=%d, pixel_mode=%d\n",
       char_code, ft_bitmap->width, ft_bitmap->rows, ft_bitmap->pitch, ft_bitmap->num_grays, ft_bitmap->pixel_mode);

    if (ft_bitmap->buffer && ft_bitmap->width > 0 && ft_bitmap->rows > 0) {

        printf("    First 5x5 pixels of FT_Bitmap for U+%04lX:\n", char_code);
        for (int r = 0; r < 5 && r < ft_bitmap->rows; ++r) {
            printf("      ");
            for (int c = 0; c < 5 && c < ft_bitmap->width; ++c) {
                printf("%03u ", (unsigned char)ft_bitmap->buffer[r * ft_bitmap->pitch + c]);
            }
            printf("\n");
        }

        int sdf_padding = 4; 
        unsigned char* sdf_data = generate_sdf_from_bitmap(
            ft_bitmap->buffer,
            ft_bitmap->width,
            ft_bitmap->rows,
            ft_bitmap->pitch,
            sdf_padding,
            &result.sdfTextureWidth,
            &result.sdfTextureHeight
        );

        if (sdf_data) {
        #ifndef UNIT_TESTING
            glGenTextures(1, &result.sdfTextureID);
            glBindTexture(GL_TEXTURE_2D, result.sdfTextureID);

            // === ESTO ARREGLÓ EL PROBLEMA DE LAS LETRAS GARBAGE ===
            /*
            The problem of deformed or "sheared" letters, especially when some 
            characters render correctly while others don't, and the issue persists 
            across different GPUs, strongly points to a data alignment problem when 
            providing texture data to OpenGL. Specifically, the GL_UNPACK_ALIGNMENT 
            parameter, which defaults to 4, is the likely culprit.

            GL_UNPACK_ALIGNMENT: OpenGL, by default, assumes that the starting address
            of each row of pixel data you provide (e.g., to glTexImage2D) is aligned 
            to a 4-byte boundary.
            SDF Bitmap Data: In sdf_generator.c, the output SDF bitmap (sdf_bitmap_out) 
            is a tightly packed array where each row immediately follows the previous. 
            The width of this bitmap (sdf_w, which becomes result.sdfTextureWidth in 
            glyph_manager.c) depends on the original FreeType bitmap width plus padding. 
            This width is not guaranteed to be a multiple of 4.
                unsigned char* sdf_bitmap_out = (unsigned char*)malloc(sdf_w * sdf_h);
                Data is accessed as sdf_bitmap_out[y * sdf_w + x].
            */
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, result.sdfTextureWidth, result.sdfTextureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, sdf_data);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // === OPTIONAL: Restore default alignment if other parts of your code expect it ===
            // glPixelStorei(GL_UNPACK_ALIGNMENT, 4); 

            glBindTexture(GL_TEXTURE_2D, 0);
        #else
            result.sdfTextureID = 0; 
        #endif
            free_sdf_bitmap(sdf_data); // Usar la función de tu sdf_generator.h
        } else {
            fprintf(stderr, "WARN::GLYPH_MANAGER::GENERATE_GLYPH: SDF generation failed for U+%04lX.\n", char_code);
            result.sdfTextureID = 0;
            result.sdfTextureWidth = 0;
            result.sdfTextureHeight = 0;
        }
    } else { 
        // Si FT_Render_Glyph falló o el bitmap estaba vacío, no hay datos de textura.
        // bitmap_left/top podrían haberse establecido desde el slot antes del fallo de render,
        // pero no corresponden a un bitmap renderizado utilizable para SDF.
        // Es mejor dejarlos como los puso FT_Load_Glyph o ponerlos a 0 si no hay bitmap.
        // Si FT_Render_Glyph falló, ya hemos retornado.
        // Si el bitmap es simplemente vacío (ej. para espacio ' '):
        if (char_code != ' ') { // No imprimas warnings para el espacio
             // fprintf(stderr, "WARN::GLYPH_MANAGER::GENERATE_GLYPH: Bitmap for U+%04lX is empty after render.\n", char_code);
        }
        result.sdfTextureID = 0;
        result.sdfTextureWidth = 0;
        result.sdfTextureHeight = 0;
        // result.bitmap_left = 0; // Opcional: resetear si no hay bitmap válido.
        // result.bitmap_top = 0;
    }
    
    // VAO, VBO, EBO e indexCount no se usan para renderizado SDF puro con un quad global
    result.vao = 0;
    result.vbo = 0;
    result.ebo = 0;
    result.indexCount = 0;
    
    return result;
}

int initGlyphCache() {
    if (!ftFace && !ftEmojiFace) { // Al menos una fuente debe estar cargada
        fprintf(stderr, "ERROR::GLYPH_MANAGER::INIT_GLYPH_CACHE: Ninguna fuente (ftFace) inicializada. Llame a loadFonts primero.\n");
        // return -1; // O permitir inicializar la cache vacía.
    }
    printf("Inicializando caché de glifos (tabla hash).\n");
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        glyphHashTable[i] = NULL;
    }
    printf("Caché de glifos listo.\n");
    return 0; 
}

GlyphInfo getGlyphInfo(FT_ULong char_code) {
    // Podrías querer una GlyphInfo "inválida" estática para devolver en caso de error grave.
    // static GlyphInfo invalidGlyph; // inicializada a ceros globalmente o con init_glyph_info
    // if (!ftFace && !ftEmojiFace) return invalidGlyph; // O manejar error

    unsigned int hash_index = char_code % HASH_TABLE_SIZE;
    GlyphCacheNode* node = glyphHashTable[hash_index];

    while (node != NULL) {
        if (node->char_code == char_code) {
            return node->glyph_info; 
        }
        node = node->next;
    }

    GlyphInfo new_glyph_data = generate_glyph_data_for_codepoint(char_code);

    // No imprimir warnings si el VAO es 0, ya que no lo estamos usando para SDF con quad global.
    // if (new_glyph_data.sdfTextureID == 0 && char_code != ' ' && char_code != '\t' && char_code != '\n' && char_code != 0xFFFD) {
    //     fprintf(stderr, "ADVERTENCIA::GLYPH_MANAGER::GET_GLYPH_INFO: No se generó textura SDF para U+%04lX.\n", char_code);
    // }

    GlyphCacheNode* newNode = (GlyphCacheNode*)malloc(sizeof(GlyphCacheNode));
    if (newNode == NULL) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GET_GLYPH_INFO: Malloc falló para GlyphCacheNode U+%04lX\n", char_code);
        // Devolver una GlyphInfo vacía o la new_glyph_data (que podría estar parcialmente vacía si falló la gen)
        init_glyph_info(&new_glyph_data); // Devolver una vacía segura
        return new_glyph_data; 
    }
    newNode->char_code = char_code;
    newNode->glyph_info = new_glyph_data; // Copia la estructura
    
    newNode->next = glyphHashTable[hash_index];
    glyphHashTable[hash_index] = newNode;
    
    return newNode->glyph_info; // Devuelve la info del nuevo nodo (una copia)
}

void cleanupGlyphCache() {
    printf("Limpiando caché de glifos...\n");
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        GlyphCacheNode* node = glyphHashTable[i];
        while (node != NULL) {
            GlyphCacheNode* temp = node;
            node = node->next;

        #ifndef UNIT_TESTING
            // VAO, VBO, EBO no se crean individualmente para SDF con quad global,
            // así que no hay necesidad de borrarlos aquí.
            // glDeleteVertexArrays(1, &temp->glyph_info.vao);
            // glDeleteBuffers(1, &temp->glyph_info.vbo);
            // glDeleteBuffers(1, &temp->glyph_info.ebo);
            
            if (temp->glyph_info.sdfTextureID != 0) {
                glDeleteTextures(1, &temp->glyph_info.sdfTextureID);
            }
        #endif
            free(temp);
        }
        glyphHashTable[i] = NULL;
    }
    printf("Caché de glifos limpiado.\n");
}