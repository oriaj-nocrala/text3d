#include "utils.h"
#include <GL/glu.h>

// Implementación del decodificador UTF-8
FT_ULong utf8_to_codepoint(const char** s_ptr) {
    if (!s_ptr || !(*s_ptr)) {
        // Manejar puntero nulo, aunque el bucle de llamada debería prevenirlo
        return 0xFFFD; // Carácter de reemplazo
    }

    const unsigned char* s = (const unsigned char*)*s_ptr;
    unsigned char c1 = *s;
    FT_ULong codepoint = 0xFFFD; // Carácter de reemplazo por defecto

    if (c1 == 0) { // Terminador nulo
        return 0; // O un valor especial para indicar fin de cadena
    }

    if (c1 < 0x80) { // 0xxxxxxx (ASCII, 1 byte)
        codepoint = c1;
        *s_ptr = (const char*)(s + 1);
    } else if ((c1 & 0xE0) == 0xC0) { // 110xxxxx 10xxxxxx (2 bytes)
        if (*(s + 1) == '\0' || (*(s + 1) & 0xC0) != 0x80) { goto error; }
        codepoint = ((FT_ULong)(c1 & 0x1F) << 6) |
                    ((FT_ULong)(*(s + 1) & 0x3F));
        *s_ptr = (const char*)(s + 2);
    } else if ((c1 & 0xF0) == 0xE0) { // 1110xxxx 10xxxxxx 10xxxxxx (3 bytes)
        if (*(s + 1) == '\0' || (*(s + 1) & 0xC0) != 0x80 ||
            *(s + 2) == '\0' || (*(s + 2) & 0xC0) != 0x80) { goto error; }
        codepoint = ((FT_ULong)(c1 & 0x0F) << 12) |
                    ((FT_ULong)(*(s + 1) & 0x3F) << 6) |
                    ((FT_ULong)(*(s + 2) & 0x3F));
        *s_ptr = (const char*)(s + 3);
    } else if ((c1 & 0xF8) == 0xF0) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (4 bytes)
        if (*(s + 1) == '\0' || (*(s + 1) & 0xC0) != 0x80 ||
            *(s + 2) == '\0' || (*(s + 2) & 0xC0) != 0x80 ||
            *(s + 3) == '\0' || (*(s + 3) & 0xC0) != 0x80) { goto error; }
        codepoint = ((FT_ULong)(c1 & 0x07) << 18) |
                    ((FT_ULong)(*(s + 1) & 0x3F) << 12) |
                    ((FT_ULong)(*(s + 2) & 0x3F) << 6) |
                    ((FT_ULong)(*(s + 3) & 0x3F));
        *s_ptr = (const char*)(s + 4);
    } else { // Secuencia UTF-8 inválida
    error:
        // fprintf(stderr, "Error: Secuencia UTF-8 inválida comenzando con 0x%02X\n", c1);
        codepoint = 0xFFFD; // Carácter de reemplazo
        *s_ptr = (const char*)(s + 1); // Avanzar un byte para intentar recuperarse
    }
    return codepoint;
}

void checkOpenGLError(const char* stage_name) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error AT [%s]: %u", stage_name, err);
        // const GLubyte* errString = gluErrorString(err); // Descomenta si enlazas con GLU
        // if (errString) {
        //     fprintf(stderr, " (%s)", errString);
        // }
        fprintf(stderr, "\n");
    }
}