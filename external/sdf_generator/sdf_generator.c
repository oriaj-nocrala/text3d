#include <stdlib.h> // Para malloc, free
#include <string.h> // Para memset, memcpy
#include <float.h>  // Para FLT_MAX
#include <math.h>   // Para sqrtf, fminf, fabsf
#include <stdio.h>

// Estructura para un punto en el grid 2D (para cálculos de distancia)
typedef struct {
    short dx, dy; // Desplazamiento al pixel del borde más cercano
} PointDist;

// Valor grande para representar infinito en componentes de PointDist
#define SDF_INF_VAL 32767 // SHRT_MAX, usado como marcador de distancia infinita

// Helper para normalizar la distancia a un rango de 0-255
// spread: cuántos píxeles de distancia (desde el borde) se mapean al rango normalizado.
//         Distancias [-spread, +spread] se mapean a [0, 255], con 0.5 (127/128) en el contorno.
static unsigned char normalize_distance(float dist_val, float spread) {
    if (spread == 0.0f) { // Evitar división por cero; tratar como sin dispersión (0 o 255)
        if (dist_val == 0.0f) return 128; // Contorno exacto
        return (dist_val < 0.0f) ? 0 : 255;
    }
    // Mapear distancias [-spread, +spread] al rango [0, 1]
    // donde 0.5 es el contorno (distancia 0)
    float normalized = dist_val / spread; // Ahora en [-1, 1] si dist_val está en [-spread, spread]
    float clamped = fmaxf(-1.0f, fminf(1.0f, normalized)); // Clampear a [-1, 1]
    // Mapear de [-1, 1] a [0, 1] y luego a [0, 255]
    unsigned char val = (unsigned char)((clamped * 0.5f + 0.5f) * 255.0f);
    return val;
}

// Helper para comparar y actualizar PointDist durante el barrido de transformación de distancia
// Compara la distancia cuadrada actual de current_p con una nueva distancia calculada
// a partir de neighbor_p y los offsets.
static inline void compare_and_set(PointDist* current_p, const PointDist* neighbor_p, short dx_offset, short dy_offset) {
    // Si el vecino tiene distancia infinita, no se puede propagar desde él
    if (neighbor_p->dx == SDF_INF_VAL || neighbor_p->dy == SDF_INF_VAL) {
        return;
    }

    short test_dx = neighbor_p->dx + dx_offset;
    short test_dy = neighbor_p->dy + dy_offset;

    long current_dist_sq = (long)current_p->dx * current_p->dx + (long)current_p->dy * current_p->dy;
    long test_dist_sq = (long)test_dx * test_dx + (long)test_dy * test_dy;

    if (test_dist_sq < current_dist_sq) {
        current_p->dx = test_dx;
        current_p->dy = test_dy;
    }
}

// Aplica el algoritmo de transformación de distancia de barrido secuencial de 8 puntos (8SSEDT)
static void propagate_distances_8ssedt(PointDist* grid, int width, int height) {
    // Pass 1: Top-left to bottom-right
    // Vecinos (relativos a P(x,y)): N(x,y-1), NW(x-1,y-1), NE(x+1,y-1), W(x-1,y)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            PointDist* p = &grid[y * width + x];
            if (y > 0) { // N, NW, NE
                compare_and_set(p, &grid[(y - 1) * width + x], 0, 1);         // N
                if (x > 0) {
                    compare_and_set(p, &grid[(y - 1) * width + (x - 1)], 1, 1); // NW
                }
                if (x < width - 1) {
                    compare_and_set(p, &grid[(y - 1) * width + (x + 1)], -1, 1);// NE
                }
            }
            if (x > 0) { // W
                 compare_and_set(p, &grid[y * width + (x - 1)], 1, 0);        // W
            }
        }
    }

    // Pass 2: Bottom-right to top-left
    // Vecinos (relativos a P(x,y)): S(x,y+1), SE(x+1,y+1), SW(x-1,y+1), E(x+1,y)
    for (int y = height - 1; y >= 0; --y) {
        for (int x = width - 1; x >= 0; --x) {
            PointDist* p = &grid[y * width + x];
            if (y < height - 1) { // S, SW, SE
                compare_and_set(p, &grid[(y + 1) * width + x], 0, -1);        // S
                if (x > 0) {
                    compare_and_set(p, &grid[(y + 1) * width + (x - 1)], 1, -1); // SW
                }
                if (x < width - 1) {
                    compare_and_set(p, &grid[(y + 1) * width + (x + 1)], -1, -1);// SE
                }
            }
            if (x < width - 1) { // E
                compare_and_set(p, &grid[y * width + (x + 1)], -1, 0);       // E
            }
        }
    }
}

unsigned char* generate_sdf_from_bitmap(
    const unsigned char* mono_bitmap_buffer,
    int width,      // Ancho del bitmap de entrada (sin padding)
    int height,     // Alto del bitmap de entrada (sin padding)
    int pitch,      // Pitch del bitmap de entrada
    int padding,    // Padding a añadir alrededor para el SDF
    float spread,   // Spread para normalización (ej: valor del padding)
    int* out_sdf_width,
    int* out_sdf_height) {

    if (!mono_bitmap_buffer || width <= 0 || height <= 0 || padding < 0 || spread <= 0.0f) {
        if (out_sdf_width) *out_sdf_width = 0;
        if (out_sdf_height) *out_sdf_height = 0;
        return NULL;
    }

    int sdf_w = width + 2 * padding;
    int sdf_h = height + 2 * padding;
    if (out_sdf_width) *out_sdf_width = sdf_w;
    if (out_sdf_height) *out_sdf_height = sdf_h;

    // 1. Crear bitmap umbralizado con padding
    unsigned char* thresholded_bitmap = (unsigned char*)malloc(sdf_w * sdf_h);
    if (!thresholded_bitmap) {
        // No es necesario resetear out_sdf_width/height aquí si ya se asignaron arriba
        return NULL;
    }
    memset(thresholded_bitmap, 0, sdf_w * sdf_h); // Inicializar a exterior (negro)

    unsigned char BORDER_THRESHOLD = 128;
    for (int y_bm = 0; y_bm < height; ++y_bm) {
        for (int x_bm = 0; x_bm < width; ++x_bm) {
            int current_padded_x = x_bm + padding;
            int current_padded_y = y_bm + padding;
            int padded_idx = current_padded_y * sdf_w + current_padded_x;

            if (mono_bitmap_buffer[y_bm * pitch + x_bm] >= BORDER_THRESHOLD) {
                thresholded_bitmap[padded_idx] = 255; // Interior (blanco)
            } else {
                thresholded_bitmap[padded_idx] = 0;   // Exterior (negro)
            }
        }
    }

    // 2. Asignar memoria para grids y buffer de salida SDF
    PointDist* grid_outside = (PointDist*)malloc(sdf_w * sdf_h * sizeof(PointDist));
    PointDist* grid_inside  = (PointDist*)malloc(sdf_w * sdf_h * sizeof(PointDist));
    unsigned char* sdf_output_buffer = (unsigned char*)malloc(sdf_w * sdf_h);

    if (!grid_outside || !grid_inside || !sdf_output_buffer) {
        free(thresholded_bitmap);
        free(grid_outside); // free(NULL) es seguro
        free(grid_inside);
        free(sdf_output_buffer);
        // Resetear dimensiones de salida en caso de error de asignación parcial
        if (out_sdf_width) *out_sdf_width = 0;
        if (out_sdf_height) *out_sdf_height = 0;
        return NULL;
    }

    // 3. Inicializar grids
    // grid_outside: distancia al pixel 0 (exterior) más cercano
    // grid_inside:  distancia al pixel 255 (interior) más cercano
    PointDist zero_pt = {0, 0};
    PointDist inf_pt  = {SDF_INF_VAL, SDF_INF_VAL};

    for (int i = 0; i < sdf_w * sdf_h; ++i) {
        if (thresholded_bitmap[i] == 0) { // Pixel es exterior
            grid_outside[i] = zero_pt;
            grid_inside[i]  = inf_pt;
        } else { // Pixel es interior (thresholded_bitmap[i] == 255)
            grid_outside[i] = inf_pt;
            grid_inside[i]  = zero_pt;
        }
    }

    // 4. Propagar distancias (transformación de distancia)
    propagate_distances_8ssedt(grid_outside, sdf_w, sdf_h);
    propagate_distances_8ssedt(grid_inside, sdf_w, sdf_h);

    // 5. Combinar distancias y normalizar
    for (int i = 0; i < sdf_w * sdf_h; ++i) {
        long dist_out_sq = (long)grid_outside[i].dx * grid_outside[i].dx + (long)grid_outside[i].dy * grid_outside[i].dy;
        long dist_in_sq  = (long)grid_inside[i].dx * grid_inside[i].dx + (long)grid_inside[i].dy * grid_inside[i].dy;

        // Tomar la raíz cuadrada para obtener la distancia euclidiana real
        float actual_dist_out = sqrtf((float)dist_out_sq);
        float actual_dist_in  = sqrtf((float)dist_in_sq);

        float signed_distance;
        // Si el pixel en el bitmap umbralizado era exterior (0), la distancia es positiva.
        // Si era interior (255), la distancia es negativa.
        if (thresholded_bitmap[i] == 0) { // El píxel i es EXTERIOR
            // La distancia al borde (píxel 255 más cercano) es actual_dist_in. Debe ser positiva.
            signed_distance = actual_dist_in;
        } else { // El píxel i es INTERIOR (thresholded_bitmap[i] == 255)
            // La distancia al borde (píxel 0 más cercano) es actual_dist_out. Debe ser negativa.
            signed_distance = -actual_dist_out;
        }

        // ----- INICIO DEBUG -----
        if (i % (sdf_w * 10) == 0 && i / sdf_w < 10) { // Imprime para algunos píxeles de las primeras 10 filas
            printf("SDF_GEN_DEBUG: pixel_idx=%d (x=%d, y=%d), thresh_val=%u, d_out=%.2f, d_in=%.2f, signed_dist=%.2f\n",
                i, i % sdf_w, i / sdf_w,
                thresholded_bitmap[i], actual_dist_out, actual_dist_in, signed_distance);
        }
        // ----- FIN DEBUG -----

        sdf_output_buffer[i] = normalize_distance(signed_distance, spread);
    }

    // Liberar memoria temporal
    free(thresholded_bitmap);
    free(grid_outside);
    free(grid_inside);

    return sdf_output_buffer; // Devolver el bitmap SDF calculado
}

// La función free_sdf_bitmap proporcionada sigue siendo válida
void free_sdf_bitmap(unsigned char* sdf_data) {
    if (sdf_data) {
        free(sdf_data);
    }
}