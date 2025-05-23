#ifndef UTILS_H
#define UTILS_H

#include <ft2build.h>
#include FT_FREETYPE_H

FT_ULong utf8_to_codepoint(const char** s);
void checkOpenGLError(const char* stage_name);

#endif // UTILS_H