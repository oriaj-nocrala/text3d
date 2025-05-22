#include "opengl_setup.h"
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

GLuint shaderProgram;

// --- Inicialización OpenGL y Shaders ---
int initOpenGL() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error inicializando GLEW: %s\n", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    printf("Usando GLEW %s\n", glewGetString(GLEW_VERSION));

    // Cargar y compilar shaders (ahora usamos el nuestro)
    shaderProgram = createShaderProgram("./shaders/vertex_shader.glsl", "./shaders/fragment_shader.glsl"); // O usa el hardcodeado
    if (shaderProgram == 0) {
        fprintf(stderr, "Error al crear el programa de shaders.\n");
        exit(EXIT_FAILURE);
    }

    // Configuración General OpenGL
    //glEnable(GL_DEPTH_TEST); // Probablemente no necesario para texto 2D
    glEnable(GL_MULTISAMPLE); // Habilitar Antialiasing (MSAA)
    glEnable(GL_BLEND); // Habilitar blending para posible antialiasing de SDF en el futuro
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Fondo gris
    return 1;
}

// Función para leer el contenido completo de un archivo en una cadena
// ¡El llamador es responsable de liberar la memoria devuelta!
char* readFileToString(const char* filepath) {
    FILE* file = fopen(filepath, "rb"); // "rb" para modo binario, evita problemas con \r\n en Windows
    if (file == NULL) {
        fprintf(stderr, "ERROR::SHADER::FILE_NOT_FOUND: %s\n", filepath);
        return NULL;
    }

    // Buscar el final del archivo para determinar el tamaño
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET); // Volver al inicio del archivo

    if (length < 0) { // ftell puede devolver -1 en error
        fprintf(stderr, "ERROR::SHADER::COULD_NOT_GET_FILE_SIZE: %s\n", filepath);
        fclose(file);
        return NULL;
    }
    
    // Asignar memoria para el contenido + terminador nulo
    // Usar 'length + 1' para el terminador nulo.
    // Si length es 0 (archivo vacío), malloc(1) es válido.
    char* buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR::SHADER::MEMORY_ALLOCATION_FAILED for file: %s\n", filepath);
        fclose(file);
        return NULL;
    }

    // Leer el archivo en el búfer
    // fread devuelve el número de items leídos. Si es menor que 'length' (y no es EOF), hay un error.
    size_t itemsRead = fread(buffer, 1, length, file);
    if (itemsRead < (size_t)length) { // size_t para evitar warnings de comparación signed/unsigned
        // feof(file) o ferror(file) pueden dar más detalles
        fprintf(stderr, "ERROR::SHADER::FILE_READ_FAILED: %s. Read %zu of %ld bytes.\n", filepath, itemsRead, length);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0'; // Asegurar terminación nula

    fclose(file);
    return buffer;
}


// --- Función para Cargar y Compilar Shaders (Ejemplo Básico) ---
// Necesitarás adaptar esto para leer tus archivos .glsl
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    // Código para leer los archivos, compilar los shaders (glCompileShader),
    // crear un programa (glCreateProgram), adjuntar shaders (glAttachShader),
    // linkear (glLinkProgram) y verificar errores.
    // Si falla, devuelve 0. Si tiene éxito, devuelve el ID del programa.

    const char* vertexShaderSource;
    const char* fragmentShaderSource;
    vertexShaderSource = readFileToString(vertexPath);
    if (vertexShaderSource == NULL) {
        fprintf(stderr, "Error al leer el shader de vértices: %s\n", vertexPath);
        return 0;
    }
    fragmentShaderSource = readFileToString(fragmentPath);
    if (fragmentShaderSource == NULL) {
        fprintf(stderr, "Error al leer el shader de fragmentos: %s\n", fragmentPath);
        free((void*)vertexShaderSource); // Liberar memoria del shader de vértices
        return 0;
    }

    GLuint vertexShader, fragmentShader, shaderProgram;
    int success;
    char infoLog[512];

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
        return 0;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
        glDeleteShader(vertexShader); // Limpiar shader de vértice
        return 0;
    }

    shaderProgram = glCreateProgram();
    free((void*)vertexShaderSource); // Liberar memoria del shader de vértices
    free((void*)fragmentShaderSource); // Liberar memoria del shader de fragmentos
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Los shaders ya están linkeados, podemos borrarlos
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // --- Fin: Ejemplo Muy Básico Hardcodeado ---

    printf("Shaders compilados y linkeados correctamente.\n");
    return shaderProgram;
}

void cleanupOpenGL(){
    // Borrar shader (igual que antes)
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}