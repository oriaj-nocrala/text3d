#ifndef OPENGL_SETUP_H
#define OPENGL_SETUP_H

#include <GL/glew.h>

int initOpenGL();
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);
void cleanupOpenGL();

extern GLuint shaderProgram;

#endif