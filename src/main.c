#include <stdio.h>      // Para printf, fprintf, stderr
#include <stdlib.h>     // Para malloc, realloc, free, exit
#include <string.h>     // Para strlen
#include <math.h>       // Para cos, sin, etc. (si usas matrices)
#include <GL/glew.h>      // Incluir GLEW ANTES de GLUT
#include <GL/freeglut.h>  // Usaremos FreeGLUT
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "tesselator.h" // <--- ¡IMPORTANTE! Incluir cabecera de libtess2

#include "renderer.h"
#include "glyph_manager.h"
#include "opengl_setup.h"
#include "freetype_handler.h"

// Global variables for shader program ID and text to render
// These will be initialized in main() and used by callbacks.
GLuint globalShaderProgramID = 0;
const char* globalTextToRender = NULL; // Renamed to avoid conflict with local textToRender in main

void display() {
    // Pass the global shader program ID and text to renderText
    renderText(globalShaderProgramID, globalTextToRender);
}

// --- Función Reshape (Ejemplo básico) ---
void reshape(int width, int height) {
    if (height == 0) height = 1; // Evitar división por cero
    glViewport(0, 0, width, height);
    // Aquí podrías recalcular una matriz de proyección si la usaras
    // Para la matriz simple en display(), no es estrictamente necesario
    // Pero si usas glm_ortho o glm_perspective, actualízala aquí.
    // printf("Ventana redimensionada a: %d x %d\n", width, height);
}

// --- Función Idle (Ejemplo básico) ---
void idle() {
    // Aquí podrías poner lógica de animación o actualizaciones
    // glutPostRedisplay(); // Llama a display() de nuevo si algo cambió
}


// --- Limpieza ---
void cleanup() {
    printf("Limpiando...\n");

    cleanupGlyphCache();
    if (globalShaderProgramID != 0) { // Only cleanup if successfully initialized
        cleanupOpenGL(globalShaderProgramID);
    }
    cleanupFreeType();
}

int main(int argc, char *argv[]){ // <--- Firma correcta
    const char* textToRenderFromArg = "Default Text"; // Default text

    if(argc == 2){
        if(strlen(argv[1]) > 0) {
            textToRenderFromArg = argv[1];
        } else {
            fprintf(stderr, "ADVERTENCIA::MAIN: Argumento de texto proporcionado está vacío. Usando texto por defecto.\n");
        }
    } else if (argc > 2) {
        fprintf(stderr, "ADVERTENCIA::MAIN: Demasiados argumentos. Se esperaba un solo argumento de texto. Usando el primero.\n");
        textToRenderFromArg = argv[1];
    } else {
        fprintf(stderr, "INFO::MAIN: No se proporcionó argumento de texto. Usando texto por defecto.\n");
    }
    globalTextToRender = textToRenderFromArg; // Assign to the global variable

    // Inicializar FreeGLUT
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("FreeType + libtess2 + OpenGL");

    // Inicializar GLEW y nuestro setup (Shaders, estado GL)
    // ¡Debe hacerse DESPUÉS de crear la ventana y el contexto!
    globalShaderProgramID = initOpenGL(); // Assign to global
    if (globalShaderProgramID == 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al inicializar OpenGL (shaderProgramID es 0). Saliendo.\n");
        // No llamar a cleanupOpenGL aquí ya que falló la inicialización del shader.
        return 1;
    }
    if (initFreeType() != 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al inicializar FreeType. Saliendo.\n");
        cleanupOpenGL(globalShaderProgramID); // OpenGL se inicializó, así que se puede limpiar.
        return 1;
    }
    // Usar una ruta de fuente definida, o mejor aún, configurable.
    const char* fontPath = "/usr/share/fonts/TTF/DejaVuSans.ttf"; 
    if (loadFont(fontPath) != 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al cargar la fuente '%s'. Saliendo.\n", fontPath);
        cleanupFreeType();
        cleanupOpenGL(globalShaderProgramID);
        return 1;
    }
    if (initGlyphCache() != 0) {
        fprintf(stderr, "ERROR::MAIN: Fallo al inicializar el caché de glifos. Saliendo.\n");
        cleanupFreeType();
        cleanupOpenGL(globalShaderProgramID);
        return 1;
    }

    // Registrar Callbacks de GLUT
    glutDisplayFunc(display);
    glutReshapeFunc(reshape); // <--- Añadida función reshape
    glutIdleFunc(idle);       // <--- Añadida función idle
    glutCloseFunc(cleanup);

    printf("Iniciando bucle principal de GLUT...\n");
    glutMainLoop(); // Iniciar bucle de eventos

    // cleanup() se llama automáticamente al cerrar la ventana via glutCloseFunc

    return 0; // No debería alcanzarse normalmente
}