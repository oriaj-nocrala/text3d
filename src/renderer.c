#include "renderer.h"
#include "glyph_manager.h"
#include "opengl_setup.h"
#include <stdio.h>
#include <GL/freeglut.h>  // Usaremos FreeGLUT

// const char* textToRender; // Removed global variable

// --- Función de Dibujo (Modificada para el glifo) ---
void renderText(GLuint shaderProgramID, const char* text) { // Parameters added
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID); // Use passed shaderProgramID

    // --- Texto a Dibujar ---
    // const char* text = textToRender; // Removed, use parameter 'text'
    if (!text) {
        fprintf(stderr, "ERROR::RENDERER: text parameter is NULL.\n");
        return;
    }
    float startX = -0.9f; // Posición X inicial (en coordenadas NDC -1 a 1)
    float startY = 0.0f; // Posición Y inicial
    float scale = 0.005f; // Escala general del texto (ajusta según tamaño de fuente y vista)

    float currentX = startX; // Cursor X actual

    GLint transformLoc = glGetUniformLocation(shaderProgramID, "transform"); // Use passed shaderProgramID
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "textColor"); // Use passed shaderProgramID
    glUniform3f(colorLoc, 0.8f, 0.9f, 0.2f); // Color ejemplo (amarillo claro)


    // --- Iterar sobre el Texto ---
    for (const char* p = text; *p; ++p) {
        unsigned char uc = (unsigned char)*p; // Use unsigned char for indexing

        // Asegurarse que el carácter está en el rango cacheado
        if (uc >= CACHE_SIZE) { // c < 0 check is redundant with unsigned char
            fprintf(stderr, "Advertencia: Carácter '%c' (ASCII %u) fuera del rango del caché.\n", *p, uc);
            continue; // Saltar este carácter
        }

        GlyphInfo info = getGlyphInfo((char)uc); // getGlyphInfo expects char, but it handles unsigned conversion

        // Si el carácter no tiene VAO (ej: espacio en blanco, error de generación), solo avanza
        if (info.vao == 0) {
            currentX += info.advanceX * scale; // Avanzar incluso si no se dibuja
            continue;
        }

        // Calcular Matriz de Transformación para este carácter
        GLfloat transformMatrix[16] = {
            scale, 0.0f,  0.0f, 0.0f,
            0.0f,  scale, 0.0f, 0.0f,
            0.0f,  0.0f,  1.0f, 0.0f,
            currentX, startY, 0.0f, 1.0f // Posición actual
        };

        // Enviar matriz al shader
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);

        // Dibujar el carácter cacheado
        glBindVertexArray(info.vao);
        glDrawElements(GL_TRIANGLES, info.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Avanzar el cursor X para el siguiente carácter
        currentX += info.advanceX * scale;
    }

    glutSwapBuffers();
}