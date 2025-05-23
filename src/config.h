#ifndef CONFIG_H
#define CONFIG_H

#ifdef UNIT_TESTING
#define APP_TEXT_BUFFER_SIZE 256
#else
#define APP_TEXT_BUFFER_SIZE 1024
#endif

#endif // CONFIG_H
