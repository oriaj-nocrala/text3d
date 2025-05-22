#include "renderer.h"
#include "glyph_manager.h"
#include "opengl_setup.h"
#include <stdio.h>
#include <GL/freeglut.h>  // Usaremos FreeGLUT
#include "utils.h" // Para utf8_to_codepoint

const char* textToRender;

// --- Función de Dibujo (Modificada para el glifo) ---
void renderText() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    if (textToRender == NULL) {
        // fprintf(stderr, "Advertencia: textToRender es NULL en renderText().\n");
        textToRender = ""; // Evitar crash, no renderizar nada o un mensaje de error.
    }

    // --- Texto a Dibujar ---
    // float startX = -0.9f; // Posición X inicial (en coordenadas NDC -1 a 1)
    // float startY = 0.0f;  // Posición Y inicial
    // float scale = 0.005f; // Escala general del texto

    // Ajusta estos valores para un mejor tamaño y posición inicial
    float startX = -0.95f;
    float startY = 0.0f;
    float scale = 0.003f; // Prueba con una escala más pequeña si el texto es muy grande


    float currentX = startX; // Cursor X actual

    GLint transformLoc = glGetUniformLocation(shaderProgram, "transform");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    
    if (transformLoc == -1 || colorLoc == -1) {
        fprintf(stderr, "Error: No se pudieron encontrar las ubicaciones de los uniformes 'transform' o 'textColor'.\n");
        // Podrías querer no renderizar si los uniformes no se encuentran.
    }
    glUniform3f(colorLoc, 0.9f, 0.9f, 0.2f); // Color ejemplo (amarillo claro)


    // --- Iterar sobre el Texto ---
    const char* s = textToRender;
    while (*s != '\0') { // Iterar hasta el final de la cadena
        FT_ULong codepoint = utf8_to_codepoint(&s); // s es avanzado por la función

        if (codepoint == 0) { // Si utf8_to_codepoint devuelve 0 para fin de cadena (según implementación)
            break;
        }
        if (codepoint == 0xFFFD) {
            // Opcional: Manejar explícitamente el carácter de reemplazo si se desea
            // fprintf(stderr, "Nota: Se encontró un carácter de reemplazo (U+FFFD) en la cadena.\n");
        }

        GlyphInfo info = getGlyphInfo(codepoint);

        // Solo intentar dibujar si el glifo tiene un VAO válido
        if (info.vao != 0) {
            // Calcular Matriz de Transformación para este carácter
            // Matriz de transformación simple para escala y posición
            // OpenGL usa matrices por columnas (column-major order)
            // GLfloat transformMatrix[16] = {
            //     scale, 0.0f,  0.0f, 0.0f,  // Columna 1: Escala X, Sesgo Y
            //     0.0f,  scale, 0.0f, 0.0f,  // Columna 2: Sesgo X, Escala Y
            //     0.0f,  0.0f,  1.0f, 0.0f,  // Columna 3: Escala Z (1 para 2D)
            //     currentX + (info.bearingX * scale), // Columna 4: Posición X (con bearing)
            //     startY + (info.bearingY * scale) - (info.height * scale), // Posición Y (con bearing y ajuste de altura)
            //                    0.0f, 1.0f   // Profundidad Z, Homogénea W
            // };
            // // Nota: El cálculo de la posición Y puede necesitar ajustes dependiendo
            // de cómo se definan las coordenadas de tus glifos (línea base, etc.)
            // Y si las métricas como bearingY y height están disponibles en GlyphInfo.
            // Si no tienes bearingY o height en GlyphInfo, simplifica:
            GLfloat transformMatrix[16] = {
                scale, 0.0f,  0.0f, 0.0f,
                0.0f,  scale, 0.0f, 0.0f,
                0.0f,  0.0f,  1.0f, 0.0f,
                currentX, startY, 0.0f, 1.0f
            };


            // Enviar matriz al shader
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transformMatrix);

            // Dibujar el carácter cacheado
            glBindVertexArray(info.vao);
            glDrawElements(GL_TRIANGLES, info.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        } 
        // else {
        //     // Si vao es 0, el glifo no tiene geometría para dibujar (ej. espacio, tab, o error de carga)
        //     // No se dibuja nada, pero se avanza el cursor.
        //     if (codepoint != ' ') { // No imprimir advertencia para espacios
        //          // fprintf(stderr, "Advertencia: VAO es 0 para el carácter U+%04lX. No se dibujará.\n", codepoint);
        //     }
        // }

        // Avanzar el cursor X para el siguiente carácter, usando el avance del glifo actual
        currentX += info.advanceX * scale;
    }

    // --- Dibujar el Prompt/Cursor de Bloque ---
    // Usaremos el carácter U+2588 (Full Block '█') para un cursor de bloque.
    // Otros candidatos podrían ser U+258C (▌ Left Half Block) o U+2590 (▐ Right Half Block)
    // si U+2588 parece demasiado ancho.
    FT_ULong prompt_char_code = 0x2588; 
    GlyphInfo promptInfo = getGlyphInfo(prompt_char_code);

    if (promptInfo.vao != 0) {
        // Cambiar el color para el cursor de bloque.
        // Asumiendo un fondo oscuro (glClearColor(0.1f, 0.1f, 0.1f, 1.0f)),
        // un cursor blanco o gris claro resaltará.
        glUniform3f(colorLoc, 0.85f, 0.85f, 0.85f); // Gris claro para el cursor

        // La posición del prompt es el currentX final del texto.
        // El carácter '█' generalmente se asienta en la línea base y se extiende hacia arriba.
        GLfloat promptTransformMatrix[16] = {
            scale, 0.0f,  0.0f, 0.0f,
            0.0f,  scale, 0.0f, 0.0f,
            0.0f,  0.0f,  1.0f, 0.0f,
            currentX, startY, 0.0f, 1.0f
        };

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, promptTransformMatrix);
        glBindVertexArray(promptInfo.vao);
        glDrawElements(GL_TRIANGLES, promptInfo.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Restaurar el color original del texto para el siguiente frame o si hubiera más elementos.
        glUniform3f(colorLoc, 0.9f, 0.9f, 0.2f); // Amarillo claro
    }

    glutSwapBuffers();
}