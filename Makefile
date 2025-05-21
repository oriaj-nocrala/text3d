# Define el compilador
CC = gcc

# Define directorios
SRC_DIR = src
BUILD_DIR = build
EXTERNAL_DIR = external
TESS_INC = $(EXTERNAL_DIR)/libtess2-1.0.2/Include
TESS_LIB_DIR = $(EXTERNAL_DIR)/libtess2-1.0.2/lib
TEST_SRC_DIR = tests
TEST_BUILD_DIR = $(BUILD_DIR)/tests

# Define flags de compilación
CFLAGS = -I$(SRC_DIR) -I$(TEST_SRC_DIR) -I$(TESS_INC) $(shell pkg-config --cflags freetype2 || echo "") -Wall -Wextra -g -std=c99 -pthread -Wno-unused-parameter

# Define flags de enlazado
LDFLAGS_COMMON = -L$(TESS_LIB_DIR) -lm -pthread
LDFLAGS_FREETYPE = $(shell pkg-config --libs freetype2 || echo "-lfreetype")
LDFLAGS_OPENGL = -lGL -lGLEW -lglut 
LDFLAGS_TESS = -ltess2

# Define los archivos fuente (.c) buscando en SRC_DIR
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Genera los nombres de los archivos objeto (.o) en BUILD_DIR
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Define los archivos fuente de test (.c)
TEST_FREETYPE_SRC = $(TEST_SRC_DIR)/freetype_handler_test.c
TEST_TESSELLATION_SRC = $(TEST_SRC_DIR)/tessellation_handler_test.c
TEST_GLYPH_SRC = $(TEST_SRC_DIR)/glyph_manager_test.c

# Genera los nombres de los archivos objeto de test (.o) en TEST_BUILD_DIR
TEST_FREETYPE_OBJ = $(TEST_BUILD_DIR)/freetype_handler_test.o
TEST_TESSELLATION_OBJ = $(TEST_BUILD_DIR)/tessellation_handler_test.o
TEST_GLYPH_OBJ = $(TEST_BUILD_DIR)/glyph_manager_test.o

# Define el nombre del ejecutable final (se creará en el directorio raíz '.')
EXEC = texto
# Define los nombres de los ejecutables de tests
TEST_FREETYPE_EXEC = $(TEST_BUILD_DIR)/freetype_test
TEST_TESSELLATION_EXEC = $(TEST_BUILD_DIR)/tessellation_test
TEST_GLYPH_EXEC = $(TEST_BUILD_DIR)/glyph_manager_test

# Target por defecto: construye el ejecutable
all: $(EXEC)

# Regla para enlazar los archivos objeto (.o) y crear el ejecutable principal
$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE) $(LDFLAGS_OPENGL) $(LDFLAGS_TESS)
	@echo "Ejecutable '$(EXEC)' creado exitosamente."

# Regla para el target de test
test: $(TEST_FREETYPE_EXEC) $(TEST_TESSELLATION_EXEC) $(TEST_GLYPH_EXEC)
	@echo "Running FreeType tests..."
	@$(TEST_FREETYPE_EXEC)
	@echo "\nRunning Tessellation tests..."
	@$(TEST_TESSELLATION_EXEC)
	@echo "\nRunning Glyph Manager tests..."
	@$(TEST_GLYPH_EXEC)
	@echo "\nAll tests finished."

# Regla para enlazar el test de FreeType
$(TEST_FREETYPE_EXEC): $(TEST_FREETYPE_OBJ) $(BUILD_DIR)/freetype_handler.o | $(TEST_BUILD_DIR)
	@echo "Linking test: $@"
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE)
	@echo "Ejecutable de test '$(TEST_FREETYPE_EXEC)' creado exitosamente."

# Regla para enlazar el test de Tessellation
$(TEST_TESSELLATION_EXEC): $(TEST_TESSELLATION_OBJ) $(BUILD_DIR)/tessellation_handler.o $(BUILD_DIR)/freetype_handler.o $(TESS_LIB_DIR)/libtess2.a | $(TEST_BUILD_DIR)
	@echo "Linking test: $@"
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE)
	@echo "Ejecutable de test '$(TEST_TESSELLATION_EXEC)' creado exitosamente."

# Regla para enlazar el test de Glyph Manager
$(TEST_GLYPH_EXEC): $(TEST_GLYPH_OBJ) $(BUILD_DIR)/glyph_manager.o $(BUILD_DIR)/freetype_handler.o $(BUILD_DIR)/tessellation_handler.o $(TESS_LIB_DIR)/libtess2.a | $(TEST_BUILD_DIR)
	@echo "Linking test: $@"
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE) $(LDFLAGS_OPENGL) $(LDFLAGS_TESS)
	@echo "Ejecutable de test '$(TEST_GLYPH_EXEC)' creado exitosamente."

# Regla patrón para compilar archivos .c de SRC_DIR en archivos .o en BUILD_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla patrón para compilar archivos _test.c de TEST_SRC_DIR en archivos .o en TEST_BUILD_DIR
$(TEST_BUILD_DIR)/%.o: $(TEST_SRC_DIR)/%.c | $(TEST_BUILD_DIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para crear los directorios BUILD_DIR y TEST_BUILD_DIR si no existen
# Se usa como prerrequisito de orden único (|)
$(BUILD_DIR) $(TEST_BUILD_DIR):
	@mkdir -p $@
	@echo "Directorio '$@' creado o ya existente."

# Target para limpiar: elimina el directorio BUILD_DIR y los ejecutables de test y principal
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXEC) $(TEST_FREETYPE_EXEC) $(TEST_TESSELLATION_EXEC) $(TEST_GLYPH_EXEC)
	@echo "Limpieza completa."

# Targets "phony" que no representan archivos reales
.PHONY: all clean test