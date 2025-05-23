#include "glyph_manager.h"
#include "freetype_handler.h"
#include "tessellation_handler.h"
#include "sdf_generator.h" // Added for SDF generation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h> // Sigue siendo necesario para los tipos GLuint, etc.

// Definición de la tabla hash
GlyphCacheNode* glyphHashTable[HASH_TABLE_SIZE];

static GlyphInfo generate_glyph_data_for_codepoint(FT_ULong char_code);

// Helper function to initialize GlyphInfo
static void init_glyph_info(GlyphInfo* info) {
    memset(info, 0, sizeof(GlyphInfo));
    // Specific initializations if 0 is not the desired default for some fields
    // For example, if sdfTextureID should be a specific "invalid" marker other than 0
}


int initGlyphCache() {
    if (!ftFace) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::INIT_GLYPH_CACHE: ftFace no inicializada. Llame a loadFonts primero.\n");
        return -1; 
    }
    printf("Inicializando caché de glifos (tabla hash).\n");
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        glyphHashTable[i] = NULL;
    }
    printf("Caché de glifos listo.\n");
    return 0; 
}

GlyphInfo getGlyphInfo(FT_ULong char_code) {
    static GlyphInfo invalidGlyph = {0}; 
    unsigned int hash_index = char_code % HASH_TABLE_SIZE;

    GlyphCacheNode* node = glyphHashTable[hash_index];
    while (node != NULL) {
        if (node->char_code == char_code) {
            return node->glyph_info; 
        }
        node = node->next;
    }

    GlyphInfo new_glyph_data = generate_glyph_data_for_codepoint(char_code);

    if (new_glyph_data.vao == 0 && char_code != ' ' && char_code != '\t' && char_code != '\n' && char_code != 0xFFFD) {
        // En modo UNIT_TESTING, vao será 0, así que este mensaje podría aparecer más a menudo.
        // Se podría añadir #ifndef UNIT_TESTING alrededor de este fprintf si es muy ruidoso durante las pruebas.
        // fprintf(stderr, "ADVERTENCIA::GLYPH_MANAGER::GET_GLYPH_INFO: No se generó VAO para U+%04lX. Se cacheará como glifo vacío.\n", char_code);
    }

    GlyphCacheNode* newNode = (GlyphCacheNode*)malloc(sizeof(GlyphCacheNode));
    if (newNode == NULL) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GET_GLYPH_INFO: Malloc falló para GlyphCacheNode U+%04lX\n", char_code);
        return invalidGlyph; 
    }
    newNode->char_code = char_code;
    newNode->glyph_info = new_glyph_data;
    
    newNode->next = glyphHashTable[hash_index];
    glyphHashTable[hash_index] = newNode;
    
    return newNode->glyph_info;
}

static GlyphInfo generate_glyph_data_for_codepoint(FT_ULong char_code) {
    GlyphInfo result; 
    init_glyph_info(&result); // Initialize all fields to 0 or default
    FT_Error ftError;
    FT_Face current_ft_face = ftFace; 

    if (!ftFace) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: ftFace no está inicializada.\n");
        return result;
    }

    FT_UInt glyph_index = FT_Get_Char_Index(current_ft_face, char_code);

    if (glyph_index == 0 && ftEmojiFace != NULL) {
        current_ft_face = ftEmojiFace;
        glyph_index = FT_Get_Char_Index(current_ft_face, char_code);
    }
    
    if (glyph_index == 0) {
        return result; 
    }

    ftError = FT_Set_Pixel_Sizes(current_ft_face, 0, 48); 
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Set_Pixel_Sizes falló para U+%04lX. Error: %d\n", char_code, ftError);
        return result;
    }

    ftError = FT_Load_Glyph(current_ft_face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Load_Glyph falló para U+%04lX (índice %u). Error: %d\n", char_code, glyph_index, ftError);
        return result;
    }

    result.advanceX = (float)(current_ft_face->glyph->advance.x) / 64.0f;

    // SDF Generation Attempt
    if (current_ft_face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) { // SDF typically needs an outline
        ftError = FT_Render_Glyph(current_ft_face->glyph, FT_RENDER_MODE_NORMAL); // Render to bitmap
        if (ftError) {
            fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Render_Glyph failed for U+%04lX. Error: %d\n", char_code, ftError);
            // Proceed to mesh generation or return. For now, let mesh generation be the fallback.
        } else {
            FT_Bitmap* ft_bitmap = &current_ft_face->glyph->bitmap;
            if (ft_bitmap->buffer && ft_bitmap->width > 0 && ft_bitmap->rows > 0) {
                int sdf_padding = 4; // Example padding
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
                    // === OpenGL Texture Creation ===
                    glGenTextures(1, &result.sdfTextureID);
                    glBindTexture(GL_TEXTURE_2D, result.sdfTextureID);
                    // Use GL_RED for internal format if the SDF is single channel, GL_R8 is better.
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, result.sdfTextureWidth, result.sdfTextureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, sdf_data);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glBindTexture(GL_TEXTURE_2D, 0);
#else
                    result.sdfTextureID = 0; // No GL textures in unit tests
#endif
                    free_sdf_bitmap(sdf_data);
                } else {
                    fprintf(stderr, "WARN::GLYPH_MANAGER::GENERATE_GLYPH: SDF generation failed for U+%04lX.\n", char_code);
                    result.sdfTextureID = 0; // Ensure fields are zeroed on failure
                    result.sdfTextureWidth = 0;
                    result.sdfTextureHeight = 0;
                }
            } else {
                // Bitmap is empty, clear SDF fields
                result.sdfTextureID = 0;
                result.sdfTextureWidth = 0;
                result.sdfTextureHeight = 0;
            }
        }
    } else { // Not an outline glyph, or some other condition preventing SDF generation
        result.sdfTextureID = 0;
        result.sdfTextureWidth = 0;
        result.sdfTextureHeight = 0;
    }

    // Mesh Generation (Outline based)
    // This part might be conditional in the future based on SDF success.
    // For now, it always runs if it's an outline glyph.
    if (current_ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        // If it's not an outline glyph (e.g. pre-rendered bitmap), 
        // we might not be able to generate a mesh this way.
        // Or if it was, but SDF failed and we don't want to proceed with mesh for non-outlines.
        // For now, if no outline, result (with potentially SDF data if that path changes) is returned.
        return result; 
    }

    OutlineDataC outlineData;
    if (initOutlineData(&outlineData, 4) != 0) { 
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: Fallo al inicializar OutlineDataC para U+%04lX.\n", char_code);
        return result;
    }

    FT_Outline_Funcs ftFuncs = {0};
    ftFuncs.move_to = ftMoveToFunc;
    ftFuncs.line_to = ftLineToFunc;
    ftFuncs.conic_to = ftConicToFunc;
    ftFuncs.cubic_to = ftCubicToFunc;

    ftError = FT_Outline_Decompose(&current_ft_face->glyph->outline, &ftFuncs, &outlineData);
    if (ftError) {
        fprintf(stderr, "ERROR::GLYPH_MANAGER::GENERATE_GLYPH: FT_Outline_Decompose falló para U+%04lX. Error: %d\n", char_code, ftError);
        freeOutlineData(&outlineData);
        return result;
    }
    
    int hasValidContours = 0;
    if (outlineData.count > 0) {
        for (size_t i = 0; i < outlineData.count; ++i) {
            if (outlineData.contours[i].count >= 3) {
                hasValidContours = 1;
                break;
            }
        }
    }

    if (!hasValidContours) {
        freeOutlineData(&outlineData);
        return result; 
    }
    
    TessellationResult tessResult = generateGlyphTessellation(&outlineData);
    freeOutlineData(&outlineData); 

    if (tessResult.allocationFailed || tessResult.vertices == NULL || tessResult.elements == NULL || tessResult.vertexCount == 0 || tessResult.elementCount == 0) {
        // No es necesario imprimir error aquí si generateGlyphTessellation ya lo hizo.
        free(tessResult.vertices); 
        free(tessResult.elements);
        return result;
    }
    
    // Calculamos indexCount incluso en modo test
    result.indexCount = tessResult.elementCount * 3;

#ifndef UNIT_TESTING
    // --- Crear Buffers OpenGL (VBO, EBO, VAO) ---
    // Estas llamadas fallarían sin un contexto OpenGL
    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);

    glBindVertexArray(result.vao);

    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, tessResult.vertexCount * 2 * sizeof(TESSreal), tessResult.vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, result.indexCount * sizeof(TESSindex), tessResult.elements, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(TESSreal), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);             
#else
    // En UNIT_TESTING, vao, vbo, ebo permanecerán en 0.
    // result.indexCount ya se calculó arriba.
#endif
    
    free(tessResult.vertices);
    free(tessResult.elements);
    
    return result;
}

void cleanupGlyphCache() {
    printf("Limpiando caché de glifos...\n");
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        GlyphCacheNode* node = glyphHashTable[i];
        while (node != NULL) {
            GlyphCacheNode* temp = node;
            node = node->next;

#ifndef UNIT_TESTING
            // Estas llamadas fallarían sin un contexto OpenGL
            if (temp->glyph_info.vao != 0) {
                glDeleteVertexArrays(1, &temp->glyph_info.vao);
            }
            if (temp->glyph_info.vbo != 0) {
                glDeleteBuffers(1, &temp->glyph_info.vbo);
            }
            if (temp->glyph_info.ebo != 0) {
                glDeleteBuffers(1, &temp->glyph_info.ebo);
            }
            if (temp->glyph_info.sdfTextureID != 0) { // Removed comparison with dummy 99
                glDeleteTextures(1, &temp->glyph_info.sdfTextureID);
            }
#endif
            free(temp);
        }
        glyphHashTable[i] = NULL;
    }
    printf("Caché de glifos limpiado.\n");
}