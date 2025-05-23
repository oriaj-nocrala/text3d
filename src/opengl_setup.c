#include "opengl_setup.h"
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

// Define global VAO and VBO
GLuint globalQuadVAO = 0;
GLuint globalQuadVBO = 0;

// Helper function to read shader files
static char* readFileToString(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::FILE_NOT_FOUND: %s\n", filepath);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length < 0) {
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::FILE_SIZE_ERROR: %s\n", filepath);
        fclose(file);
        return NULL;
    }
    char* buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::MEMORY_ALLOCATION_FAILED for %s\n", filepath);
        fclose(file);
        return NULL;
    }
    size_t itemsRead = fread(buffer, 1, length, file);
    if (itemsRead < (size_t)length) {
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::FILE_READ_FAILED: %s. Read %zu of %ld bytes.\n", filepath, itemsRead, length);
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

GLuint initOpenGL() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "ERROR::OPENGL_SETUP: Error inicializando GLEW: %s\n", glewGetErrorString(err));
        return 0;
    }
    printf("Usando GLEW %s\n", glewGetString(GLEW_VERSION));

    // Provide actual paths to your shaders
    // Create a "shaders" directory in your project root or adjust paths
    GLuint programID = createShaderProgram("./shaders/vertex_shader.glsl", "./shaders/fragment_shader.glsl");
    if (programID == 0) {
        fprintf(stderr, "ERROR::OPENGL_SETUP: Fallo al crear el programa de shaders.\n");
        return 0;
    }

    // Quad vertices: posX, posY, texX, texY
    float quadVertices[] = {
        // First triangle
        0.0f, 1.0f, 0.0f, 1.0f, // Top-left
        0.0f, 0.0f, 0.0f, 0.0f, // Bottom-left
        1.0f, 1.0f, 1.0f, 1.0f, // Top-right
        // Second triangle
        1.0f, 0.0f, 1.0f, 0.0f  // Bottom-right
    };

    glGenVertexArrays(1, &globalQuadVAO);
    glGenBuffers(1, &globalQuadVBO);
    glBindVertexArray(globalQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, globalQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute (location = 0 in new shader)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute (location = 1 in new shader)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_MULTISAMPLE); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 
    return programID;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    char* vertexShaderSource = readFileToString(vertexPath);
    char* fragmentShaderSource = readFileToString(fragmentPath);

    if (!vertexShaderSource) {
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::VERTEX_SOURCE_NULL from %s\n", vertexPath);
        free(fragmentShaderSource); // In case fragment loaded but vertex failed
        return 0;
    }
    if (!fragmentShaderSource) {
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::FRAGMENT_SOURCE_NULL from %s\n", fragmentPath);
        free(vertexShaderSource);
        return 0;
    }

    GLuint vertexShader, fragmentShader, shaderProgram;
    int success;
    char infoLog[512];

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar * const*)&vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return 0;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar * const*)&fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
        glDeleteShader(vertexShader);
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return 0;
    }
    
    free(vertexShaderSource); // Free source strings after compilation
    free(fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::OPENGL_SETUP::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    printf("Shaders compilados y linkeados correctamente desde %s y %s.\n", vertexPath, fragmentPath);
    return shaderProgram;
}

void cleanupOpenGL(GLuint programID) {
    if (programID != 0) {
        glDeleteProgram(programID);
    }
    if (globalQuadVAO != 0) {
        glDeleteVertexArrays(1, &globalQuadVAO);
    }
    if (globalQuadVBO != 0) {
        glDeleteBuffers(1, &globalQuadVBO);
    }
}