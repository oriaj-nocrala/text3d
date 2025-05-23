// En tu archivo t3d/src/renderer.c

#include "renderer.h"
#include "opengl_setup.h"  // For globalQuadVAO
#include "glyph_manager.h" // For actual getGlyphInfo and GlyphInfo struct
#include "utils.h"         // Para utf8_to_codepoint
#include "text_layout.h"   // For TextLayoutInfo and calculateTextLayout signature
#include <stdio.h> 
#include <GL/freeglut.h>
#include <string.h> 
#include <math.h>
#include <stdbool.h>

// ... (getGlyphMetrics_wrapper y calculateTextLayout sin cambios) ...
MinimalGlyphInfo getGlyphMetrics_wrapper(FT_ULong codepoint) {
#ifndef UNIT_TESTING
    GlyphInfo real_info = getGlyphInfo(codepoint);
    MinimalGlyphInfo min_info = {0};
    min_info.advanceX = real_info.advanceX; 
    min_info.codepoint = codepoint; 
    return min_info;
#else
    MinimalGlyphInfo dummy_info = {0};
    dummy_info.advanceX = 50; 
    dummy_info.codepoint = codepoint;
    return dummy_info;
#endif
}

TextLayoutInfo calculateTextLayout(
    const char* text, size_t cursorBytePos,
    float startX, float startY, float scale,
    float maxLineWidth, float lineHeight,
    GetGlyphMetricsFunc get_glyph_metrics) {

    TextLayoutInfo layout_info = {0};
    layout_info.cursor_pos.x = startX;
    layout_info.cursor_pos.y = startY;

    if (!text) return layout_info;

    const char* s_iter = text;
    size_t current_byte_iter_offset = 0;
    float simulatedCurrentX = startX;
    float simulatedCurrentY = startY; 

    while (*s_iter != '\0') {
        const char* temp_s_ptr = s_iter;
        FT_ULong codepoint_for_check = utf8_to_codepoint(&temp_s_ptr);
        if (codepoint_for_check == 0) break;
        size_t char_byte_length = temp_s_ptr - s_iter;

        MinimalGlyphInfo glyph_info_minimal = get_glyph_metrics(codepoint_for_check);

        if (simulatedCurrentX > startX && (simulatedCurrentX + (glyph_info_minimal.advanceX * scale)) > (startX + maxLineWidth)) {
            simulatedCurrentX = startX;
            simulatedCurrentY -= lineHeight; 
        }

        if (current_byte_iter_offset == cursorBytePos) {
            layout_info.cursor_pos.x = simulatedCurrentX;
            layout_info.cursor_pos.y = simulatedCurrentY; 
            layout_info.codepoint_under_cursor = codepoint_for_check;
            layout_info.glyph_info_under_cursor = glyph_info_minimal; 
            layout_info.cursor_is_over_char = 1;
        }
        simulatedCurrentX += glyph_info_minimal.advanceX * scale;
        s_iter += char_byte_length;
        current_byte_iter_offset += char_byte_length;
    }

    if (!layout_info.cursor_is_over_char && cursorBytePos == current_byte_iter_offset) {
        layout_info.cursor_pos.x = simulatedCurrentX;
        layout_info.cursor_pos.y = simulatedCurrentY;
    }
    return layout_info;
}


void renderText(GLuint shaderProgramID, const char* text, size_t cursorBytePos) {
#ifndef UNIT_TESTING
    checkOpenGLError("renderText Start");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkOpenGLError("After glClear");
    glUseProgram(shaderProgramID);
    checkOpenGLError("After glUseProgram");
    glBindVertexArray(globalQuadVAO); 
    checkOpenGLError("After glBindVertexArray globalQuadVAO");

    if (!text) {
        fprintf(stderr, "ERROR::RENDERER: El parámetro de texto es NULL.\n");
        glBindVertexArray(0); 
        glutSwapBuffers(); 
        return;
    }

    // --- Configuración de Renderizado ---
    float startX = -0.98f;
    float startY = 0.82f; 
    const float maxLineWidth = 1.96f;
    const float lineHeight = 0.18f; 
    float scale = 0.003f; 
    const int sdf_padding = 4; 

    TextLayoutInfo layout = calculateTextLayout(text, cursorBytePos, startX, startY, scale, maxLineWidth, lineHeight, getGlyphMetrics_wrapper);
    
    // --- Uniforms Base ---
    GLint transformLoc = glGetUniformLocation(shaderProgramID, "transform");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "textColor"); // Renombrado de textColor a baseTextColor para claridad
    GLint sdfTextureSamplerLoc = glGetUniformLocation(shaderProgramID, "sdfTexture"); // Nombre común para el sampler
    
    // === INICIO: NUEVOS UNIFORMS PARA EL SHADER SDF "MÁS PRO" ===
    GLint sdfEdgeValueLoc = glGetUniformLocation(shaderProgramID, "sdfEdgeValue");
    GLint smoothingFactorLoc = glGetUniformLocation(shaderProgramID, "smoothingFactor");

    // Uniforms para contorno (opcional, obtén sus localizaciones si los vas a usar)
    GLint enableOutlineLoc = glGetUniformLocation(shaderProgramID, "enableOutline");
    GLint outlineColorLoc = glGetUniformLocation(shaderProgramID, "outlineColor");
    GLint outlineWidthSDFLoc = glGetUniformLocation(shaderProgramID, "outlineWidthSDF");
    GLint outlineEdgeOffsetLoc = glGetUniformLocation(shaderProgramID, "outlineEdgeOffset");

    // Uniforms para sombra (opcional)
    GLint enableShadowLoc = glGetUniformLocation(shaderProgramID, "enableShadow");
    GLint shadowColorLoc = glGetUniformLocation(shaderProgramID, "shadowColor");
    GLint shadowOffsetScreenLoc = glGetUniformLocation(shaderProgramID, "shadowOffsetScreen"); // O shadowOffsetUVLoc
    GLint shadowSoftnessSDFLoc = glGetUniformLocation(shaderProgramID, "shadowSoftnessSDF");
    // === FIN: NUEVOS UNIFORMS PARA EL SHADER SDF "MÁS PRO" ===

    if (transformLoc == -1 || colorLoc == -1 || sdfTextureSamplerLoc == -1) {
        fprintf(stderr, "ERROR::RENDERER: No se pudieron encontrar uniformes base (transform, textColor, o sdfTexture). ShaderID: %u\n", shaderProgramID);
    }
     if (sdfEdgeValueLoc == -1 || smoothingFactorLoc == -1) {
        fprintf(stderr, "ADVERTENCIA::RENDERER: No se pudieron encontrar uniformes SDF (sdfEdgeValue, smoothingFactor). Los efectos SDF pueden no funcionar. ShaderID: %u\n", shaderProgramID);
    }
    
    glUniform1i(sdfTextureSamplerLoc, 0); 
    glActiveTexture(GL_TEXTURE0);  

    // === INICIO: Establecer valores para los nuevos uniforms SDF ===
    glUniform1f(sdfEdgeValueLoc, 0.5f); // Correcto para tu sdf_generator
    glUniform1f(smoothingFactorLoc, 0.7f); // Puedes experimentar con este valor (ej. 0.5 a 1.0 con fwidth)

    // Configuración de efectos (ejemplos, puedes hacer esto configurable)
    bool G_ENABLE_OUTLINE = false; // Cambia a true para probar contornos
    bool G_ENABLE_SHADOW = false;  // Cambia a true para probar sombras

    if (enableOutlineLoc != -1) glUniform1i(enableOutlineLoc, G_ENABLE_OUTLINE);
    if (G_ENABLE_OUTLINE) {
        if (outlineColorLoc != -1) glUniform3f(outlineColorLoc, 0.0f, 0.0f, 0.0f); // Contorno negro
        if (outlineWidthSDFLoc != -1) glUniform1f(outlineWidthSDFLoc, 0.03f); // Experimenta
        if (outlineEdgeOffsetLoc != -1) glUniform1f(outlineEdgeOffsetLoc, 0.01f); // Experimenta
    }

    if (enableShadowLoc != -1) glUniform1i(enableShadowLoc, G_ENABLE_SHADOW);
    if (G_ENABLE_SHADOW) {
        if (shadowColorLoc != -1) glUniform4f(shadowColorLoc, 0.0f, 0.0f, 0.0f, 0.5f); // Sombra negra semitransparente
        // Para shadowOffsetScreen, necesitarías calcularlo o pasar un offset UV
        // Ejemplo con un offset UV fijo pequeño (tendrías que renombrar el uniform en el shader a "shadowOffsetUV")
        // if (glGetUniformLocation(shaderProgramID, "shadowOffsetUV") != -1) glUniform2f(glGetUniformLocation(shaderProgramID, "shadowOffsetUV"), 0.002f, -0.002f);
        if (shadowSoftnessSDFLoc != -1) glUniform1f(shadowSoftnessSDFLoc, 0.1f); // Experimenta
    }
    // === FIN: Establecer valores para los nuevos uniforms SDF ===


    // --- Renderizado del Texto Principal ---
    float mainTextColor[3] = {0.8f, 0.9f, 0.2f}; 
    glUniform3fv(colorLoc, 1, mainTextColor); // colorLoc ahora es el textColor base

    const char* s_iter = text;
    float currentX = startX; 
    float currentY = startY; 
    size_t current_byte_render_offset = 0; 

    int char_count_on_line = 0;
    while (*s_iter != '\0') {
        const char* char_start_ptr_for_offset = s_iter; 
        FT_ULong current_codepoint = utf8_to_codepoint(&s_iter);
        size_t char_byte_length = s_iter - char_start_ptr_for_offset; 

        if (current_codepoint == 0) break;

        GlyphInfo loop_glyph_info = getGlyphInfo(current_codepoint);
        
        if (char_count_on_line > 0 && (currentX + (loop_glyph_info.advanceX * scale)) > (startX + maxLineWidth) ) {
            currentX = startX;
            currentY -= lineHeight; 
            char_count_on_line = 0;
        }

        if (!(layout.cursor_is_over_char && current_byte_render_offset == cursorBytePos)) {
            if (loop_glyph_info.sdfTextureID != 0 && loop_glyph_info.sdfTextureWidth > 0 && loop_glyph_info.sdfTextureHeight > 0) {
                glBindTexture(GL_TEXTURE_2D, loop_glyph_info.sdfTextureID);

                float quad_world_width = (float)loop_glyph_info.sdfTextureWidth * scale;
                float quad_world_height = (float)loop_glyph_info.sdfTextureHeight * scale;

                float actualPosX = currentX + ((float)loop_glyph_info.bitmap_left - sdf_padding) * scale;
                float actualPosY = currentY + ((float)loop_glyph_info.bitmap_top + sdf_padding) * scale - quad_world_height;
                
                GLfloat transformMatrix[16] = {
                    quad_world_width, 0.0f,            0.0f, 0.0f,
                    0.0f,             quad_world_height, 0.0f, 0.0f,
                    0.0f,             0.0f,            1.0f, 0.0f,
                    actualPosX,       actualPosY,      0.0f, 1.0f
                };
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        
        currentX += loop_glyph_info.advanceX * scale;
        char_count_on_line++;
        current_byte_render_offset += char_byte_length; 
    }
    
    // --- Renderizado del Cursor y Carácter Sobre el Cursor ---
    float cursorBackgroundColor[3] = {0.85f, 0.85f, 0.85f}; 
    float textOnCursorColor[3] = {0.1f, 0.1f, 0.1f};   

    float cursorPenX = layout.cursor_pos.x;
    float cursorPenY = layout.cursor_pos.y;

    GlyphInfo block_glyph_info = getGlyphInfo(0x2588); 

    if (block_glyph_info.sdfTextureID != 0 && block_glyph_info.sdfTextureWidth > 0 && block_glyph_info.sdfTextureHeight > 0) {
        // Para el fondo del cursor, podrías querer desactivar temporalmente efectos como el contorno o sombra,
        // o usar un color base simple para el "textColor" del bloque.
        // Aquí, simplemente cambiamos el color base.
        glUniform3fv(colorLoc, 1, cursorBackgroundColor); 
        // Si tienes efectos como contorno/sombra activos globalmente y no los quieres para el bloque del cursor:
        // if (enableOutlineLoc != -1) glUniform1i(enableOutlineLoc, false);
        // if (enableShadowLoc != -1) glUniform1i(enableShadowLoc, false);


        float quad_w_cursor_bg = (float)block_glyph_info.sdfTextureWidth * scale;
        float quad_h_cursor_bg = (float)block_glyph_info.sdfTextureHeight * scale;
        
        float actualPosX_cursor_bg = cursorPenX + ((float)block_glyph_info.bitmap_left - sdf_padding) * scale;
        float actualPosY_cursor_bg = cursorPenY + ((float)block_glyph_info.bitmap_top + sdf_padding) * scale - quad_h_cursor_bg;

        GLfloat cursorBgTransformMatrix[16] = {
            quad_w_cursor_bg, 0.0f,               0.0f, 0.0f,
            0.0f,             quad_h_cursor_bg,   0.0f, 0.0f,
            0.0f,             0.0f,               1.0f, 0.0f,
            actualPosX_cursor_bg, actualPosY_cursor_bg, 0.0f, 1.0f
        };
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, cursorBgTransformMatrix);
        
        glBindTexture(GL_TEXTURE_2D, block_glyph_info.sdfTextureID);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Restaurar configuración de efectos si la cambiaste para el bloque del cursor
        // if (enableOutlineLoc != -1) glUniform1i(enableOutlineLoc, G_ENABLE_OUTLINE);
        // if (enableShadowLoc != -1) glUniform1i(enableShadowLoc, G_ENABLE_SHADOW);
    }

    if (layout.cursor_is_over_char) {
        GlyphInfo char_on_cursor_info = getGlyphInfo(layout.codepoint_under_cursor);

        if (char_on_cursor_info.sdfTextureID != 0 && char_on_cursor_info.sdfTextureWidth > 0 && char_on_cursor_info.sdfTextureHeight > 0) {
            glUniform3fv(colorLoc, 1, textOnCursorColor); // colorLoc es el "textColor" base del shader

            float quad_w_char_on_cursor = (float)char_on_cursor_info.sdfTextureWidth * scale;
            float quad_h_char_on_cursor = (float)char_on_cursor_info.sdfTextureHeight * scale;
            
            float actualPosX_char_on_cursor = cursorPenX + ((float)char_on_cursor_info.bitmap_left - sdf_padding) * scale;
            float actualPosY_char_on_cursor = cursorPenY + ((float)char_on_cursor_info.bitmap_top + sdf_padding) * scale - quad_h_char_on_cursor;

            GLfloat charOnCursorTransformMatrix[16] = {
                quad_w_char_on_cursor, 0.0f,                     0.0f, 0.0f,
                0.0f,                  quad_h_char_on_cursor,    0.0f, 0.0f,
                0.0f,                  0.0f,                     1.0f, 0.0f,
                actualPosX_char_on_cursor, actualPosY_char_on_cursor, 0.0f, 1.0f
            };
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, charOnCursorTransformMatrix);

            glBindTexture(GL_TEXTURE_2D, char_on_cursor_info.sdfTextureID);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0); 
    glBindVertexArray(0);            
    checkOpenGLError("Before glutSwapBuffers");
    glutSwapBuffers();
    checkOpenGLError("After glutSwapBuffers");
#else
    (void)shaderProgramID; (void)text; (void)cursorBytePos;
#endif
}