// En text3d/src/renderer.c
// En text3d/src/renderer.c
#include "renderer.h"
#include "glyph_manager.h" // For actual getGlyphInfo and GlyphInfo struct
#include "utils.h"         // Para utf8_to_codepoint
#include "text_layout.h"   // For TextLayoutInfo and calculateTextLayout signature
#include <stdio.h>
#include <GL/freeglut.h>
#include <string.h> // Para strlen

// Wrapper function to match GetGlyphMetricsFunc signature for real rendering
MinimalGlyphInfo getGlyphMetrics_wrapper(FT_ULong codepoint) {
    GlyphInfo real_info = getGlyphInfo(codepoint);
    MinimalGlyphInfo min_info = {0};
    min_info.advanceX = real_info.advanceX;
    // Copy other fields if needed by calculateTextLayout or if MinimalGlyphInfo evolves
    min_info.vao = real_info.vao;
    min_info.vbo = real_info.vbo;
    min_info.ebo = real_info.ebo;
    min_info.textureID = real_info.textureID;
    min_info.width = real_info.width;
    min_info.height = real_info.height;
    min_info.bearingX = real_info.bearingX;
    min_info.bearingY = real_info.bearingY;
    min_info.advanceY = real_info.advanceY;
    min_info.indexCount = real_info.indexCount;
    min_info.codepoint = real_info.codepoint;
    return min_info;
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
        return layout_info; // Return default positions if text is NULL
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

        MinimalGlyphInfo glyph_info = get_glyph_metrics(codepoint_for_check);

        if (simulatedCurrentX + (glyph_info.advanceX * scale) > startX + maxLineWidth) {
            simulatedCurrentX = startX;
            simulatedCurrentY -= lineHeight;
        }

        if (current_byte_iter_offset == cursorBytePos) {
            layout_info.cursor_pos.x = simulatedCurrentX;
            layout_info.cursor_pos.y = simulatedCurrentY;
            layout_info.codepoint_under_cursor = codepoint_for_check;
            layout_info.glyph_info_under_cursor = glyph_info;
            layout_info.cursor_is_over_char = 1;
            // No break here, continue to calculate full text length for end cursor pos if needed
        }
        simulatedCurrentX += glyph_info.advanceX * scale;
        s_iter += char_byte_length;
        current_byte_iter_offset += char_byte_length;
    }

    // If cursor is at the end of the text
    if (!layout_info.cursor_is_over_char && cursorBytePos == current_byte_iter_offset) {
        layout_info.cursor_pos.x = simulatedCurrentX;
        layout_info.cursor_pos.y = simulatedCurrentY;
    }
    return layout_info;
}


void renderText(GLuint shaderProgramID, const char* text, size_t cursorBytePos) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    if (!text) {
        fprintf(stderr, "ERROR::RENDERER: El parámetro de texto es NULL.\n");
        glutSwapBuffers();
        return;
    }

    // --- Configuration ---
    float startX = -0.98f;
    float startY = 0.95f;
    const float maxLineWidth = 1.96f;
    const float lineHeight = 0.18f;
    float scale = 0.003f;

    // --- Calculate Layout ---
    // Use the real getGlyphInfo via wrapper for rendering
    TextLayoutInfo layout = calculateTextLayout(text, cursorBytePos, startX, startY, scale, maxLineWidth, lineHeight, getGlyphMetrics_wrapper);
    
    float cursorRenderX = layout.cursor_pos.x;
    float cursorRenderY = layout.cursor_pos.y;
    FT_ULong codepoint_under_cursor = layout.codepoint_under_cursor;
    MinimalGlyphInfo glyph_info_minimal_under_cursor = layout.glyph_info_under_cursor;
    // Convert MinimalGlyphInfo to GlyphInfo if needed for rendering character under cursor, or adjust rendering part
    // For now, assuming MinimalGlyphInfo fields are sufficient or identical for rendering
    GlyphInfo glyph_info_under_cursor = {0}; // Reconstruct or map if necessary
    if (layout.cursor_is_over_char) {
        // This is a bit of a hack; ideally GlyphInfo and MinimalGlyphInfo would be more aligned
        // or calculateTextLayout would return the full GlyphInfo if possible (but that couples it more with FreeType details for tests)
        glyph_info_under_cursor.vao = glyph_info_minimal_under_cursor.vao;
        glyph_info_under_cursor.indexCount = glyph_info_minimal_under_cursor.indexCount;
        // ... copy other necessary fields ...
    }

    int cursor_is_over_char = layout.cursor_is_over_char;


    GLint transformLoc = glGetUniformLocation(shaderProgramID, "transform");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "textColor");

    if (transformLoc == -1 || colorLoc == -1) {
        fprintf(stderr, "ERROR::RENDERER: No se pudieron encontrar los uniformes 'transform' o 'textColor'. ShaderProgramID: %u\n", shaderProgramID);
    }

    float mainTextColor[3] = {0.8f, 0.9f, 0.2f};
    float cursorBackgroundColor[3] = {0.85f, 0.85f, 0.85f};
    float textOnCursorColor[3] = {0.1f, 0.1f, 0.1f};

    glUniform3fv(colorLoc, 1, mainTextColor);

    // --- Actual Rendering Loop ---
    const char* s_iter = text;
    size_t current_byte_iter_offset = 0;
    float currentX = startX;
    float currentY = startY;

    while (*s_iter != '\0') {
        const char* loop_char_start_ptr = s_iter;
        FT_ULong current_codepoint = utf8_to_codepoint(&s_iter);
        size_t char_byte_length = s_iter - loop_char_start_ptr;

        if (current_codepoint == 0) break;

        // Use the real getGlyphInfo for rendering glyphs
        GlyphInfo loop_glyph_info = getGlyphInfo(current_codepoint);

        if (currentX + (loop_glyph_info.advanceX * scale) > startX + maxLineWidth) {
            currentX = startX;
            currentY -= lineHeight;
        }

        if (!(cursor_is_over_char && current_byte_iter_offset == cursorBytePos)) {
            if (loop_glyph_info.vao != 0) {
                GLfloat transformMatrix[16] = {
                    scale, 0.0f,  0.0f, 0.0f,
                    0.0f,  scale, 0.0f, 0.0f,
                    0.0f,  0.0f,  1.0f, 0.0f,
                    currentX, currentY, 0.0f, 1.0f
                };
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);
                glBindVertexArray(loop_glyph_info.vao);
                glDrawElements(GL_TRIANGLES, loop_glyph_info.indexCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
        currentX += loop_glyph_info.advanceX * scale;
        current_byte_iter_offset += char_byte_length;
    }

    // --- BUCLE 2: Dibujar el cursor y el carácter invertido si aplica ---
    // Use real getGlyphInfo for block glyph
    GlyphInfo block_glyph_info = getGlyphInfo(0x2588); // '█'

    if (block_glyph_info.vao != 0) {
        glUniform3fv(colorLoc, 1, cursorBackgroundColor);
        GLfloat cursorBgTransformMatrix[16] = {
            scale, 0.0f,  0.0f, 0.0f,
            0.0f,  scale, 0.0f, 0.0f,
            0.0f,  0.0f,  1.0f, 0.0f,
            cursorRenderX, cursorRenderY, 0.0f, 1.0f
        };
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, cursorBgTransformMatrix);
        glBindVertexArray(block_glyph_info.vao);
        glDrawElements(GL_TRIANGLES, block_glyph_info.indexCount, GL_UNSIGNED_INT, 0);

        if (cursor_is_over_char && glyph_info_under_cursor.vao != 0) { // Check VAO of the actual GlyphInfo for char under cursor
            glUniform3fv(colorLoc, 1, textOnCursorColor);
             GLfloat charOnCursorTransformMatrix[16] = {
                scale, 0.0f,  0.0f, 0.0f,
                0.0f,  scale, 0.0f, 0.0f,
                0.0f,  0.0f,  1.0f, 0.0f,
                cursorRenderX, cursorRenderY, 0.0f, 1.0f
            };
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, charOnCursorTransformMatrix);
            glBindVertexArray(glyph_info_under_cursor.vao); // Use the VAO from the (potentially reconstructed) GlyphInfo
            glDrawElements(GL_TRIANGLES, glyph_info_under_cursor.indexCount, GL_UNSIGNED_INT, 0);
        }
    }
    glBindVertexArray(0);

    glutSwapBuffers();
}