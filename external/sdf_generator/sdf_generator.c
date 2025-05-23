#include "sdf_generator.h"
#include <stdlib.h> // Para malloc, free
#include <string.h> // Para memset, memcpy
#include <float.h>  // Para FLT_MAX
#include <math.h>   // Para sqrtf, fminf, fabsf

// Estructura para un punto en el grid 2D (para cálculos de distancia)
typedef struct {
    short dx, dy; // Desplazamiento al pixel del borde más cercano
} PointDist;

// Valor grande para representar infinito en distancias de píxeles
#define SDF_INF_DIST 32767 // Max short / 2 aprox, para evitar overflow con dx*dx + dy*dy

// Helper para normalizar la distancia a un rango de 0-1
// spread: cuántos píxeles de distancia (desde el borde) se mapean al rango 0-1.
//         Un valor típico podría ser la mitad del padding o un poco más.
static unsigned char normalize_distance(float dist_val, float spread) {
    // Mapear distancias [-spread, +spread] al rango [0, 1]
    // donde 0.5 es el contorno (distancia 0)
    float normalized = dist_val / spread; // Ahora en [-1, 1] si dist_val está en [-spread, spread]
    float clamped = fmaxf(-1.0f, fminf(1.0f, normalized)); // Clampear a [-1, 1]
    unsigned char val = (unsigned char)((clamped * 0.5f + 0.5f) * 255.0f); // Mapear a [0, 255]
    return val;
}


// En t3d/external/sdf_generator/sdf_generator.c

unsigned char* generate_sdf_from_bitmap(
    const unsigned char* mono_bitmap_buffer,
    int width,      // Ancho del bitmap de entrada (sin padding)
    int height,     // Alto del bitmap de entrada (sin padding)
    int pitch,      // Pitch del bitmap de entrada
    int padding,    // Padding a añadir alrededor para el SDF
    int* out_sdf_width,
    int* out_sdf_height) {

    if (!mono_bitmap_buffer || width <= 0 || height <= 0) {
        if (out_sdf_width) *out_sdf_width = 0;
        if (out_sdf_height) *out_sdf_height = 0;
        return NULL;
    }

    int sdf_w = width + 2 * padding; // Mantenemos el tamaño con padding para que las dimensiones coincidan
    int sdf_h = height + 2 * padding;
    if (out_sdf_width) *out_sdf_width = sdf_w;
    if (out_sdf_height) *out_sdf_height = sdf_h;

    // Usaremos este buffer para devolver el bitmap umbralizado
    unsigned char* debug_threshold_bitmap = (unsigned char*)malloc(sdf_w * sdf_h);
    if (!debug_threshold_bitmap) {
        return NULL;
    }
    memset(debug_threshold_bitmap, 0, sdf_w * sdf_h); // Inicializar a negro (exterior)

    // Rellenar la parte central del debug_threshold_bitmap con el bitmap umbralizado
    unsigned char BORDER_THRESHOLD = 128; // Puedes probar con diferentes umbrales aquí también (64, 32, etc.)
                                         // para ver qué se considera "interior"

    for (int y_bm = 0; y_bm < height; ++y_bm) { // Iterar sobre el bitmap original
        for (int x_bm = 0; x_bm < width; ++x_bm) {
            // Coordenadas en el bitmap de debug (con padding)
            int current_debug_x = x_bm + padding;
            int current_debug_y = y_bm + padding;
            int debug_idx = current_debug_y * sdf_w + current_debug_x;

            if (mono_bitmap_buffer[y_bm * pitch + x_bm] >= BORDER_THRESHOLD) {
                debug_threshold_bitmap[debug_idx] = 255; // Blanco si es "interior"
            } else {
                debug_threshold_bitmap[debug_idx] = 0;   // Negro si es "exterior"
            }
        }
    }

    // NO HACEMOS EL CÁLCULO DE SDF, SOLO DEVOLVEMOS EL BITMAP UMBRALIZADO
    // free(grid1); // No se usan ahora
    // free(grid2); // No se usan ahora

    return debug_threshold_bitmap; // Devolver el bitmap umbralizado
}

// free_sdf_bitmap sigue igual, ya que solo hace free().

void free_sdf_bitmap(unsigned char* sdf_data) {
    if (sdf_data) {
        free(sdf_data);
    }
}