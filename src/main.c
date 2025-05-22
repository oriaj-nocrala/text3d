#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#ifdef UNIT_TESTING
// If compiling for unit tests, and GLUT_KEY_BACKSPACE is not defined by freeglut.h
// (which might happen if headers are not found correctly despite CFLAGS),
// define it manually. freeglut_std.h is where it's usually defined.
// We ensure main_module.o can compile. The test itself uses 8.
#ifndef GLUT_KEY_BACKSPACE
#define GLUT_KEY_BACKSPACE 8
#endif
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "renderer.h"
#include "glyph_manager.h"
#include "opengl_setup.h"
#include "freetype_handler.h"
#include "input_handler.h"    // << NUEVO INCLUDE

// --- Variables Globales ---
GLuint globalShaderProgramID = 0;
// Valores predeterminados
// const char* globalTextToRender = "Texto ¡Hola €!"; // Original
char globalTextInputBuffer[256] = "Texto ¡Hola €!"; // Modifiable buffer
const char* globalTextToRender = globalTextInputBuffer; // Point to the buffer
size_t globalCursorBytePos = 0; // Posición del cursor en BYTES dentro del buffer

const char* globalMainFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; // Fuente principal por defecto
const char* globalEmojiFontPath = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf"; // Fuente de emoji por defecto (opcional)

// --- Funciones de GLUT ---
void display() {
    renderText(globalShaderProgramID, globalTextToRender, globalCursorBytePos);
}

void reshape(int width, int height) {
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
}

void idle() {
    // glutPostRedisplay(); // Para animación o redibujado continuo
}

void cleanup() {
    printf("Limpiando...\n");
    cleanupGlyphCache();
    if (globalShaderProgramID != 0) {
        cleanupOpenGL(globalShaderProgramID);
    }
    cleanupFreeType();
    printf("Limpieza finalizada.\n");
}

// --- Función Principal ---
#ifndef UNIT_TESTING // Exclude main function when compiling for unit tests
int main(int argc, char *argv[]){
    // Inicializa las variables que se usarán con los valores globales predeterminados
    const char* textToRender = globalTextToRender;
    const char* mainFontPath = globalMainFontPath;
    const char* emojiFontPath = globalEmojiFontPath;

    // --- Análisis de Argumentos ---
    // Uso esperado: ./programa ["texto"] [ruta_fuente_principal] [ruta_fuente_emoji]
    if (argc >= 2) { // Al menos se proporciona el texto
        if (strlen(argv[1]) > 0) {
            textToRender = argv[1];
        } else {
            fprintf(stderr, "ADVERTENCIA::MAIN: El argumento de texto está vacío. Usando texto por defecto.\n");
        }
    } else {
        fprintf(stderr, "INFO::MAIN: No se proporcionó argumento de texto. Usando texto por defecto: \"%s\"\n", textToRender);
    }

    if (argc >= 3) { // Se proporciona la fuente principal
        if (strlen(argv[2]) > 0) {
            mainFontPath = argv[2];
        } else {
             fprintf(stderr, "ADVERTENCIA::MAIN: La ruta de la fuente principal está vacía. Usando fuente por defecto: \"%s\"\n", mainFontPath);
        }
    } else if (argc > 1) { // Solo se dio texto, no fuentes
         fprintf(stderr, "INFO::MAIN: No se proporcionó ruta para la fuente principal. Usando fuente por defecto: \"%s\"\n", mainFontPath);
    }


    if (argc >= 4) { // Se proporciona la fuente de emoji
        if (strlen(argv[3]) > 0) {
            emojiFontPath = argv[3];
        } else {
            fprintf(stderr, "ADVERTENCIA::MAIN: La ruta de la fuente de emoji está vacía. No se usará fuente de emoji de fallback o se usará la predeterminada si está configurada.\n");
            // Si quieres desactivar el emoji por completo si la ruta es vacía:
            // emojiFontPath = NULL; 
        }
    } else if (argc > 1) { // No se dio fuente de emoji
         fprintf(stderr, "INFO::MAIN: No se proporcionó ruta para la fuente de emoji. Usando fuente de emoji por defecto (si está configurada): \"%s\"\n", emojiFontPath ? emojiFontPath : "Ninguna");
    }
    
    // Asigna el texto final a la variable global que usa display()
    // globalTextToRender = textToRender; // Original assignment
    // Instead, copy to the buffer if different from default
    if (strcmp(textToRender, globalTextInputBuffer) != 0) {
        strncpy(globalTextInputBuffer, textToRender, sizeof(globalTextInputBuffer) - 1);
        globalTextInputBuffer[sizeof(globalTextInputBuffer) - 1] = '\0';
    }
    globalCursorBytePos = strlen(globalTextInputBuffer); // Inicializar cursor al final

    // --- Inicialización de GLUT y OpenGL ---
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("FreeType + libtess2 + OpenGL (UTF-8)");

    globalShaderProgramID = initOpenGL();
    if (globalShaderProgramID == 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al inicializar OpenGL. Saliendo.\n");
        return 1;
    }

    // --- Inicialización de FreeType y Carga de Fuentes ---
    if (initFreeType() != 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al inicializar FreeType. Saliendo.\n");
        cleanupOpenGL(globalShaderProgramID);
        return 1;
    }

    printf("INFO::MAIN: Cargando fuente principal desde: \"%s\"\n", mainFontPath);
    if (emojiFontPath && strlen(emojiFontPath) > 0) {
        printf("INFO::MAIN: Cargando fuente de emoji desde: \"%s\"\n", emojiFontPath);
    } else {
        printf("INFO::MAIN: No se cargará fuente de emoji.\n");
    }

    if (loadFonts(mainFontPath, (emojiFontPath && strlen(emojiFontPath) > 0) ? emojiFontPath : NULL) != 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al cargar fuentes. Asegúrate que las rutas son correctas y las fuentes son válidas.\n");
        fprintf(stderr, "Ruta principal intentada: %s\n", mainFontPath);
        if (emojiFontPath && strlen(emojiFontPath) > 0) fprintf(stderr, "Ruta emoji intentada: %s\n", emojiFontPath);
        cleanupFreeType();
        cleanupOpenGL(globalShaderProgramID);
        return 1;
    }

    // --- Inicialización del Caché de Glifos ---
    if (initGlyphCache() != 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al inicializar el caché de glifos. Saliendo.\n");
        cleanupFreeType();
        cleanupOpenGL(globalShaderProgramID);
        return 1;
    }

    // --- Registrar Callbacks y Bucle Principal ---
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutCloseFunc(cleanup);

    // Usar los callbacks del nuevo módulo input_handler
    glutKeyboardFunc(app_keyboard_callback);
    glutSpecialFunc(app_special_keyboard_callback);

    printf("Iniciando bucle principal de GLUT...\nMostrando texto: \"%s\"\n", globalTextToRender);
    glutMainLoop();

    return 0;
}
#endif // UNIT_TESTING