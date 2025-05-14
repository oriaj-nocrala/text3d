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
    shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl"); // O usa el hardcodeado
    if (shaderProgram == 0) {
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


// --- Función para Cargar y Compilar Shaders (Ejemplo Básico) ---
// Necesitarás adaptar esto para leer tus archivos .glsl
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    // Código para leer los archivos, compilar los shaders (glCompileShader),
    // crear un programa (glCreateProgram), adjuntar shaders (glAttachShader),
    // linkear (glLinkProgram) y verificar errores.
    // Si falla, devuelve 0. Si tiene éxito, devuelve el ID del programa.

    // --- Inicio: Ejemplo Muy Básico Hardcodeado ---
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 transform;\n" // Matriz para escalar/posicionar
        "void main() {\n"
        "   gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "}\0";
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 textColor;\n"
        "void main() {\n"
        "   FragColor = vec4(textColor, 1.0);\n"
        "}\0";

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