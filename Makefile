# Define el compilador
CC = gcc

# Define directorios
SRC_DIR = src
BUILD_DIR = build
EXTERNAL_DIR = external
TESS_INC = $(EXTERNAL_DIR)/libtess2-1.0.2/Include
TESS_LIB_DIR = $(EXTERNAL_DIR)/libtess2-1.0.2/lib
TEST_SRC_DIR = tests
# TEST_BUILD_DIR ya está definido como $(BUILD_DIR)/tests en tu Makefile original

# Define flags de compilación
# CFLAGS para la aplicación principal y para módulos de src/ cuando se compilan para la app
APP_CFLAGS = -I$(SRC_DIR) -I$(TEST_SRC_DIR) -I$(TESS_INC) $(shell pkg-config --cflags freetype2 || echo "") -Wall -Wextra -g -std=c99 -pthread -Wno-unused-parameter
# CFLAGS para compilar cualquier cosa que forme parte de un ejecutable de PRUEBA
# Esto incluye los archivos *_test.c y los módulos de src/ que se recompilan para pruebas
TEST_CFLAGS = $(APP_CFLAGS) -DUNIT_TESTING

# Define flags de enlazado (sin cambios)
LDFLAGS_COMMON = -L$(TESS_LIB_DIR) -lm -pthread
LDFLAGS_FREETYPE = $(shell pkg-config --libs freetype2 || echo "-lfreetype")
LDFLAGS_OPENGL = -lGL -lGLEW -lglut 
LDFLAGS_TESS = -ltess2
STATIC_TESS_LIB = $(TESS_LIB_DIR)/libtess2.a


# Define los archivos fuente (.c) buscando en SRC_DIR
APP_SRCS = $(wildcard $(SRC_DIR)/*.c)
# Genera los nombres de los archivos objeto (.o) para la APP en BUILD_DIR (o un subdir como build/app_obj)
APP_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/app_obj/%.o,$(APP_SRCS))

# Define los archivos fuente de test (.c)
TEST_FREETYPE_SRC = $(TEST_SRC_DIR)/freetype_handler_test.c
TEST_TESSELLATION_SRC = $(TEST_SRC_DIR)/tessellation_handler_test.c
TEST_GLYPH_SRC = $(TEST_SRC_DIR)/glyph_manager_test.c

# Objetos de los archivos _test.c (compilados con TEST_CFLAGS)
TEST_FREETYPE_MAIN_OBJ = $(BUILD_DIR)/tests_obj/freetype_handler_test.o
TEST_TESSELLATION_MAIN_OBJ = $(BUILD_DIR)/tests_obj/tessellation_handler_test.o
TEST_GLYPH_MAIN_OBJ = $(BUILD_DIR)/tests_obj/glyph_manager_test.o

# Módulos de src/ compilados específicamente para pruebas (con TEST_CFLAGS)
TEST_MODULE_freetype_OBJ = $(BUILD_DIR)/tests_obj/freetype_handler_module.o # Nombre diferente para evitar colisión con app_obj
TEST_MODULE_tessellation_OBJ = $(BUILD_DIR)/tests_obj/tessellation_handler_module.o
TEST_MODULE_glyph_OBJ = $(BUILD_DIR)/tests_obj/glyph_manager_module.o
TEST_MODULE_utils_OBJ = $(BUILD_DIR)/tests_obj/utils_module.o # Si utils.c también necesitara -DUNIT_TESTING
# Si utils.c no necesita -DUNIT_TESTING, puedes usar el de la app: $(BUILD_DIR)/app_obj/utils.o


# Define el nombre del ejecutable final
EXEC = texto
# Define los nombres de los ejecutables de tests
TEST_FREETYPE_EXEC = $(BUILD_DIR)/freetype_test
TEST_TESSELLATION_EXEC = $(BUILD_DIR)/tessellation_test
TEST_GLYPH_EXEC = $(BUILD_DIR)/glyph_manager_test

# Directorios a crear
APP_OBJ_DIR_CREATE = $(BUILD_DIR)/app_obj
TEST_OBJS_DIR_CREATE = $(BUILD_DIR)/tests_obj

# Target por defecto: construye el ejecutable
all: $(EXEC)

# Regla para enlazar los archivos objeto (.o) y crear el ejecutable principal
$(EXEC): $(APP_OBJS) | $(BUILD_DIR) $(APP_OBJ_DIR_CREATE)
	$(CC) $(APP_OBJS) -o $(EXEC) $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE) $(LDFLAGS_OPENGL) $(LDFLAGS_TESS) $(STATIC_TESS_LIB)
	@echo "Ejecutable '$(EXEC)' creado exitosamente."

# Target para el target de test
test: $(TEST_FREETYPE_EXEC) $(TEST_TESSELLATION_EXEC) $(TEST_GLYPH_EXEC)
	@echo "\nRunning FreeType tests..."
	@./$(TEST_FREETYPE_EXEC)
	@echo "\nRunning Tessellation tests..."
	@./$(TEST_TESSELLATION_EXEC)
	@echo "\nRunning Glyph Manager tests..."
	@./$(TEST_GLYPH_EXEC)
	@echo "\nAll tests finished."

# Regla para enlazar el test de FreeType
$(TEST_FREETYPE_EXEC): $(TEST_FREETYPE_MAIN_OBJ) $(TEST_MODULE_freetype_OBJ) | $(BUILD_DIR) $(TEST_OBJS_DIR_CREATE)
	@echo "Linking test: $@"
	$(CC) $^ -o $@ $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE)
	@echo "Ejecutable de test '$@' creado exitosamente."

# Regla para enlazar el test de Tessellation
$(TEST_TESSELLATION_EXEC): $(TEST_TESSELLATION_MAIN_OBJ) $(TEST_MODULE_tessellation_OBJ) $(TEST_MODULE_freetype_OBJ) $(STATIC_TESS_LIB) | $(BUILD_DIR) $(TEST_OBJS_DIR_CREATE)
	@echo "Linking test: $@"
	$(CC) $(filter-out %.a,$^) $(filter %.a,$^) -o $@ $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE) $(LDFLAGS_TESS)
	@echo "Ejecutable de test '$@' creado exitosamente."

# Regla para enlazar el test de Glyph Manager
GLYPH_MANAGER_TEST_DEPS = $(TEST_GLYPH_MAIN_OBJ) \
                          $(TEST_MODULE_glyph_OBJ) \
                          $(TEST_MODULE_freetype_OBJ) \
                          $(TEST_MODULE_tessellation_OBJ) \
                          $(BUILD_DIR)/app_obj/utils.o # Asumimos que utils.o de app está bien
                          
$(TEST_GLYPH_EXEC): $(GLYPH_MANAGER_TEST_DEPS) $(STATIC_TESS_LIB) | $(BUILD_DIR) $(TEST_OBJS_DIR_CREATE) $(APP_OBJ_DIR_CREATE)
	@echo "Linking test: $@"
	$(CC) $(filter-out %.a,$^) $(filter %.a,$^) -o $@ $(LDFLAGS_COMMON) $(LDFLAGS_FREETYPE) $(LDFLAGS_OPENGL) $(LDFLAGS_TESS)
	@echo "Ejecutable de test '$@' creado exitosamente."

# --- Reglas de Compilación ---
# Regla patrón para compilar archivos .c de SRC_DIR para la APLICACIÓN
$(BUILD_DIR)/app_obj/%.o: $(SRC_DIR)/%.c | $(APP_OBJ_DIR_CREATE)
	$(CC) $(APP_CFLAGS) -c $< -o $@

# Regla patrón para compilar archivos _test.c (los que tienen el main de minunit)
$(BUILD_DIR)/tests_obj/%.o: $(TEST_SRC_DIR)/%.c | $(TEST_OBJS_DIR_CREATE)
	$(CC) $(TEST_CFLAGS) -c $< -o $@ # <--- USA TEST_CFLAGS (con -DUNIT_TESTING)

# Regla patrón para compilar archivos de src/ específicamente PARA PRUEBAS
$(BUILD_DIR)/tests_obj/%_module.o: $(SRC_DIR)/%.c | $(TEST_OBJS_DIR_CREATE)
	$(CC) $(TEST_CFLAGS) -c $< -o $@ # <--- USA TEST_CFLAGS (con -DUNIT_TESTING)


# Regla para crear los directorios BUILD_DIR etc. si no existen
$(BUILD_DIR) $(APP_OBJ_DIR_CREATE) $(TEST_OBJS_DIR_CREATE):
	@mkdir -p $@
	@echo "Directorio '$@' creado o ya existente."

# Target para limpiar: elimina el directorio BUILD_DIR y los ejecutables
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXEC) $(TEST_FREETYPE_EXEC) $(TEST_TESSELLATION_EXEC) $(TEST_GLYPH_EXEC)
	@echo "Limpieza completa."

# Targets "phony" que no representan archivos reales
.PHONY: all clean test