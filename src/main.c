#include <stdio.h>      // Para printf, fprintf, stderr
#include <stdlib.h>     // Para malloc, realloc, free, exit
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

void display() {
    renderText();
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
    cleanupOpenGL();
    cleanupFreeType();
}

int main(int argc, char *argv[]){ // <--- Firma correcta

    const char* mainFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; // O la que uses
    const char* emojiFontPath = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf"; // EJEMPLO, cambia esta ruta o pon NULL


    if(argc == 2){
        if(strlen(argv[1]) > 0) {
            textToRender = argv[1];
        }
    } else {
        fprintf(stderr, "Argumentos incorrectos o no se proporcionó argumento.\n");
        return 1;
    }

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
    if (!initOpenGL()) return 1;
    if (!initFreeType()) return 1;
    if (!loadFonts(mainFontPath, emojiFontPath)) { // Llama a la nueva función
        fprintf(stderr, "Fallo al cargar fuentes.\n");
        cleanupFreeType(); // Limpiar lo que se haya inicializado
        return 1;
    }
    if (!initGlyphCache()) return 1; // Llena el caché

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