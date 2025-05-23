#include "renderer.h"
#include "opengl_setup.h"  // For globalQuadVAO
#include "glyph_manager.h" // For actual getGlyphInfo and GlyphInfo struct
#include "utils.h"         // Para utf8_to_codepoint
#include "text_layout.h"   // For TextLayoutInfo and calculateTextLayout signature
#include <stdio.h>
#include <GL/freeglut.h>
#include <string.h> // Para strlen

// Wrapper function to match GetGlyphMetricsFunc signature for real rendering
MinimalGlyphInfo getGlyphMetrics_wrapper(FT_ULong codepoint) {
#ifndef UNIT_TESTING
    GlyphInfo real_info = getGlyphInfo(codepoint);
    MinimalGlyphInfo min_info = {0};

    // Populate MinimalGlyphInfo from the real GlyphInfo
    min_info.vao = real_info.vao; // This will be 0 if SDF is used, as VAO is separate
    min_info.vbo = real_info.vbo; // Also 0
    min_info.ebo = real_info.ebo; // Also 0
    min_info.advanceX = real_info.advanceX; 
    min_info.indexCount = real_info.indexCount; // 0 if SDF (mesh rendering not used)
    
    min_info.codepoint = codepoint; 

    // For SDF rendering, we need texture info, which is not in MinimalGlyphInfo.
    // calculateTextLayout primarily uses advanceX, which is fine.
    // The rendering part will use the full GlyphInfo directly.

    return min_info;
#else
    fprintf(stderr, "UNIT_TESTING_WARNING: getGlyphMetrics_wrapper called in renderer_test context for codepoint %lu\n", codepoint);
    MinimalGlyphInfo dummy_info = {0};
    dummy_info.advanceX = 600; 
    dummy_info.codepoint = codepoint;
    return dummy_info;
#endif
}


TextLayoutInfo calculateTextLayout(
    const char* text,
    size_t cursorBytePos,
    float startX,
    float startY,
    float scale,
    float maxLineWidth,
    float lineHeight,
    GetGlyphMetricsFunc get_glyph_metrics) {

    TextLayoutInfo layout_info = {0};
    layout_info.cursor_pos.x = startX;
    layout_info.cursor_pos.y = startY;

    if (!text) {
        return layout_info;
    }

    const char* s_iter = text;
    size_t current_byte_iter_offset = 0;
    float simulatedCurrentX = startX;
    float simulatedCurrentY = startY;

    while (*s_iter != '\0') {
        const char* temp_s_ptr = s_iter;
        FT_ULong codepoint_for_check = utf8_to_codepoint(&temp_s_ptr);
        size_t char_byte_length = temp_s_ptr - s_iter;

        if (codepoint_for_check == 0) break;

        MinimalGlyphInfo glyph_info_minimal = get_glyph_metrics(codepoint_for_check);

        if (simulatedCurrentX + (glyph_info_minimal.advanceX * scale) > startX + maxLineWidth) {
            simulatedCurrentX = startX;
            simulatedCurrentY -= lineHeight;
        }

        if (current_byte_iter_offset == cursorBytePos) {
            layout_info.cursor_pos.x = simulatedCurrentX;
            layout_info.cursor_pos.y = simulatedCurrentY;
            layout_info.codepoint_under_cursor = codepoint_for_check;
            layout_info.glyph_info_under_cursor = glyph_info_minimal; // Still MinimalGlyphInfo here
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glBindVertexArray(globalQuadVAO); // Bind the global quad VAO once

    if (!text) {
        fprintf(stderr, "ERROR::RENDERER: El parámetro de texto es NULL.\n");
        glBindVertexArray(0);
        glutSwapBuffers();
        return;
    }

    float startX = -0.98f;
    float startY = 0.83f; // Top of the line
    const float maxLineWidth = 1.96f;
    const float lineHeight = 0.18f; // Distance to move down for new line
    float scale = 0.003f; // General scaling factor for advanceX
    float sdf_scale_factor = 0.1f; // Additional scale factor for SDF quad dimensions

    TextLayoutInfo layout = calculateTextLayout(text, cursorBytePos, startX, startY, scale, maxLineWidth, lineHeight, getGlyphMetrics_wrapper);
    
    // float cursorRenderX = layout.cursor_pos.x; // From layout
    // float cursorRenderY = layout.cursor_pos.y; // From layout
    // int cursor_is_over_char = layout.cursor_is_over_char;
    // FT_ULong codepoint_at_cursor = layout.codepoint_under_cursor;


    GLint transformLoc = glGetUniformLocation(shaderProgramID, "transform");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "textColor");
    GLint sdfTextureLoc = glGetUniformLocation(shaderProgramID, "sdfTexture");

    if (transformLoc == -1 || colorLoc == -1 || sdfTextureLoc == -1) {
        fprintf(stderr, "ERROR::RENDERER: No se pudieron encontrar uniformes. ShaderID: %u\n", shaderProgramID);
    }
    
    glUniform1i(sdfTextureLoc, 0); // Tell sampler to use texture unit 0
    glActiveTexture(GL_TEXTURE0); // Activate texture unit 0

    float mainTextColor[3] = {0.8f, 0.9f, 0.2f};
    // float cursorBackgroundColor[3] = {0.85f, 0.85f, 0.85f};
    // float textOnCursorColor[3] = {0.1f, 0.1f, 0.1f};

    glUniform3fv(colorLoc, 1, mainTextColor);

    const char* s_iter = text;
    // size_t current_byte_iter_offset = 0; // For cursor logic, if re-enabled
    float currentX = startX;
    float currentY = startY; 

    while (*s_iter != '\0') {
        const char* loop_char_start_ptr = s_iter;
        FT_ULong current_codepoint = utf8_to_codepoint(&s_iter);
        size_t char_byte_length = s_iter - loop_char_start_ptr;

        if (current_codepoint == 0) break;

        GlyphInfo loop_glyph_info = getGlyphInfo(current_codepoint);

        if (currentX + (loop_glyph_info.advanceX * scale) > startX + maxLineWidth) {
            currentX = startX;
            currentY -= lineHeight;
        }

        if (loop_glyph_info.sdfTextureID != 0 && loop_glyph_info.sdfTextureWidth > 0 && loop_glyph_info.sdfTextureHeight > 0) {
            glBindTexture(GL_TEXTURE_2D, loop_glyph_info.sdfTextureID);

            float charWidth = loop_glyph_info.sdfTextureWidth * scale * sdf_scale_factor;
            float charHeight = loop_glyph_info.sdfTextureHeight * scale * sdf_scale_factor;
            
            // This positioning needs to be based on FreeType metrics for accuracy.
            // FT_GlyphSlot slot = ftFace->glyph; (or ftEmojiFace->glyph)
            // float bearingX = slot->bitmap_left * scale * sdf_scale_factor;
            // float bearingY = slot->bitmap_top * scale * sdf_scale_factor;
            // float actualPosX = currentX + bearingX;
            // float actualPosY = currentY - (charHeight - bearingY); // Y is from top, convert to from bottom

            // Simplified positioning: currentX is left, currentY is top of line, quad draws downwards from currentY
            float actualPosX = currentX; // Placeholder: Use actual bearingX if available
            float actualPosY = currentY - charHeight; // Placeholder: Adjust with bearingY and baseline

            GLfloat transformMatrix[16] = {
                charWidth, 0.0f,      0.0f, 0.0f,
                0.0f,      charHeight,0.0f, 0.0f,
                0.0f,      0.0f,      1.0f, 0.0f,
                actualPosX, actualPosY, 0.0f, 1.0f
            };
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        currentX += loop_glyph_info.advanceX * scale;
        // current_byte_iter_offset += char_byte_length; // For cursor
    }
    
    // Main text rendering loop finishes above this line
    // Bindings are still active: globalQuadVAO, GL_TEXTURE0, shaderProgramID
    
    // --- Cursor Rendering (BUCLE 2 adapted for SDF) ---
    float cursorRenderX = layout.cursor_pos.x; 
    float cursorRenderY = layout.cursor_pos.y; 
    int cursor_is_over_char = layout.cursor_is_over_char;
    FT_ULong codepoint_at_cursor = layout.codepoint_under_cursor;

    float cursorBackgroundColor[3] = {0.85f, 0.85f, 0.85f}; // Define here as it was commented out
    float textOnCursorColor[3] = {0.1f, 0.1f, 0.1f};   // Define here
    
    GlyphInfo block_glyph_info = getGlyphInfo(0x2588); // '█'
    
    // Ensure globalQuadVAO is still bound (it should be from the start of renderText)
    // glBindVertexArray(globalQuadVAO); // Not strictly needed if already bound and not unbound by text loop
    // glActiveTexture(GL_TEXTURE0); // Also should still be active

    if (block_glyph_info.sdfTextureID != 0 && block_glyph_info.sdfTextureWidth > 0 && block_glyph_info.sdfTextureHeight > 0) {
        glUniform3fv(colorLoc, 1, cursorBackgroundColor);
        glBindTexture(GL_TEXTURE_2D, block_glyph_info.sdfTextureID);

        // Option 1: Use SDF texture dimensions for block size
        // float cursorQuadWidth = block_glyph_info.sdfTextureWidth * scale * sdf_scale_factor;
        // float cursorQuadHeight = block_glyph_info.sdfTextureHeight * scale * sdf_scale_factor;
        // Option 2: Use advanceX of the block character and lineHeight for a more uniform cursor
        float cursorQuadWidth = block_glyph_info.advanceX * scale; 
        float cursorQuadHeight = lineHeight; // Use the general line height for cursor block

        float cursorActualPosX = cursorRenderX;
        // Assuming cursorRenderY is the baseline, quad is rendered from bottom-left.
        // For SDF, quad is 0,0 to 1,1. currentY in main loop is top-left.
        // If cursorRenderY is top of line (like currentY in main loop):
        float cursorActualPosY = cursorRenderY - cursorQuadHeight; 

        GLfloat cursorBgTransformMatrix[16] = {
            cursorQuadWidth, 0.0f,             0.0f, 0.0f,
            0.0f,            cursorQuadHeight,  0.0f, 0.0f,
            0.0f,            0.0f,             1.0f, 0.0f,
            cursorActualPosX, cursorActualPosY,0.0f, 1.0f
        };
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, cursorBgTransformMatrix);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (cursor_is_over_char) {
            // Get full GlyphInfo for the character under the cursor to access SDF details
            GlyphInfo actual_char_on_cursor_info = getGlyphInfo(codepoint_at_cursor);
            if (actual_char_on_cursor_info.sdfTextureID != 0 && actual_char_on_cursor_info.sdfTextureWidth > 0 && actual_char_on_cursor_info.sdfTextureHeight > 0) {
                glUniform3fv(colorLoc, 1, textOnCursorColor);
                glBindTexture(GL_TEXTURE_2D, actual_char_on_cursor_info.sdfTextureID);

                // Scale the character quad based on its SDF texture dimensions
                float charOnCursorWidth = actual_char_on_cursor_info.sdfTextureWidth * scale * sdf_scale_factor;
                float charOnCursorHeight = actual_char_on_cursor_info.sdfTextureHeight * scale * sdf_scale_factor;
                
                // Position the character using the same top-left as the cursor block
                // but its own height. This might need fine-tuning based on font metrics.
                float charActualPosX = cursorActualPosX; // Align with cursor block start
                float charActualPosY = cursorActualPosY + (cursorQuadHeight - charOnCursorHeight) / 2.0f; // Center vertically in block (approx)

                GLfloat charOnCursorTransformMatrix[16] = {
                    charOnCursorWidth,  0.0f,               0.0f, 0.0f,
                    0.0f,               charOnCursorHeight, 0.0f, 0.0f,
                    0.0f,               0.0f,               1.0f, 0.0f,
                    charActualPosX,     charActualPosY,     0.0f, 1.0f
                };
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, charOnCursorTransformMatrix);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    }
    
    // Cleanup: Unbind VAO and Texture after all rendering (text and cursor) is done.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glutSwapBuffers();
#else
    (void)shaderProgramID;
    (void)text;
    (void)cursorBytePos;
#endif
}