# Define el compilador
CC = gcc

# Define directorios
SRC_DIR = src
BUILD_DIR = build
EXTERNAL_DIR = external
TESS_INC = $(EXTERNAL_DIR)/libtess2-1.0.2/Include
TESS_LIB = $(EXTERNAL_DIR)/libtess2-1.0.2/lib

# Define flags de compilación
# -I$(SRC_DIR) : Busca headers en tu directorio src
# -I$(TESS_INC) : Ruta a los headers de libtess2
# $(shell pkg-config --cflags freetype2) : Flags para FreeType2
# -Wall -Wextra : Más advertencias
# -g : Depuración
CFLAGS = -I$(SRC_DIR) -I$(TESS_INC) $(shell pkg-config --cflags freetype2) -Wall -Wextra -g

# Define flags de enlazado
# -L$(TESS_LIB) : Ruta a la librería libtess2
# $(shell pkg-config --libs freetype2) : Librerías para FreeType2
# -lGL -lGLEW -lglut : Librerías OpenGL
# -ltess2 : Librería libtess2
# -lm : Librería matemática
LDFLAGS = -L$(TESS_LIB) $(shell pkg-config --libs freetype2) -lGL -lGLEW -lglut -ltess2 -lm

# Define los archivos fuente (.c) buscando en SRC_DIR
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Genera los nombres de los archivos objeto (.o) en BUILD_DIR
# Reemplaza 'src/' con 'build/' y cambia la extensión '.c' a '.o'
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Define el nombre del ejecutable final (se creará en el directorio raíz '.')
EXEC = texto
# Opcional: si quieres el ejecutable dentro de build también:
# EXEC = $(BUILD_DIR)/texto

# Target por defecto: construye el ejecutable
all: $(EXEC)

# Regla para enlazar los archivos objeto (.o) y crear el ejecutable
# Asegura que el directorio BUILD_DIR existe antes de enlazar
$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)
	@echo "Ejecutable '$(EXEC)' creado exitosamente."

# Regla patrón para compilar archivos .c de SRC_DIR en archivos .o en BUILD_DIR
# $@ : Nombre del target (ej. build/main.o)
# $< : Nombre del primer prerrequisito (ej. src/main.c)
# Asegura que el directorio BUILD_DIR existe antes de compilar
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para crear el directorio BUILD_DIR si no existe
# Se usa como prerrequisito de orden único (|) para que se ejecute solo si es necesario
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@echo "Directorio '$(BUILD_DIR)' creado o ya existente."

# Target para limpiar: elimina el directorio BUILD_DIR y el ejecutable
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXEC)
	@echo "Limpieza completa (eliminado $(BUILD_DIR)/ y $(EXEC))."

# Targets "phony" que no representan archivos reales
.PHONY: all clean