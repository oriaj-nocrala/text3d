// src/keybindings.h
#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

// Códigos para teclas especiales que nuestra aplicación usará internamente.
// Estos son los valores que los tests estaban usando y que esperamos
// que app_special_keyboard_callback reciba y procese.

#define APP_KEY_BACKSPACE 8
#define APP_KEY_LEFT  100
#define APP_KEY_RIGHT 102
#define APP_KEY_HOME  104
#define APP_KEY_END   105
#define APP_KEY_DEL 127


// Podríamos añadir otros si los usamos explícitamente,
// pero Shift, Ctrl, Alt son más para glutGetModifiers o ya vienen
// procesados por glutKeyboardFunc.
// Para los códigos de teclas muertas, ASCII_DEL_KEY, etc.,
// estos ya están bien como #define dentro de input_handler.c
// porque son más específicos de la lógica interna de ese manejador.
// keybindings.h es más para los códigos que GLUT nos da para teclas "especiales".

#endif // KEYBINDINGS_H