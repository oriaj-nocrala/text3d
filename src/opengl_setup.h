#ifndef OPENGL_SETUP_H
#define OPENGL_SETUP_H

#include <GL/glew.h>

extern GLuint globalQuadVAO;
extern GLuint globalQuadVBO;

GLuint initOpenGL(); // Return type changed to GLuint
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);
void cleanupOpenGL(GLuint programID); // Parameter added

// extern GLuint shaderProgram; // Removed

#endif