#include "renderer.h"
#include "glyph_manager.h"
#include "opengl_setup.h" // Aunque no es estrictamente necesario para renderer.c
#include "utils.h"        // Para utf8_to_codepoint
#include <stdio.h>
#include <GL/freeglut.h>  

void renderText(GLuint shaderProgramID, const char* text) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    if (!text) {
        fprintf(stderr, "ERROR::RENDERER: El parámetro de texto es NULL.\n");
        glutSwapBuffers(); // Intercambia buffers para evitar ventana no responsiva
        return;
    }

    float startX = -0.95f; 
    float startY = 0.0f;  
    float scale = 0.003f; // Escala ajustada, puede necesitar más afinación
    float currentX = startX;

    GLint transformLoc = glGetUniformLocation(shaderProgramID, "transform");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "textColor");
    
    if (transformLoc == -1 || colorLoc == -1) {
        fprintf(stderr, "ERROR::RENDERER: No se pudieron encontrar los uniformes 'transform' o 'textColor'. ShaderProgramID: %u\n", shaderProgramID);
    }
    
    // Color del texto principal
    float mainTextColor[3] = {0.8f, 0.9f, 0.2f}; // Amarillo claro
    glUniform3fv(colorLoc, 1, mainTextColor);

    const char* s = text;
    while (*s != '\0') {
        FT_ULong codepoint = utf8_to_codepoint(&s); // s es avanzado por la función

        if (codepoint == 0) { // Fin de cadena según la convención de utf8_to_codepoint
            break;
        }
        
        GlyphInfo info = getGlyphInfo(codepoint);

        if (info.vao != 0) { // Solo dibujar si el VAO es válido
            GLfloat transformMatrix[16] = {
                scale, 0.0f,  0.0f, 0.0f,
                0.0f,  scale, 0.0f, 0.0f,
                0.0f,  0.0f,  1.0f, 0.0f,
                currentX, startY, 0.0f, 1.0f
            };
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);
            glBindVertexArray(info.vao);
            glDrawElements(GL_TRIANGLES, info.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        // Siempre avanzar por el avance del glifo, incluso si no se dibuja (ej. espacio)
        currentX += info.advanceX * scale;
    }

    // --- Dibujar el Prompt/Cursor de Bloque ---
    // Usaremos el carácter U+2588 (Full Block '█') para un cursor de bloque.
    FT_ULong prompt_char_code = 0x2588; 
    GlyphInfo promptInfo = getGlyphInfo(prompt_char_code);

    if (promptInfo.vao != 0) {
        // Cambia el color para el cursor de bloque.
        // Asumiendo un fondo oscuro (glClearColor(0.1f, 0.1f, 0.1f, 1.0f)),
        // un cursor blanco o gris claro resaltará.
        float promptColor[3] = {0.85f, 0.85f, 0.85f}; // Gris claro para el cursor
        glUniform3fv(colorLoc, 1, promptColor);

        // La posición del prompt es el currentX final del texto.
        GLfloat promptTransformMatrix[16] = {
            scale, 0.0f,  0.0f, 0.0f,
            0.0f,  scale, 0.0f, 0.0f,
            0.0f,  0.0f,  1.0f, 0.0f,
            currentX, startY, 0.0f, 1.0f // Posición después del último carácter
        };

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, promptTransformMatrix);
        glBindVertexArray(promptInfo.vao);
        glDrawElements(GL_TRIANGLES, promptInfo.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Opcional: Restaurar el color original del texto si vas a dibujar más cosas después
        // glUniform3fv(colorLoc, 1, mainTextColor); 
    }

    glutSwapBuffers();
}