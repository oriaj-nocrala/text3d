#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h> // For GLuint
#include <stddef.h>  // For size_t

// Modificado para aceptar la posición del cursor
void renderText(GLuint shaderProgramID, const char* text, size_t cursorBytePos);

#endif