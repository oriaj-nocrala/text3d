// En text3d/src/renderer.c
#include "renderer.h"
#include "glyph_manager.h"
#include "utils.h" // Para utf8_to_codepoint
#include <stdio.h>
#include <GL/freeglut.h>
#include <string.h> // Para strlen

void renderText(GLuint shaderProgramID, const char* text, size_t cursorBytePos) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    if (!text) {
        fprintf(stderr, "ERROR::RENDERER: El parámetro de texto es NULL.\n");
        glutSwapBuffers();
        return;
    }

    float startX = -0.95f;
    float startY = 0.0f;
    float scale = 0.003f;
    float currentX = startX;

    GLint transformLoc = glGetUniformLocation(shaderProgramID, "transform");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "textColor");

    if (transformLoc == -1 || colorLoc == -1) {
        fprintf(stderr, "ERROR::RENDERER: No se pudieron encontrar los uniformes 'transform' o 'textColor'. ShaderProgramID: %u\n", shaderProgramID);
    }

    float mainTextColor[3] = {0.8f, 0.9f, 0.2f};       // Amarillo claro para el texto
    float cursorBackgroundColor[3] = {0.85f, 0.85f, 0.85f}; // Gris claro para el fondo del cursor
    float textOnCursorColor[3] = {0.1f, 0.1f, 0.1f};   // Color oscuro (como el fondo de la ventana) para el texto sobre el cursor

    // Información sobre el carácter que está debajo del cursor
    float cursorRenderX = startX; // Coordenada X donde se dibujará el cursor
    FT_ULong codepoint_under_cursor = 0;
    GlyphInfo glyph_info_under_cursor = {0}; // {0} inicializa vao, vbo, etc. a 0
    int cursor_is_over_char = 0;      // Flag: 1 si el cursor está sobre un carácter, 0 si está al final

    // --- BUCLE 1: Renderizar todo el texto y obtener información del carácter bajo el cursor ---
    const char* s_iter = text;
    size_t current_byte_iter_offset = 0;

    glUniform3fv(colorLoc, 1, mainTextColor); // Color para el texto normal

    while (*s_iter != '\0') {
        const char* loop_char_start_ptr = s_iter; // Guardar puntero al inicio del carácter actual
        FT_ULong current_codepoint = utf8_to_codepoint(&s_iter); // s_iter es avanzado
        size_t char_byte_length = s_iter - loop_char_start_ptr; // Longitud en bytes del carácter actual

        if (current_codepoint == 0) break;

        GlyphInfo loop_glyph_info = getGlyphInfo(current_codepoint);

        // Comprobar si el cursor está en la posición de este carácter
        if (current_byte_iter_offset == cursorBytePos) {
            cursorRenderX = currentX;
            codepoint_under_cursor = current_codepoint;
            glyph_info_under_cursor = loop_glyph_info; // Guardar info del glifo bajo el cursor
            cursor_is_over_char = 1;
            // No dibujamos este carácter ahora, lo haremos después con colores invertidos
        } else {
            // Dibujar el carácter normalmente
            if (loop_glyph_info.vao != 0) {
                GLfloat transformMatrix[16] = {
                    scale, 0.0f,  0.0f, 0.0f,
                    0.0f,  scale, 0.0f, 0.0f,
                    0.0f,  0.0f,  1.0f, 0.0f,
                    currentX, startY, 0.0f, 1.0f
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

    // Si el cursor está al final del texto (no sobre un carácter existente)
    if (!cursor_is_over_char && cursorBytePos == strlen(text)) {
        cursorRenderX = currentX;
    }

    // --- BUCLE 2: Dibujar el cursor y el carácter invertido si aplica ---
    FT_ULong block_glyph_code = 0x2588; // Usar el glifo de bloque '█' para el fondo del cursor
    GlyphInfo block_glyph_info = getGlyphInfo(block_glyph_code);

    if (block_glyph_info.vao != 0) {
        // 1. Dibujar el fondo del cursor (el bloque '█' con color de fondo del cursor)
        glUniform3fv(colorLoc, 1, cursorBackgroundColor);
        GLfloat cursorBgTransformMatrix[16] = {
            scale, 0.0f,  0.0f, 0.0f,
            0.0f,  scale, 0.0f, 0.0f,
            0.0f,  0.0f,  1.0f, 0.0f,
            cursorRenderX, startY, 0.0f, 1.0f
        };
        // Podríamos querer escalar el ancho del bloque '█' para que coincida con el avance
        // del carácter debajo del cursor, pero usar el ancho natural del bloque '█' es más simple
        // y común para cursores de bloque que no son "I-beam".
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, cursorBgTransformMatrix);
        glBindVertexArray(block_glyph_info.vao);
        glDrawElements(GL_TRIANGLES, block_glyph_info.indexCount, GL_UNSIGNED_INT, 0);
        // No es necesario desbindar VAO aquí si la siguiente operación usa otro o lo bindea.

        // 2. Si el cursor estaba sobre un carácter, dibujar ese carácter con color de texto invertido
        if (cursor_is_over_char && glyph_info_under_cursor.vao != 0) {
            glUniform3fv(colorLoc, 1, textOnCursorColor); // Color del texto sobre el cursor

            // Usar la misma matriz de transformación que el fondo del cursor,
            // ya que se posiciona en el inicio de la celda del carácter.
            GLfloat charOnCursorTransformMatrix[16] = {
                scale, 0.0f,  0.0f, 0.0f,
                0.0f,  scale, 0.0f, 0.0f,
                0.0f,  0.0f,  1.0f, 0.0f,
                cursorRenderX, startY, 0.0f, 1.0f
            };
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, charOnCursorTransformMatrix);
            glBindVertexArray(glyph_info_under_cursor.vao);
            glDrawElements(GL_TRIANGLES, glyph_info_under_cursor.indexCount, GL_UNSIGNED_INT, 0);
        }
    }
    glBindVertexArray(0); // Desbindar VAO al final

    glutSwapBuffers();
}