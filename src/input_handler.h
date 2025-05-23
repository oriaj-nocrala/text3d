#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stddef.h> // For size_t

extern char globalTextInputBuffer[]; // No size specified
extern size_t globalCursorBytePos;

// Prototipos de las funciones de callback para GLUT
void app_keyboard_callback(unsigned char key, int x, int y);
void app_special_keyboard_callback(int key, int x, int y);

#endif // INPUT_HANDLER_H